MEMORY
{
    rom (rx) : ORIGIN = 0x08000000, LENGTH = 2048K
    ram (rwx) : ORIGIN = 0x20000000, LENGTH = 0x30000
    ccmram (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

INCLUDE ../../../libopencm3/lib/cortex-m-generic.ld