#include <inttypes.h>
#include "util.h"
#include "syscall.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "vmem.h"
#include "fb.h"

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

int process_screen()
{
	for (;;)
	{
		drawBlue();
		drawRed();
	}
	return EXIT_SUCCESS;
}

int kmain( void )
{
    struct pcb_s* process_led_on_pcb;
    struct pcb_s* process_led_off_pcb;
    struct pcb_s* process_screen_pcb;
    
    FramebufferInitialize();	
    sched_init();
	
    process_led_on_pcb = sys_create_process((func_t*)&process_led_on, 20);
    process_led_off_pcb = sys_create_process((func_t*)&process_led_off, 20);
    process_screen_pcb = sys_create_process((func_t*)&process_screen, 10);
	
	// ******************************************
	// switch CPU to USER mode
	// ******************************************
	ENABLE_IRQ();
    __asm("cps 0x10");
    
    // USER Program
    // ******************************************
	
	//On attend la terminaison de notre processus.
	sys_wait(process_screen_pcb);
    sys_wait(process_led_on_pcb);
    sys_wait(process_led_off_pcb);
	
	return 0;
}
