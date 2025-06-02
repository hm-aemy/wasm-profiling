MEMORY
{
    rom (rx) : ORIGIN = 0x08000000, LENGTH = 2M
    ram (rwx) : ORIGIN = 0x20000000, LENGTH = 0x80000
}

INCLUDE ../../../libopencm3/lib/cortex-m-generic.ld