#include "benchmarks.h"
#include "benchmarks-defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wasm_export.h>
#include <wasm_c_api.h>
#include <lib_export.h>

typedef int (*module_hook)(wasm_module_inst_t *module, wasm_val_t *args);

#define expander(B, m) m(B)
#define _TEST_knucleotide 1
#define _TEST_reverse_complement 2
#define _test(benchname) _TEST_##benchname
#define _TEST_result expander(BENCHMARK, _test)

uint32_t register_wasi();
int run_bench(uint8_t *mod, size_t mod_size, wasm_val_t *args, size_t args_len,
              wasm_val_t *results, size_t results_len, module_hook hook, uint32_t heap_size)
{
    char error_buf[128];
    wasm_module_t module;
    wasm_module_inst_t module_inst;
    wasm_function_inst_t func;
    wasm_exec_env_t exec_env;
    uint32_t stack_size = 1 << 13;

    /* initialize the wasm runtime by default configurations */
    RuntimeInitArgs runtime_args = {
        .mem_alloc_type = Alloc_With_System_Allocator,
        .running_mode = Mode_Interp,
    };
    clock_t start = clock();
    __sync_synchronize();
    wasm_runtime_full_init(&runtime_args);

    if (!register_wasi())
    {
        printf("Error registering native functions.\n");
        return 1;
    }
    /* parse the WASM file from buffer and create a WASM module */
    module = wasm_runtime_load(mod, mod_size, error_buf, sizeof(error_buf));

    /* create an instance of the WASM module (WASM linear memory is ready) */
    module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                           error_buf, sizeof(error_buf));

    if (hook && hook(&module_inst, args))
    {
        printf("error running module hook!\n");
        return 1;
    }

    // func = wasm_runtime_lookup_function(module_inst, "_initialize");
    exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
    // if (func && !wasm_runtime_call_wasm_a(exec_env, func, 0, NULL, 0, NULL))
    // {
        // printf("error initializing wasm module!\n%s\n", wasm_runtime_get_exception(module_inst));
        // return 1;
    // }
    func = wasm_runtime_lookup_function(module_inst, "_run");
    if (!wasm_runtime_call_wasm_a(exec_env, func, results_len, results, args_len, args))
    {
        printf("error executing wasm function!\n%s\n", wasm_runtime_get_exception(module_inst));
        return 1;
    }
    __sync_synchronize();
    clock_t end = clock();
    printf("First runtime delay: %ldms\n", (end - start) * 1000 / (CLOCKS_PER_SEC));
#ifndef HEAP_TRACE
    start = clock();
    __sync_synchronize();
    if (!wasm_runtime_call_wasm_a(exec_env, func, results_len, results, args_len, args))
    {
        printf("error executing wasm function!\n%s\n", wasm_runtime_get_exception(module_inst));
        return 1;
    }
    __sync_synchronize();
    end = clock();
    printf("Second runtime delay: %ldms\n", (end - start) * 1000 / (CLOCKS_PER_SEC));
