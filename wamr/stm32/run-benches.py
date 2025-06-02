from pathlib import Path
from ctypes import *
import sys
import subprocess
import serial
from datetime import datetime
import traceback
import re
import io
from shutil import rmtree
import time
import socket
import os
from pygdbmi.gdbcontroller import GdbController
from pprint import pprint
from multiprocessing import Pool
import argparse

VERBOSE = os.environ.get('VERBOSE')
date = None
glob = {}

class SWOReader(io.RawIOBase):
    def __init__(self, wrapped):
        self.wrapped = wrapped

    @staticmethod
    def buffered(wrapped):
        return io.BufferedReader(SWOReader(wrapped))
    
    def readable(self):
        return True
    
    def readinto(self, b):
        data = b''
        while len(data) < 1:
            data = self.wrapped.recv(1, socket.MSG_WAITALL)
        expected = int(data[0])
        if expected > 3 or expected & 0x3 == 0:
            print("WARNING: trying to fix corrupted packet")
            b[0] = expected
            return 1
        expected = [0, 1, 2, 4][expected]
        data = bytearray()
        while len(data) < expected:
            data += bytearray(self.wrapped.recv(expected - len(data), socket.MSG_WAITALL))
        b[0:len(data)] = data
        assert(len(data) == expected)
        return expected

    def write(self, b):
        raise io.UnsupportedOperation()


def main(benchpath, outpath, benches, configuration, outname, aot_flag, semihosted_flag):
    coremark_flag = "coremark" in configuration
    embench_flag = "embench" in configuration
    configuration += "-aot" if aot_flag else ""
    configuration += "-semihosted" if semihosted_flag else "-standalone"
    p = Path(benchpath)
    benches = [p.stem.replace("-", "_") for p in list(p.glob("**/*.wasm"))] if not benches else [b.replace("-", "_") for b in benches]
    sizes = {}
    measurements = {}
    for name in benches:
        (delay1, delay2, stack, heap) = (-1, -1, -1, -1)
        text, data = (-1, -1)
        for trace_flag in [True, False]:
            print("start")
            clear_build_dir()
            try:
                gdbc = None
                print(f"building {name}.bin")
                build_bin(name, trace_flag, aot_flag, embench_flag)
                text, data = get_size()
                print(f"{name}.bin size: text = {text}, data = {data}")
                with Pool(processes=3) as pl:
                    print(f"flashing {name}")
                    measure2 = pl.map_async(get_heap, [None])
                    measure1 = pl.map_async(get_measurements, [coremark_flag])
                    gdbc = flash_bin(name)
                    time.sleep(1)
                    print("getting measurements")
                    start_bin(gdbc)
                    if trace_flag:
                        heap = measure2.get()[0]
                        print(f"got heap: {heap}")
                        measure1.wait()
                    else:
                        delay1, delay2, stack = measure1.get()[0]
            except ValueError as err:
                print(f"Unexpected {err=}, {type(err)=}")
                traceback.print_exc()
                continue
            finally:
                gdbc.exit()
                time.sleep(3)
        if delay1 > -1:
            measurements[name] = (delay1, delay2, stack, heap)
            sizes[name] = (text, data)
    write_csv(f"{outpath}", sizes, measurements, configuration + f"_{outname}")

def write_csv(path, sizes, measurements, extension):
    with open(f"{path}/{date}__{extension}.csv", mode='w') as f:
        for name in sizes:
            size = sizes[name]
            measurement = measurements[name]
            f.write(f"{name},{size[0]},{size[1]},{measurement[0]},{measurement[1]},{measurement[2]},{measurement[3]}\n")

def clear_build_dir():
    for path in Path("./build").glob("*"):
        print(path)
        if path.is_file():
            path.unlink()
        elif path.is_dir():
            rmtree(path)

