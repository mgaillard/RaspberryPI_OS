#include <inttypes.h>
#include "util.h"
#include "syscall.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "vmem.h"

int process_dynamic_alloc()
{
    const int TAILLE = 10;
    int* tableau = (int*)sys_malloc(TAILLE * sizeof(int));

    //Si un problème d'allocation est survenu.
    if (tableau != NULL)
    {
        for (int i = 0;i < TAILLE;i++)
        {
            tableau[i] = i;
        }

        sys_free(tableau);

        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}

int process_led_on()
{
	for (;;)
	{
		led_on();
	}
	return EXIT_SUCCESS;
}

int process_led_off()
{
	for (;;)
	{
		led_off();
	}
	return EXIT_SUCCESS;
}

int kmain( void )
{
    struct pcb_s* process_alloc;
    struct pcb_s* process_on;
    struct pcb_s* process_off;
    
    sched_init();

    process_alloc = sys_create_process((func_t*)&process_dynamic_alloc, 20);
    process_on = sys_create_process((func_t*)&process_led_on, 10);
    process_off = sys_create_process((func_t*)&process_led_off, 10);
	
	// ******************************************
	// switch CPU to USER mode
	// ******************************************
	ENABLE_IRQ();
    __asm("cps 0x10");
    
    // USER Program
    // ******************************************
	
	//On attend la terminaison de notre processus.
    sys_wait(process_alloc);
    sys_wait(process_on);
    sys_wait(process_off);
	
	return 0;
}