#endif
    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_deinstantiate(module_inst);
    wasm_runtime_unload(module);
    wasm_runtime_destroy();
    return 0;
}
#ifdef EMBENCH
#define FUN_NAME_GEN(b) run_##b
#define FUN_NAME expander(BENCHMARK, FUN_NAME_GEN)
__attribute__((weak)) bench_result FUN_NAME(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    wasm_val_t results[1];
    int err = run_bench(BENCH, SIZE, NULL, 0, results, 1, NULL, 1 << 13);
    if (!err)
    {
        printf("BENCHMARK"
               " result: %ld\n",
               results[0].of.i32);
    }
    return err;
}
#undef FUN_NAME
#else
static bench_result run_fannkuch_redux(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    wasm_val_t results[1];
    wasm_val_t funargs[1];
    funargs[0].kind = WASM_I32;
    funargs[0].of.i32 = 8;
    int err = run_bench(BENCH, SIZE, funargs, 1, results, 1, NULL, 1 << 13);
    if (!err)
    {
        printf("fannkuch-redux result: %ld\n", results[0].of.i32);
    }
    return err;
}
static bench_result run_coremark(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    wasm_val_t results[1];
    int err = run_bench(BENCH, SIZE, NULL, 0, results, 1, NULL, 1 << 13);
    if (!err)
    {
        printf("coremark result: %ld\n", results[0].of.i32);
    }
    return err;
}
static bench_result run_coremark_standalone(bench_args args) __attribute__((alias("run_coremark")));
static bench_result run_coremark_semihosted(bench_args args) __attribute__((alias("run_coremark")));
static bench_result run_binary_trees(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    wasm_val_t results[1];
    wasm_val_t funargs[1];
    funargs[0].kind = WASM_I32;
    funargs[0].of.i32 = 9;
    int err = run_bench(BENCH, SIZE, funargs, 1, results, 1, NULL, (1 << 15) + (1 << 14)) | results[0].of.i32;
    if (!err)
    {
        printf("binary-trees result: %ld\n", results[0].of.i32);
    }
    return err;
}
static bench_result run_dhrystone_semihosted(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    wasm_val_t results[1];
    wasm_val_t funargs[1];
    funargs[0].kind = WASM_I32;
    funargs[0].of.i32 = 100000;
    int err = run_bench(BENCH, SIZE, funargs, 1, results, 1, NULL, (1 << 15) + (1 << 14));
    if (!err)
    {
        printf("dhrystone result: %ld\n", results[0].of.i32);
    }
    return err;
}
static bench_result run_dhrystone_standalone(bench_args args) __attribute__((alias("run_dhrystone_semihosted")));
static bench_result run_nbody(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    wasm_val_t results[1];
    wasm_val_t funargs[1];
    funargs[0].kind = WASM_I32;
    funargs[0].of.i32 = 5000;
    int err = run_bench(BENCH, SIZE, funargs, 1, results, 1, NULL, 1 << 13);
    if (!err)
    {
        printf("nbody result: %ld\n", results[0].of.i32);
    }
    return err;
}
static bench_result run_spectral_norm(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    wasm_val_t results[1];
    wasm_val_t funargs[1];
    funargs[0].kind = WASM_I32;
    funargs[0].of.i32 = 100;
    int err = run_bench(BENCH, SIZE, funargs, 1, results, 1, NULL, 1 << 13);
    if (!err)
    {
        printf("spectral_norm result: %ld\n", results[0].of.i32);
    }
    return err;
}