def build_bin(name, trace_heap, aot_flag, embench_flag):
    args = ["cmake", "..", f"-DBENCHMARK={name}", "-DCMAKE_BUILD_TYPE=Release"]
    if trace_heap:
        args.append("-DHEAP_TRACE=1")
    if aot_flag:
        args.append("-DAOT=1")
    if embench_flag:
        args.append("-DEMBENCH=1")
    with subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, cwd="./build") as p:
        ret = p.wait()
        if ret != 0:
            raise Exception("Unsuccessful cmake")
    with subprocess.Popen(["make"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, cwd="./build") as p:
        ret = p.wait()
        if ret != 0:
            raise Exception("Unsuccessful make")

def get_size():
    with subprocess.Popen(["size", "wamr"], cwd="./build", stdout=subprocess.PIPE, text=True) as p:
        out, err = p.communicate()
        match = re.search(r"^\s*(?P<text>\d+)\s*(?P<data>\d+)\s*(?P<bss>\d+)\s*(?P<dec>\d+)", out, re.MULTILINE)
        text = int(match.group('text'))
        data = int(match.group('data')) + int(match.group('bss'))
        return text, data

class M3Realloc(Structure):
    _pack_ = 1
    _fields_ = [
        ("old", c_uint32),
        ("new", c_uint32),
        ("size", c_uint32)
    ]
class M3Malloc(Structure):
    _pack_ = 1
    _fields_ = [
        ("new", c_uint32),
        ("size", c_uint32)
    ]
class M3Free(Structure):
    _pack_ = 1
    _fields_ = [
        ("old", c_uint32)
    ]
class M3HeapTraceUnion(Union):
    _pack_ = 1
    _fields_ = [
        ("realloc", M3Realloc),
        ("malloc", M3Malloc),
        ("free", M3Free)
    ]

class M3HeapTrace(Structure):
    _pack_ = 1
    _fields_ = [
        ("tag", c_int8),
        ("_as", M3HeapTraceUnion)
    ]

def get_heap(unused):
    total = bytearray()
    while True:
        next_line = stream.readline()
        if re.fullmatch(rb".*trace ready!\n$", next_line, re.DOTALL) != None:
            print("found trace ready!")
            total += next_line
            break
    while True:
        next_line = stream.readline()
        total += next_line
        if re.fullmatch(rb".*TRACE_DONE\n$", next_line) != None:
            break
    # if VERBOSE != None:
    #     print(str(total))
    data = re.match(rb".*trace ready!\n(?P<data>.*?)TRACE_DONE\n", total, re.DOTALL).group('data')
    packets = [
        M3HeapTrace.from_buffer_copy(data, i)
        for i in range(0, len(data), sizeof(M3HeapTrace))]
    memmap = {}
    peak = 0
    total = 0
    for v in packets:
        if v.tag == 0:
            # malloc
            ptr = v._as.malloc.new
            if ptr == 0:
                continue
            size = v._as.malloc.size
            assert(memmap.get(ptr, None) == None)
            memmap[ptr] = size
            # print("malloc: ", size)
            total += size
        elif v.tag == 1:
            # realloc
            new = v._as.realloc.new
            size = v._as.realloc.size
            old = v._as.realloc.old
            if new == 0:
                continue
            if old != 0:
                old_size = memmap[old]
                del(memmap[old])
            else:
                old_size = 0
            memmap[new] = size
            # print("realloc: ", size)
            total += size - old_size
        elif v.tag == 2:
            # free
            old = v._as.free.old
            if old == 0:
                continue
            if old not in memmap:
                print("potential double free")
                continue
            old_size = memmap[old]
            del(memmap[old])
            total -= old_size
        else:
            raise ValueError("unknown type")
        peak = max(peak, total)
    return peak

def get_measurements(coremark_flag):
    with serial.Serial("/dev/ttyACM0", 115200, serial.EIGHTBITS, serial.PARITY_NONE, serial.STOPBITS_ONE, timeout=120) as ser:
        y = bytearray()
        y.extend(ser.read_until(b'END OF TEST'))
        s = str(y, 'utf-8')
        if VERBOSE is not None:
            print(s)
        first = re.search(r"First runtime delay: (\d+)ms", s)
        second = re.search(r"Second runtime delay: (\d+)ms", s)
        delay1 = int(first.group(1))
        if second is not None:
            delay2 = int(second.group(1))
        else:
            delay2 = -1
        match = re.search(r"Max stack use: (\d+)", s)
        stack = int(match.group(1))
        if coremark_flag:
            match = re.search(r"CoreMark 1.0 : (\d+\.\d?)", s)
            score = float(match.group(1))
            print(f"Coremark score: {score}")
        return delay1, delay2, stack

def flash_bin(name):
    gdbc = GdbController(command=[glob["gdb"], "--interpreter=mi3"],
        time_to_check_for_additional_output_sec=3)
    gdbc.write('-target-select extended-remote :3333')
    gdbc.write('-file-exec-and-symbols ./build/wamr')
    gdbc.write('-target-download', 10)
    gdbc.write('-interpreter-exec console \"monitor reset halt\"', 10)
    gdbc.write('-break-delete', 10)
    gdbc.write('-break-insert post_main', 10)
    gdbc.write('-exec-continue')
    return gdbc

def start_bin(gdbc):
    gdbc.write('-break-delete', 10)
    gdbc.write('-exec-continue')

def boolean(s):
    return True if s in {"1", "true", "True"} else False

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Build and execute WAMR benchmarks",
    )
    parser.add_argument("sources", help="Directory with wasm benchmark sources")
    parser.add_argument("results", help="Directory to save benchmark results")
    parser.add_argument("config", choices=["embench", "coremark"])
    parser.add_argument("--outname", type=str, default="")
    parser.add_argument("--aot", default=False, type=boolean)
    parser.add_argument("--date", default=datetime.now(), type=datetime.fromisoformat)
    parser.add_argument("--semihosted", default=False, type=boolean)
    parser.add_argument("--gdb", default="arm-none-eabihf-gdb")
    parser.add_argument("--benches", default=[], type=str, nargs="*", help="List of benchmark names to run. Defaults to all.")
    args = parser.parse_args()
    date = args.date.strftime('%m-%d_%H-%M-%S')
    glob["gdb"] = args.gdb
    sock = socket.create_connection(("localhost", 2332))
    stream = SWOReader.buffered(sock)
    main(args.sources, args.results, args.benches, args.config, args.outname, args.aot, args.semihosted)