#include <inttypes.h>
#include "util.h"
#include "syscall.h"
#include "sched.h"
#include "hw.h"
#include "asm_tools.h"
#include "vmem.h"

int user_process()
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