static bench_result run_fasta(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    wasm_val_t results[1];
    wasm_val_t funargs[1];
    funargs[0].kind = WASM_I32;
    funargs[0].of.i32 = 10000;
    int err = run_bench(BENCH, SIZE, funargs, 1, results, 1, NULL, 1 << 14);
    if (!err)
    {
        printf("fasta result: %ld\n", results[0].of.i32);
    }
    return err;
}
#if _TEST_result == _TEST_knucleotide || _TEST_result == _TEST_reverse_complement
static const char input[] = ">ONE Homo sapiens alu\n"
                            "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGA\n"
                            "TCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACT\n"
                            "AAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAG\n"
                            "GCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCG\n"
                            "CCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAAGGCCGGGCGCGGT\n"
                            "GGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCA\n"
                            "GGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAATACAAAAA\n"
                            "TTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAG\n"
                            "AATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCCA\n"
                            "GCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAAGGCCGGGCGCGGTGGCTCACGCCTGT\n"
                            "AATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGACC\n"
                            "AGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAATACAAAAATTAGCCGGGCGTG\n"
                            "GTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACC\n"
                            "CGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCCAGCCTGGGCGACAG\n"
                            "AGCGAGACTCCGTCTCAAAAAGGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTT\n"
                            "TGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACA\n"
                            "TGGTGAAACCCCGTCTCTACTAAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCT\n"
                            "GTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGG\n"
                            "TTGCAGTGAGCCGAGATCGCGCCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGT\n"
                            "CTCAAAAAGGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGG\n"
                            "CGGGCGGATCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCG\n"
                            "TCTCTACTAAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTA\n"
                            "CTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCG\n"
                            "AGATCGCGCCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAAGGCCG\n"
                            "GGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACC\n"
                            "TGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAA\n"
                            "TACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGA\n"
                            "GGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACT\n"
                            "GCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAAGGCCGGGCGCGGTGGCTC\n"
                            "ACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGT\n"
                            "TCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAATACAAAAATTAGC\n"
                            "CGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCG\n"
                            "CTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCCAGCCTG\n"
                            "GGCGACAGAGCGAGACTCCG\n"
                            ">TWO IUB ambiguity codes\n"
                            "cttBtatcatatgctaKggNcataaaSatgtaaaDcDRtBggDtctttataattcBgtcg\n"
                            "tactDtDagcctatttSVHtHttKtgtHMaSattgWaHKHttttagacatWatgtRgaaa\n"
                            "NtactMcSMtYtcMgRtacttctWBacgaaatatagScDtttgaagacacatagtVgYgt\n"
                            "cattHWtMMWcStgttaggKtSgaYaaccWStcgBttgcgaMttBYatcWtgacaYcaga\n"
                            "gtaBDtRacttttcWatMttDBcatWtatcttactaBgaYtcttgttttttttYaaScYa\n"
                            "HgtgttNtSatcMtcVaaaStccRcctDaataataStcYtRDSaMtDttgttSagtRRca\n"
                            "tttHatSttMtWgtcgtatSSagactYaaattcaMtWatttaSgYttaRgKaRtccactt\n"
                            "tattRggaMcDaWaWagttttgacatgttctacaaaRaatataataaMttcgDacgaSSt\n"
                            "acaStYRctVaNMtMgtaggcKatcttttattaaaaagVWaHKYagtttttatttaacct\n"
                            "tacgtVtcVaattVMBcttaMtttaStgacttagattWWacVtgWYagWVRctDattBYt\n"
                            "gtttaagaagattattgacVatMaacattVctgtBSgaVtgWWggaKHaatKWcBScSWa\n"
                            "accRVacacaaactaccScattRatatKVtactatatttHttaagtttSKtRtacaaagt\n"
                            "RDttcaaaaWgcacatWaDgtDKacgaacaattacaRNWaatHtttStgttattaaMtgt\n"
                            "tgDcgtMgcatBtgcttcgcgaDWgagctgcgaggggVtaaScNatttacttaatgacag\n"
                            "cccccacatYScaMgtaggtYaNgttctgaMaacNaMRaacaaacaKctacatagYWctg\n"
                            "ttWaaataaaataRattagHacacaagcgKatacBttRttaagtatttccgatctHSaat\n"
                            "actcNttMaagtattMtgRtgaMgcataatHcMtaBSaRattagttgatHtMttaaKagg\n"
                            "YtaaBataSaVatactWtataVWgKgttaaaacagtgcgRatatacatVtHRtVYataSa\n"
                            "KtWaStVcNKHKttactatccctcatgWHatWaRcttactaggatctataDtDHBttata\n"
                            "aaaHgtacVtagaYttYaKcctattcttcttaataNDaaggaaaDYgcggctaaWSctBa\n"
                            "aNtgctggMBaKctaMVKagBaactaWaDaMaccYVtNtaHtVWtKgRtcaaNtYaNacg\n"
                            "gtttNattgVtttctgtBaWgtaattcaagtcaVWtactNggattctttaYtaaagccgc\n"
                            "tcttagHVggaYtgtNcDaVagctctctKgacgtatagYcctRYHDtgBattDaaDgccK\n"
                            "tcHaaStttMcctagtattgcRgWBaVatHaaaataYtgtttagMDMRtaataaggatMt\n"
                            "ttctWgtNtgtgaaaaMaatatRtttMtDgHHtgtcattttcWattRSHcVagaagtacg\n"
                            "ggtaKVattKYagactNaatgtttgKMMgYNtcccgSKttctaStatatNVataYHgtNa\n"
                            "BKRgNacaactgatttcctttaNcgatttctctataScaHtataRagtcRVttacDSDtt\n"
                            "aRtSatacHgtSKacYagttMHtWataggatgactNtatSaNctataVtttRNKtgRacc\n"
                            "tttYtatgttactttttcctttaaacatacaHactMacacggtWataMtBVacRaSaatc\n"
                            "cgtaBVttccagccBcttaRKtgtgcctttttRtgtcagcRttKtaaacKtaaatctcac\n"
                            "aattgcaNtSBaaccgggttattaaBcKatDagttactcttcattVtttHaaggctKKga\n"
                            "tacatcBggScagtVcacattttgaHaDSgHatRMaHWggtatatRgccDttcgtatcga\n"
                            "aacaHtaagttaRatgaVacttagattVKtaaYttaaatcaNatccRttRRaMScNaaaD\n"
                            "gttVHWgtcHaaHgacVaWtgttScactaagSgttatcttagggDtaccagWattWtRtg\n"
                            "ttHWHacgattBtgVcaYatcggttgagKcWtKKcaVtgaYgWctgYggVctgtHgaNcV\n"
                            "taBtWaaYatcDRaaRtSctgaHaYRttagatMatgcatttNattaDttaattgttctaa\n"
                            "ccctcccctagaWBtttHtBccttagaVaatMcBHagaVcWcagBVttcBtaYMccagat\n"
                            "gaaaaHctctaacgttagNWRtcggattNatcRaNHttcagtKttttgWatWttcSaNgg\n"
                            "gaWtactKKMaacatKatacNattgctWtatctaVgagctatgtRaHtYcWcttagccaa\n"
                            "tYttWttaWSSttaHcaaaaagVacVgtaVaRMgattaVcDactttcHHggHRtgNcctt\n"
                            "tYatcatKgctcctctatVcaaaaKaaaagtatatctgMtWtaaaacaStttMtcgactt\n"
                            "taSatcgDataaactaaacaagtaaVctaggaSccaatMVtaaSKNVattttgHccatca\n"
                            "cBVctgcaVatVttRtactgtVcaattHgtaaattaaattttYtatattaaRSgYtgBag\n"
                            "aHSBDgtagcacRHtYcBgtcacttacactaYcgctWtattgSHtSatcataaatataHt\n"
                            "cgtYaaMNgBaatttaRgaMaatatttBtttaaaHHKaatctgatWatYaacttMctctt\n"
                            "ttVctagctDaaagtaVaKaKRtaacBgtatccaaccactHHaagaagaaggaNaaatBW\n"
                            "attccgStaMSaMatBttgcatgRSacgttVVtaaDMtcSgVatWcaSatcttttVatag\n"
                            "ttactttacgatcaccNtaDVgSRcgVcgtgaacgaNtaNatatagtHtMgtHcMtagaa\n"
                            "attBgtataRaaaacaYKgtRccYtatgaagtaataKgtaaMttgaaRVatgcagaKStc\n"
                            "tHNaaatctBBtcttaYaBWHgtVtgacagcaRcataWctcaBcYacYgatDgtDHccta\n"
                            ">THREE Homo sapiens frequency\n"
                            "aacacttcaccaggtatcgtgaaggctcaagattacccagagaacctttgcaatataaga\n"
                            "atatgtatgcagcattaccctaagtaattatattctttttctgactcaaagtgacaagcc\n"
                            "ctagtgtatattaaatcggtatatttgggaaattcctcaaactatcctaatcaggtagcc\n"
                            "atgaaagtgatcaaaaaagttcgtacttataccatacatgaattctggccaagtaaaaaa\n"
                            "tagattgcgcaaaattcgtaccttaagtctctcgccaagatattaggatcctattactca\n"
                            "tatcgtgtttttctttattgccgccatccccggagtatctcacccatccttctcttaaag\n"
                            "gcctaatattacctatgcaaataaacatatattgttgaaaattgagaacctgatcgtgat\n"
                            "tcttatgtgtaccatatgtatagtaatcacgcgactatatagtgctttagtatcgcccgt\n"
                            "gggtgagtgaatattctgggctagcgtgagatagtttcttgtcctaatatttttcagatc\n"
                            "gaatagcttctatttttgtgtttattgacatatgtcgaaactccttactcagtgaaagtc\n"
                            "atgaccagatccacgaacaatcttcggaatcagtctcgttttacggcggaatcttgagtc\n"
                            "taacttatatcccgtcgcttactttctaacaccccttatgtatttttaaaattacgttta\n"
                            "ttcgaacgtacttggcggaagcgttattttttgaagtaagttacattgggcagactcttg\n"
                            "acattttcgatacgactttctttcatccatcacaggactcgttcgtattgatatcagaag\n"
                            "ctcgtgatgattagttgtcttctttaccaatactttgaggcctattctgcgaaatttttg\n"
                            "ttgccctgcgaacttcacataccaaggaacacctcgcaacatgccttcatatccatcgtt\n"
                            "cattgtaattcttacacaatgaatcctaagtaattacatccctgcgtaaaagatggtagg\n"
                            "ggcactgaggatatattaccaagcatttagttatgagtaatcagcaatgtttcttgtatt\n"
                            "aagttctctaaaatagttacatcgtaatgttatctcgggttccgcgaataaacgagatag\n"
                            "attcattatatatggccctaagcaaaaacctcctcgtattctgttggtaattagaatcac\n"
                            "acaatacgggttgagatattaattatttgtagtacgaagagatataaaaagatgaacaat\n"
                            "tactcaagtcaagatgtatacgggatttataataaaaatcgggtagagatctgctttgca\n"
                            "attcagacgtgccactaaatcgtaatatgtcgcgttacatcagaaagggtaactattatt\n"
                            "aattaataaagggcttaatcactacatattagatcttatccgatagtcttatctattcgt\n"
                            "tgtatttttaagcggttctaattcagtcattatatcagtgctccgagttctttattattg\n"
                            "ttttaaggatgacaaaatgcctcttgttataacgctgggagaagcagactaagagtcgga\n"
                            "gcagttggtagaatgaggctgcaaaagacggtctcgacgaatggacagactttactaaac\n"
                            "caatgaaagacagaagtagagcaaagtctgaagtggtatcagcttaattatgacaaccct\n"
                            "taatacttccctttcgccgaatactggcgtggaaaggttttaaaagtcgaagtagttaga\n"
                            "ggcatctctcgctcataaataggtagactactcgcaatccaatgtgactatgtaatactg\n"
                            "ggaacatcagtccgcgatgcagcgtgtttatcaaccgtccccactcgcctggggagacat\n"
                            "gagaccacccccgtggggattattagtccgcagtaatcgactcttgacaatccttttcga\n"
                            "ttatgtcatagcaatttacgacagttcagcgaagtgactactcggcgaaatggtattact\n"
                            "aaagcattcgaacccacatgaatgtgattcttggcaatttctaatccactaaagcttttc\n"
                            "cgttgaatctggttgtagatatttatataagttcactaattaagatcacggtagtatatt\n"
                            "gatagtgatgtctttgcaagaggttggccgaggaatttacggattctctattgatacaat\n"
                            "ttgtctggcttataactcttaaggctgaaccaggcgtttttagacgacttgatcagctgt\n"
                            "tagaatggtttggactccctctttcatgtcagtaacatttcagccgttattgttacgata\n"
                            "tgcttgaacaatattgatctaccacacacccatagtatattttataggtcatgctgttac\n"
                            "ctacgagcatggtattccacttcccattcaatgagtattcaacatcactagcctcagaga\n"
                            "tgatgacccacctctaataacgtcacgttgcggccatgtgaaacctgaacttgagtagac\n"
                            "gatatcaagcgctttaaattgcatataacatttgagggtaaagctaagcggatgctttat\n"
                            "ataatcaatactcaataataagatttgattgcattttagagttatgacacgacatagttc\n"
                            "actaacgagttactattcccagatctagactgaagtactgatcgagacgatccttacgtc\n"
                            "gatgatcgttagttatcgacttaggtcgggtctctagcggtattggtacttaaccggaca\n"
                            "ctatactaataacccatgatcaaagcataacagaatacagacgataatttcgccaacata\n"
                            "tatgtacagaccccaagcatgagaagctcattgaaagctatcattgaagtcccgctcaca\n"
                            "atgtgtcttttccagacggtttaactggttcccgggagtcctggagtttcgacttacata\n"
                            "aatggaaacaatgtattttgctaatttatctatagcgtcatttggaccaatacagaatat\n"
                            "tatgttgcctagtaatccactataacccgcaagtgctgatagaaaatttttagacgattt\n"
                            "ataaatgccccaagtatccctcccgtgaatcctccgttatactaattagtattcgttcat\n"
                            "acgtataccgcgcatatatgaacatttggcgataaggcgcgtgaattgttacgtgacaga\n"
                            "gatagcagtttcttgtgatatggttaacagacgtacatgaagggaaactttatatctata\n"
                            "gtgatgcttccgtagaaataccgccactggtctgccaatgatgaagtatgtagctttagg\n"
                            "tttgtactatgaggctttcgtttgtttgcagagtataacagttgcgagtgaaaaaccgac\n"
                            "gaatttatactaatacgctttcactattggctacaaaatagggaagagtttcaatcatga\n"
                            "gagggagtatatggatgctttgtagctaaaggtagaacgtatgtatatgctgccgttcat\n"
                            "tcttgaaagatacataagcgataagttacgacaattataagcaacatccctaccttcgta\n"
                            "acgatttcactgttactgcgcttgaaatacactatggggctattggcggagagaagcaga\n"
                            "tcgcgccgagcatatacgagacctataatgttgatgatagagaaggcgtctgaattgata\n"
                            "catcgaagtacactttctttcgtagtatctctcgtcctctttctatctccggacacaaga\n"
                            "attaagttatatatatagagtcttaccaatcatgttgaatcctgattctcagagttcttt\n"
                            "ggcgggccttgtgatgactgagaaacaatgcaatattgctccaaatttcctaagcaaatt\n"
                            "ctcggttatgttatgttatcagcaaagcgttacgttatgttatttaaatctggaatgacg\n"
                            "gagcgaagttcttatgtcggtgtgggaataattcttttgaagacagcactccttaaataa\n"
                            "tatcgctccgtgtttgtatttatcgaatgggtctgtaaccttgcacaagcaaatcggtgg\n"
                            "tgtatatatcggataacaattaatacgatgttcatagtgacagtatactgatcgagtcct\n"
                            "ctaaagtcaattacctcacttaacaatctcattgatgttgtgtcattcccggtatcgccc\n"
                            "gtagtatgtgctctgattgaccgagtgtgaaccaaggaacatctactaatgcctttgtta\n"
                            "ggtaagatctctctgaattccttcgtgccaacttaaaacattatcaaaatttcttctact\n"
                            "tggattaactacttttacgagcatggcaaattcccctgtggaagacggttcattattatc\n"
                            "ggaaaccttatagaaattgcgtgttgactgaaattagatttttattgtaagagttgcatc\n"
                            "tttgcgattcctctggtctagcttccaatgaacagtcctcccttctattcgacatcgggt\n"
                            "ccttcgtacatgtctttgcgatgtaataattaggttcggagtgtggccttaatgggtgca\n"
                            "actaggaatacaacgcaaatttgctgacatgatagcaaatcggtatgccggcaccaaaac\n"
                            "gtgctccttgcttagcttgtgaatgagactcagtagttaaataaatccatatctgcaatc\n"
                            "gattccacaggtattgtccactatctttgaactactctaagagatacaagcttagctgag\n"
                            "accgaggtgtatatgactacgctgatatctgtaaggtaccaatgcaggcaaagtatgcga\n"
                            "gaagctaataccggctgtttccagctttataagattaaaatttggctgtcctggcggcct\n"
                            "cagaattgttctatcgtaatcagttggttcattaattagctaagtacgaggtacaactta\n"
                            "tctgtcccagaacagctccacaagtttttttacagccgaaacccctgtgtgaatcttaat\n"
                            "atccaagcgcgttatctgattagagtttacaactcagtattttatcagtacgttttgttt\n"
                            "ccaacattacccggtatgacaaaatgacgccacgtgtcgaataatggtctgaccaatgta\n"
                            "ggaagtgaaaagataaatat";
