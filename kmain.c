#include <inttypes.h>
#include "util.h"
#include "syscall.h"
#include "kheap.h"
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

int process_fork()
{
    int status;
    struct pcb_s* child_process = sys_fork();

    if (child_process == 0)
    {
        //On est dans le processus fils.
        return EXIT_SUCCESS;
    }
    else
    {
        //On est dans le processus pere.
        status = sys_wait(child_process);
    }

    return status;
}

void game_of_life(const int left, const int top, int width, int height, const int cell_size)
{
    //Contient l'état des pixels affichés sur l'écran.
    int* screen;
    //Un tampon contenant ce qui sera affiché sur l'écran à la prochaine itération.
    int* buffer;

    //On adapte la taille en fonction de la taille des cellules.
    width = divide(width, cell_size);
    height = divide(height, cell_size);

    //On alloue les tableaux contenant les cellules.
    screen = (int*)sys_malloc(height * width * sizeof(int));
    buffer = (int*)sys_malloc(height * width * sizeof(int));

    //On initialise les tableaux contenant les cellules.
    for (int i = 0;i < height * width;i++)
    {
        screen[i] = (i % 3);
        buffer[i] = 0;
    }

    //On affiche l'écran une première fois.
    for (int i = 0;i < height;i++)
    {
        for (int j = 0;j < width;j++)
        {
            //On calcule la couleur. Blanc si la cellule est vivante, noir sinon.
            int grey = 0;
            if (screen[i*width + j] == 1)
            {
                grey = 255;
            }
            //On dessine le carre.
            drawRect(left + j*cell_size, top + i*cell_size, cell_size, cell_size, grey, grey, grey);
        }
    }

    //On fait plusieurs tours du jeu de la vie.
    for (int tour = 0;tour < 500;tour++)
    {
        //On calcule le buffer.
        for (int i = 0;i < height;i++)
        {
            for (int j = 0;j < width;j++)
            {
                //On compte le nombre de voisins de la cellule.
                int living_cell_nb = 0;                
                for(int di = -1;di <= 1;di++)
                {
                    for (int dj = -1;dj <= 1;dj++)
                    {
                        //La ligne de la cellule adjacente.
                        int ic = i + di;
                        //La colonne de la cellule adjacente.
                        int jc = j + dj;
                        //On teste si cette cellule existe.
                        if (ic >= 0 && ic < height && jc >= 0 && jc < width && (di != 0 || dj != 0))
                        {
                            if (screen[ic*width + jc] == 1)
                            {
                                living_cell_nb++;
                            }
                        }
                    }
                }
                //On calcule le nouvel état de la cellule.
                buffer[i*width + j] = (living_cell_nb == 3) || (screen[i*width + j] == 1 && living_cell_nb == 2);
            }
        }

        //On affiche le buffer.
        for (int i = 0;i < height;i++)
        {
            for (int j = 0;j < width;j++)
            {
                //On affiche si le buffer a changé par rapport à l'écran.
                if (buffer[i*width + j] != screen[i*width + j])
                {
                    //On calcule la couleur. Blanc si la cellule est vivante, noir sinon.
                    int grey = 0;
                    if (buffer[i*width + j] == 1)
                    {
                        grey = 255;
                    }
                    //On dessine le carre.
                    drawRect(left + j*cell_size, top + i*cell_size, cell_size, cell_size, grey, grey, grey);
                    //On met à jour le buffer de l'écran.
                    screen[i*width + j] = buffer[i*width + j];
                }
            }
        }
    }

    //On libère la mémoire.
    sys_free(screen);
    sys_free(buffer);
}

int process_screen_right()
{
    uint32_t width = getWidthFB() / 2;
    uint32_t height = getHeightFB();

    game_of_life(width + 5, 0, width - 5, height, 16);

    return EXIT_SUCCESS;
}

int process_screen_top_left()
{
    int status;
    uint32_t width = getWidthFB() / 4;
    uint32_t height = getHeightFB() / 2;
    struct pcb_s* child_process;

    child_process = sys_fork();

    if (child_process == 0)
    {
        //On est dans le processus fils.
        game_of_life(0, 0, width - 5, height - 5, 8);

        return EXIT_SUCCESS;
    }
    else
    {
        //On est dans le processus pere.
        game_of_life(width + 5, 0, width - 5, height - 5, 8);

        status = sys_wait(child_process);
    }

    return status;
}

int process_screen_bottom_left()
{
    uint32_t width = getWidthFB() / 2;
    uint32_t height = getHeightFB() / 2;

    game_of_life(0, height + 5, width - 5, height - 5, 16);

    return EXIT_SUCCESS;
}

int kmain( void )
{
    struct pcb_s* process_screen_right_pcb;
    struct pcb_s* process_screen_top_left_pcb;
    struct pcb_s* process_screen_bottom_left_pcb;
    
    hw_init();
    log_str("Starting kernel...\n");
    FramebufferInitialize();
    kheap_init();
    sched_init();

    process_screen_right_pcb = sys_create_process((func_t*)&process_screen_right, 18);
    process_screen_top_left_pcb = sys_create_process((func_t*)&process_screen_top_left, 20);
    process_screen_bottom_left_pcb = sys_create_process((func_t*)&process_screen_bottom_left, 20);
	
	// ******************************************
	// switch CPU to USER mode
	// ******************************************
	ENABLE_IRQ();
    __asm("cps 0x10");
    
    // USER Program
    // ******************************************
	
	//On attend la terminaison de notre processus.
	sys_wait(process_screen_right_pcb);
    sys_wait(process_screen_top_left_pcb);
    sys_wait(process_screen_bottom_left_pcb);
	
	return 0;
}
