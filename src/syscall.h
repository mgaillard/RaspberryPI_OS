#ifndef SYSCALL_H
#define SYSCALL_H

#include <inttypes.h>
#include "sched.h"

/*************** Functions declaration mode User *****************/
void sys_reboot();
void sys_nop();
void sys_settime(uint64_t date_ms);
uint64_t sys_gettime();
void sys_yieldto(struct pcb_s* dest);
void sys_yield();
void sys_exit(int status);
int sys_wait(struct pcb_s* dest);
struct pcb_s* sys_create_process(func_t* entry, int32_t niceness);
ProcessState sys_process_state(struct pcb_s* process);
int sys_process_return_code(struct pcb_s* process);
void* sys_malloc(uint32_t size);
void sys_free(void* address);

#endif