#endif
#if _TEST_result == _TEST_knucleotide
static int knucleotide_hook(wasm_module_inst_t *instance, wasm_val_t *args)
{
    uint32_t buff = wasm_runtime_module_dup_data(*instance, input, sizeof input);
    if (!buff)
    {
        return 1;
    }
    args[0].of.i32 = buff;
    args[1].of.i32 = sizeof input;
    return 0;
}
static bench_result run_knucleotide(bench_args args)
{
    uint8_t *BENCH = knucleotide;
    const size_t SIZE = sizeof knucleotide;
    wasm_val_t results[1];
    wasm_val_t funargs[2];
    funargs[0].kind = WASM_I32;
    funargs[1].kind = WASM_I32;

    int err = run_bench(BENCH, SIZE, funargs, 2, results, 1, knucleotide_hook, 1 << 16) | results[0].of.i32;
    if (!err)
    {
        printf("knucleotide result: %ld\n", results[0].of.i32);
    }
    return err;
}
#endif
#if _TEST_result == _TEST_reverse_complement
static int rc_hook(wasm_module_inst_t *instance, wasm_val_t *args)
{
    uint32_t buff = wasm_runtime_module_dup_data(*instance, input, sizeof input);
    if (!buff)
    {
        return 1;
    }
    args[0].of.i32 = buff;
    return 0;
}
static bench_result run_reverse_complement(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    wasm_val_t results[1];
    wasm_val_t funargs[1];
    funargs[0].kind = WASM_I32;

    int err = run_bench(BENCH, SIZE, funargs, 1, results, 1, rc_hook, 1 << 16) | results[0].of.i32;
    if (!err)
    {
        printf("reverse_complement result: %ld\n", results[0].of.i32);
    }
    return err;
}
#endif
#endif // embench
bench_result run_active_bench(bench_args args)
{
#define FUN(B) (run_##B(args))
    bench_result res = expander(BENCHMARK, FUN);
#undef FUN
    return res;
}

#undef expander