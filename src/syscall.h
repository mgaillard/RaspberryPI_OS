#ifndef SYSCALL_H
#define SYSCALL_H

#include <inttypes.h>
#include "sched.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/*************** Functions declaration mode User *****************/
void sys_reboot();
void sys_nop();
void sys_settime(uint64_t date_ms);
uint64_t sys_gettime();
void sys_yieldto(struct pcb_s* dest);
void sys_yield();
void sys_exit(int status);
int sys_wait(struct pcb_s* dest);

/*************** Functions declaration mode Kernel *****************/
void swi_handler();
void do_sys_reboot();
void do_sys_nop();
void do_sys_settime(int* pile);
void do_sys_gettime(int* pile);
void do_sys_yieldto(int* pile);
void do_sys_yield(int* pile);
void do_sys_exit(int* pile);
void do_sys_free_process(int* pile);

#endif
