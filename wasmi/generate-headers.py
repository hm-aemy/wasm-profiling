PATH = __file__
from pathlib import Path
import sys
SOURCE_DIR = sys.argv[1]
OUT_DIR = sys.argv[2]
AOT_FLAG = len(sys.argv) >= 4 and sys.argv[3] == 'aot'

def main():
    p = Path(SOURCE_DIR)
    if AOT_FLAG:
        benchmarks = list(p.glob("**/*.aot"))
    else:
        benchmarks = list(p.glob("**/*.wasm"))
    defs = []
    for path in benchmarks:
        name = path.stem.replace("-", "_")
        with open(path, mode='rb') as file:
            content = file.read()
            defs.append(generate_array(name, content))
    content = "\n".join(defs)
    header = f"""#ifndef H_BENCHMARKS
#define H_BENCHMARKS
#include <stdint.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored \"-Wunused-variable\"

{content}

#pragma GCC diagnostic pop
#endif
    """
    with open(Path(OUT_DIR) / "./benchmarks.h", mode='w') as output:
        output.write(header)

def generate_array(name, content):
    content = " ,".join([f'0x{byte:x}'  for byte in content])
    content = f'static uint8_t {name}[] __attribute__((aligned (4))) = {{{content}}};\n'
    return content

if __name__ == "__main__":
    main()