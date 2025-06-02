#include "benchmarks.h"
#include "benchmarks-defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "wasm3.h"
#include "m3_api_wasi.h"

typedef int (*module_hook)(IM3Runtime module, void *args[]);

#define expander(B, m) m(B)
#define _TEST_knucleotide 1
#define _TEST_reverse_complement 2
#define _test(benchname) _TEST_##benchname
#define _TEST_result expander(BENCHMARK, _test)

#define FATAL(msg, ...)                            \
    {                                              \
        printf("Fatal: " msg "\n", ##__VA_ARGS__); \
        return 1;                                  \
    }
uint32_t register_wasi();
static int32_t start_msecs = 0;
static int32_t stop_msecs = 0;
static m3ApiRawFunction(get_time_wrapper)
{
    m3ApiReturnType(uint32_t)
        uint32_t res = stop_msecs - start_msecs;
    m3ApiReturn(res);
}
static m3ApiRawFunction(stop_time_wrapper)
{
    stop_msecs = (uint64_t)(clock() * 1000) / (CLOCKS_PER_SEC);
    m3ApiSuccess();
}
static m3ApiRawFunction(start_time_wrapper)
{
    start_msecs = (uint64_t)(clock() * 1000) / (CLOCKS_PER_SEC);
    m3ApiSuccess();
}
static m3ApiRawFunction(get_milsecs_wrapper)
{
    m3ApiReturnType(uint32_t)
        uint32_t result = (uint64_t)(clock() * 1000) / (CLOCKS_PER_SEC);
    m3ApiReturn(result);
}
int32_t run_bench(uint8_t *mod, size_t mod_size, void *args[], size_t args_len,
                  void *results[], size_t results_len, module_hook hook)
{
    M3Result result = m3Err_none;

    uint8_t *wasm = mod;
    uint32_t fsize = mod_size;

    clock_t start = clock();
    __sync_synchronize();
    printf("Loading WebAssembly...\n");
    IM3Environment env = m3_NewEnvironment();
    if (!env)
        FATAL("m3_NewEnvironment failed");

    IM3Runtime runtime = m3_NewRuntime(env, 1 << 13, NULL);
    if (!runtime)
        FATAL("m3_NewRuntime failed");

    IM3Module module;
    result = m3_ParseModule(env, &module, wasm, fsize);
    if (result)
        FATAL("m3_ParseModule: %s", result);

    result = m3_LoadModule(runtime, module);
    if (result)
        FATAL("m3_LoadModule: %s", result);
    result = m3_LinkWASI(module);
    if (result)
    {
        FATAL("link WASI: %s\n", result);
    }
    (m3_LinkRawFunction(module, "env", "start_time", "()", &start_time_wrapper));
    (m3_LinkRawFunction(module, "env", "stop_time", "()", &stop_time_wrapper));
    (m3_LinkRawFunction(module, "env", "get_time", "i()", &get_time_wrapper));
    (m3_LinkRawFunction(module, "env", "get_milsecs", "i()", &get_milsecs_wrapper));
    if (hook && hook(runtime, args))
    {
        FATAL("hook error");
    }
    IM3Function f;
    result = m3_FindFunction(&f, runtime, "_initialize");
    if (result || m3_CallV(f))
        FATAL("_initialize: %s", result);

    result = m3_FindFunction(&f, runtime, "_run");

    if (result || m3_Call(f, args_len, args))
    {
        FATAL("_start: %s", result);
    }

    result = m3_GetResults(f, results_len, results);
    if (result)
        FATAL("m3_GetResults: %s", result);

    __sync_synchronize();
    clock_t end = clock();
    printf("First runtime delay: %ldms\n", (end - start) * 1000 / (CLOCKS_PER_SEC));
#ifndef HEAP_TRACE
    start = clock();
    __sync_synchronize();
    if (m3_Call(f, args_len, args))
    {
        FATAL("_start second run: %s", result);
    }

    result = m3_GetResults(f, results_len, results);
    if (result)
        FATAL("m3_GetResults: %s", result);
    __sync_synchronize();
    end = clock();
    printf("Second runtime delay: %ldms\n", (end - start) * 1000 / (CLOCKS_PER_SEC));
#endif
    m3_FreeRuntime(runtime);
    m3_FreeEnvironment(env);
    return 0;
}
#ifdef EMBENCH
#define FUN_NAME_GEN(b) run_##b
#define FUN_NAME expander(BENCHMARK, FUN_NAME_GEN)
__attribute__((weak)) bench_result FUN_NAME(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    int32_t result;
    void *results[1] = {&result};
    int err = run_bench(BENCH, SIZE, NULL, 0, results, 1, NULL);
    if (!err)
    {
        printf("BENCHMARK"
               " result: %ld\n",
               result);
    }
    return err;
}
#undef FUN_NAME
#else
static bench_result run_coremark(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    int32_t result;
    void *results[1] = {&result};
    int err = run_bench(BENCH, SIZE, NULL, 0, results, 1, NULL);
    if (!err)
    {
        printf("coremark result: %ld\n", result);
    }
    return err;
}
static bench_result run_dhrystone_standalone(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    int32_t result;
    void *results[1] = {&result};
    int32_t argument = 100000;
    void *arguments[1] = {&argument};
    int err = run_bench(BENCH, SIZE, arguments, 1, results, 1, NULL);
    if (!err)
    {
        printf("dhrystone result: %ld\n", result);
    }
    return err;
}
static bench_result run_fannkuch_redux(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    int32_t result;
    void *results[1] = {&result};
    int32_t argument = 8;
    void *arguments[1] = {&argument};
    int err = run_bench(BENCH, SIZE, arguments, 1, results, 1, NULL);
    if (!err)
    {
        printf("fannkuch-redux result: %ld\n", result);
    }
    return err;
}
static bench_result run_binary_trees(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    int32_t result;
    void *results[1] = {&result};
    int32_t argument = 9;
    void *arguments[1] = {&argument};
    int err = run_bench(BENCH, SIZE, arguments, 1, results, 1, NULL);
    if (!err)
    {
        printf("binary trees result: %ld\n", result);
    }
    return err;
}
static bench_result run_nbody(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    int32_t result;
    void *results[1] = {&result};
    int32_t argument = 5000;
    void *arguments[1] = {&argument};
    int err = run_bench(BENCH, SIZE, arguments, 1, results, 1, NULL);
    if (!err)
    {
        printf("nbody result: %ld\n", result);
    }
    return err;
}
static bench_result run_spectral_norm(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    int32_t result;
    void *results[1] = {&result};
    int32_t argument = 100;
    void *arguments[1] = {&argument};
    int err = run_bench(BENCH, SIZE, arguments, 1, results, 1, NULL);
    if (!err)
    {
        printf("spectral norm result: %ld\n", result);
    }
    return err;
}

static bench_result run_fasta(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    int32_t result;
    void *results[1] = {&result};
    int32_t argument = 10000;
    void *arguments[1] = {&argument};
    int err = run_bench(BENCH, SIZE, arguments, 1, results, 1, NULL);
    if (!err)
    {
        printf("fasta result: %ld\n", result);
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
static int knucleotide_hook(IM3Runtime runtime, void *args[])
{
    IM3Function f;
    M3Result err = m3_FindFunction(&f, runtime, "malloc");
    if (err)
    {
        FATAL("knucleotide hook find malloc: %s\n", err);
    }
    uint32_t mem_offset;
    err = m3_CallV(f, sizeof input);
    if (err)
    {
        FATAL("knucleotide hook call malloc: %s\n", err);
    }
    uint8_t *mem = m3_GetMemory(runtime, NULL, 0);
    if (!mem)
    {
        FATAL("could not get memory.");
    }
    mem += mem_offset;
    for (int i = 0; i < sizeof input; i++)
    {
        mem[i] = input[i];
    }
    *(int32_t)args[0] = mem_offset;
    return 0;
}
static bench_result run_knucleotide(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    int32_t result;
    void *results[1] = {&result};
    int32_t argument;
    void *arguments[1] = {&argument};
    int err = run_bench(BENCH, SIZE, arguments, 1, results, 1, knucleotide_hook);
    if (!err)
    {
        printf("knucleotide result: %ld\n", result);
    }
    return err;
}
#endif
#if _TEST_result == _TEST_reverse_complement
static int rc_hook(IM3Runtime runtime, void *args[])
{
    IM3Function f;
    M3Result err = m3_FindFunction(&f, runtime, "malloc");
    if (err)
    {
        FATAL("knucleotide hook find malloc: %s\n", err);
    }
    uint32_t mem_offset;
    err = m3_CallV(f, sizeof input);
    if (err)
    {
        FATAL("knucleotide hook call malloc: %s\n", err);
    }
    uint8_t *mem = m3_GetMemory(runtime, NULL, 0);
    if (!mem)
    {
        FATAL("could not get memory.");
    }
    mem += mem_offset;
    for (int i = 0; i < sizeof input; i++)
    {
        mem[i] = input[i];
    }
    *(int32_t)args[0] = mem_offset;
    return 0;
}
static bench_result run_reverse_complement(bench_args args)
{
    uint8_t *BENCH = BENCHMARK;
    const size_t SIZE = sizeof BENCHMARK;
    int32_t result;
    void *results[1] = {&result};
    int32_t argument;
    void *arguments[1] = {&argument};
    int err = run_bench(BENCH, SIZE, arguments, 1, results, 1, rc_hook);
    if (!err)
    {
        printf("reverse-complement result: %ld\n", result);
    }
    return err;
}
#endif
#endif // EMBENCH
bench_result run_active_bench(bench_args args)
{
#define FUN(B) (run_##B(args))
    bench_result res = expander(BENCHMARK, FUN);
#undef FUN
    return res;
}

#undef expander