#ifndef INIT_H
#define INIT_H
#include <stddef.h>
#include <stdint.h>
int init(void);

void trace_buffer(const uint8_t *buffer, size_t len);

#endif