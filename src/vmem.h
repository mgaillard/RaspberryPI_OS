#ifndef VMEM_H
#define VMEM_H

#include <inttypes.h>
#include "sched.h"

#define PAGE_SIZE 4096
#define FIRST_LVL_TT_COUNT 4096
#define FIRST_LVL_TT_SIZE 16384
#define SECOND_LVL_TT_COUNT 256
#define SECOND_LVL_TT_SIZE 1024
#define PAGE_OCCUPANCY_TT_SIZE 1048576

void vmem_init();
unsigned int init_kern_translation_table();
uint8_t* vmem_alloc_for_userland(struct pcb_s* process);
uint32_t vmem_translate(uint32_t va, struct pcb_s* process);

#endif
