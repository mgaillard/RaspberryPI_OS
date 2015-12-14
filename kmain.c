#include <inttypes.h>
#include "util.h"
#include "syscall.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"


int user_process1()
{
    uint64_t i = 0;
    for(;;)
    {
        i++;
        led_off();
    }
    return EXIT_SUCCESS;
}

int user_process2()
{
    uint64_t j = 0;
    for(;;)
    {
        j++;
        led_on();
    }
    return EXIT_SUCCESS;
}

int kmain( void )
{
    int status;
    struct pcb_s* process1;
    struct pcb_s* process2;
    
    sched_init();

    process1 = create_process((func_t*)&user_process1 , 15);
    process2 = create_process((func_t*)&user_process2 , 18);	
	
	// ******************************************
	// switch CPU to USER mode
	// ******************************************
	ENABLE_IRQ();
    __asm("cps 0x10");
    
    // USER Program
    // ******************************************
	
	//On attend la terminaison de notre processus.
    status = sys_wait(process1);
    status = sys_wait(process2);
    
    status++;
	
	return 0;
}
