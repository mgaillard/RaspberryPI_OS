#include <inttypes.h>
#include "util.h"
#include "syscall.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "vmem.h"

int user_process()
{
    int i = 0;
    while(i < 10)
    {
		i++;
    }
    return EXIT_SUCCESS;
}

int kmain( void )
{
    int status;
    struct pcb_s* process;
    
    sched_init();

    process = sys_create_process((func_t*)&user_process);
	
	// ******************************************
	// switch CPU to USER mode
	// ******************************************
	ENABLE_IRQ();
    __asm("cps 0x10");
    
    // USER Program
    // ******************************************
	
	//On attend la terminaison de notre processus.
    status = sys_wait(process);
    
    status++;
	
	return 0;
}
