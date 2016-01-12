#include "vmem.h"
#include "kheap.h"
#include "asm_tools.h"
#include "hw.h"
#include "page_table.h"
#include "util.h"
#include "syscall.h"
#include "fb.h"

//La table des pages du noyau.
uint32_t* mmu_table_base;

void start_mmu_C()
{
	register uint32_t control;
	__asm("mcr p15, 0, %[zero], c1, c0, 0" : : [zero] "r"(0)); // Disable cache 
	__asm("mcr p15, 0, r0, c7, c7, 0 "); // Invalidate cache ( data and instructions )
	__asm("mcr p15, 0, r0, c8, c7, 0 "); // Invalidate TLB entries 
	
	/* Enable ARMv6 MMU features ( disable sub - page AP ) */
	control = (1<<23) | (1 << 15) | (1 << 4) | 1;
	
	/* Invalidate the translation lookaside buffer ( TLB ) */
	__asm volatile("mcr p15, 0, %[data], c8, c7, 0" : : [data] "r" (0));
	
	/* Write control register */
	__asm volatile("mcr p15, 0, %[control], c1, c0, 0" : : [control] "r" (control));
}

void configure_mmu_C()
{ 
	register uint32_t* pt_addr = mmu_table_base;
	
	/* Translation table 0 */
	__asm volatile("mcr p15, 0, %[addr], c2, c0, 0" : : [addr] "r"(pt_addr));
	
	/* Translation table 1 */
	__asm volatile("mcr p15, 0, %[addr], c2, c0, 1" : : [addr] "r"(pt_addr));
	
	/* Use translation table 0 for everything */
	__asm volatile("mcr p15, 0, %[n], c2, c0, 2" : : [n] "r"(0));
	
	/* Set Domain 0 ACL to " Manager ", not enforcing memory permissions
	 * Every mapped section/page is in domain 0 */
	__asm volatile("mcr p15, 0, %[r], c3, c0, 0" : : [r] "r" (0x3));
}

/*
 * Cette fonction initialise la table des pages de sorte à ce que les adresse
 * virtuelle située entre 0x0 et __kernel_heap_end__ renvoient vers les mêmes
 * adresses physique.
 * Dans cet intervalle, il y a 16 777 215 adresses. Une page contient 4096
 * adresses. On doit initialiser 4096 pages réparties dans 16 tables de niveau 2
 * et une table de niveau 1.
 * Le même comportement est fait pour l'espace mémoire des devices. entre les
 * adresses DEVICE_SPACE_START et DEVICE_SPACE_END.
 */
uint32_t* init_kern_translation_table()
{
	uint32_t* page_table;

	const uint32_t SECOND_LVL_TT_KERNEL_NB = ((uint32_t)&__kernel_heap_end__ + 1) / (SECOND_LVL_TT_COUNT*PAGE_SIZE);
	const uint32_t SECOND_LVL_TT_DEVICES_START = DEVICE_SPACE_START / (SECOND_LVL_TT_COUNT*PAGE_SIZE);
	const uint32_t SECOND_LVL_TT_DEVICES_END = (DEVICE_SPACE_END + 1) / (SECOND_LVL_TT_COUNT*PAGE_SIZE);
	const uint32_t FRAMEBUFFER_FIRST_FRAME = getAddressFB() / PAGE_SIZE;
	const uint32_t FRAMEBUFFER_LAST_FRAME = (getAddressFB() + getSizeFB()) / PAGE_SIZE;
	const uint32_t SECOND_LEVEL_FLAGS = 0x52;
	const uint32_t SECOND_LEVEL_DEVICE_FLAGS = 0x17;

	//On crée la table des pages du noyau.
	page_table = create_page_table();

	//On fait correspondre les adresses logiques du noyau aux adresses physiques pour la mémoire du noyau.
	for (uint32_t first_level_index = 0;first_level_index < SECOND_LVL_TT_KERNEL_NB;first_level_index++)
	{
		//On initialise les entrées de cette table.
		for (int second_level_index = 0;second_level_index < SECOND_LVL_TT_COUNT;second_level_index++)
		{
			//On calcule le debut de la frame en fonction des index de niveau 1 et de niveau 2 pour avoir les mêmes adresses logiques et physiques.
			uint32_t frame_address = first_level_index*SECOND_LVL_TT_COUNT*PAGE_SIZE + second_level_index*PAGE_SIZE;
			//On ajoute cette frame à la table des pages.
			add_entry_page_table(page_table, first_level_index, second_level_index, frame_address, SECOND_LEVEL_FLAGS);
		}
	}
	
	//On fait correspondre les adresses logiques et physiques du framebuffer de l'écran.
	for (uint32_t frame = FRAMEBUFFER_FIRST_FRAME;frame <= FRAMEBUFFER_LAST_FRAME;frame++)
	{
		uint32_t first_level_index = frame / SECOND_LVL_TT_COUNT;
        uint32_t second_level_index = frame - first_level_index * SECOND_LVL_TT_COUNT;
        uint32_t frame_address = frame*PAGE_SIZE;
        //On ajoute cette frame à la table des pages.
		add_entry_page_table(page_table, first_level_index, second_level_index, frame_address, SECOND_LEVEL_FLAGS);
	}

	//On fait correspondre les adresses logiques du noyau aux adresses physiques pour la partie mémoire des devices.
	for (uint32_t first_level_index = SECOND_LVL_TT_DEVICES_START;first_level_index < SECOND_LVL_TT_DEVICES_END;first_level_index++)
	{
		//On initialise les entrées de cette table.
		for (int second_level_index = 0;second_level_index < SECOND_LVL_TT_COUNT;second_level_index++)
		{
			//On calcule le debut de la frame en fonction des index de niveau 1 et de niveau 2 pour avoir les mêmes adresses logiques et physiques.
			uint32_t frame_address = first_level_index*SECOND_LVL_TT_COUNT*PAGE_SIZE + second_level_index*PAGE_SIZE;
			//On ajoute cette frame à la table des pages.
			add_entry_page_table(page_table, first_level_index, second_level_index, frame_address, SECOND_LEVEL_DEVICE_FLAGS);
		}
	}

	return page_table;
}

void vmem_init()
{
	init_frame_occupancy_table(FRAME_OCCUPANCY_TT_SIZE);
	mmu_table_base = init_kern_translation_table();
	configure_mmu_C();	
	start_mmu_C();
	timer_init();
	ENABLE_AB();
}

/**
 * Initialise une table de niveau 1 pour un processus.
 * Celle-ci correspond a la table du noyau pour les adresses de l'espace noyau et les adresses des devices.
 */
uint32_t* init_process_translation_table()
{
	//Le nombre de frame pour mapper le noyau jusqu'au debut du tas du noyau.
	const uint32_t KERNEL_FRAME_NB = ((uint32_t)&__kernel_heap_start__ + 1) / PAGE_SIZE;
	const uint32_t SECOND_LVL_TT_DEVICES_START = DEVICE_SPACE_START / (SECOND_LVL_TT_COUNT*PAGE_SIZE);
	const uint32_t SECOND_LVL_TT_DEVICES_END = (DEVICE_SPACE_END + 1) / (SECOND_LVL_TT_COUNT*PAGE_SIZE);
	const uint32_t FRAMEBUFFER_FIRST_FRAME = getAddressFB() / PAGE_SIZE;
	const uint32_t FRAMEBUFFER_LAST_FRAME = (getAddressFB() + getSizeFB()) / PAGE_SIZE;
	const uint32_t SECOND_LEVEL_FLAGS = 0x52;
	const uint32_t SECOND_LEVEL_DEVICE_FLAGS = 0x17;

	//On crée la table des pages du processus.
	uint32_t* page_table = create_page_table();
	
	//On fait correspondre les adresses logiques et physiques du noyau.
	for (int frame = 0;frame < KERNEL_FRAME_NB;frame++)
	{
		//On retrouve pour la frame, les index de niveau 1 et de niveau 2.
		uint32_t first_level_index = frame / SECOND_LVL_TT_COUNT;
		uint32_t second_level_index = frame - first_level_index * SECOND_LVL_TT_COUNT;
		uint32_t frame_address = frame * PAGE_SIZE;

		add_entry_page_table(page_table, first_level_index, second_level_index, frame_address, SECOND_LEVEL_FLAGS);
	}
	
	//On fait correspondre les adresses logiques et physiques du framebuffer de l'écran.
	for (uint32_t frame = FRAMEBUFFER_FIRST_FRAME;frame <= FRAMEBUFFER_LAST_FRAME;frame++)
	{
		uint32_t first_level_index = frame / SECOND_LVL_TT_COUNT;
        uint32_t second_level_index = frame - first_level_index * SECOND_LVL_TT_COUNT;
        uint32_t frame_address = frame*PAGE_SIZE;
        //On ajoute cette frame à la table des pages.
		add_entry_page_table(page_table, first_level_index, second_level_index, frame_address, SECOND_LEVEL_FLAGS);
	}
	
	//On fait correspondre les adresses logiques du noyau aux adresses physiques pour la partie mémoire des devices.
	for (uint32_t first_level_index = SECOND_LVL_TT_DEVICES_START;first_level_index < SECOND_LVL_TT_DEVICES_END;first_level_index++)
	{
		//On initialise les entrées de cette table.
		for (int second_level_index = 0;second_level_index < SECOND_LVL_TT_COUNT;second_level_index++)
		{
			//On calcule le debut de la frame en fonction des index de niveau 1 et de niveau 2 pour avoir les mêmes adresses logiques et physiques.
			uint32_t frame_address = first_level_index*SECOND_LVL_TT_COUNT*PAGE_SIZE + second_level_index*PAGE_SIZE;
			//On ajoute cette frame à la table des pages.
			add_entry_page_table(page_table, first_level_index, second_level_index, frame_address, SECOND_LEVEL_DEVICE_FLAGS);
		}
	}
	
	return page_table;
}

void load_kernel_page_table()
{
	load_page_table(mmu_table_base);
}

void load_page_table(const uint32_t* table)
{
	//On fait pointer la MMU sur la table des pages de ce processus.
	__asm volatile("mov	r0, %0" : : "r"(table));
	__asm volatile("mcr p15, 0, r0, c2, c0, 0");
	__asm volatile("mcr p15, 0, r0, c2, c0, 1");

	INVALIDATE_TLB();
}

uint8_t* vmem_alloc_for_userland(uint32_t* page_table, uint32_t size, uint32_t address, int direction)
{
	const uint32_t SECOND_LEVEL_FLAGS = 0x52;
	//On calcule le nombre de pages nécéssaires.
	uint32_t page_nb = ((size - 1) / PAGE_SIZE) + 1;

	//On recherche une plage de pages libres consécutives.
	uint32_t start_page = address / PAGE_SIZE;
	uint32_t free_pages = find_free_pages_page_table(page_table, page_nb, start_page, direction);
	//Si on a trouvé les pages libres.
	if (free_pages < UINT32_MAX)
	{
		//On alloue les pages avec des frames.
		for (uint32_t page = free_pages;page < free_pages + page_nb;page++)
		{
			//On retrouve pour la page, les index de niveau 1 et de niveau 2.
		    uint32_t first_level_index = page / SECOND_LVL_TT_COUNT;
		    uint32_t second_level_index = page - first_level_index * SECOND_LVL_TT_COUNT;

		    //On trouve une frame libre.
		    uint32_t frame = find_free_frame_occupancy_table();
		    if (frame != UINT32_MAX)
		    {
	    		uint32_t frame_address = frame * PAGE_SIZE;
	    		//On etablit le lien entre page et frame.
				add_entry_page_table(page_table, first_level_index, second_level_index, frame_address, SECOND_LEVEL_FLAGS);
		    }
		    else
		    {
		    	//Il n'y a plus de frame libre.
		    	///TODO: Libérer correctement la mémoire et retourner un pointeur null.
		    	PANIC();
		    }
		}
	}

	//On retourne l'adresse de la page basse.
	return (uint8_t*)(free_pages * PAGE_SIZE);
}

void* memcpy(void* destination, const void* source, uint32_t size)
{
	const uint32_t* copy_source = (const uint32_t*)source;
	uint32_t* copy_destination = (uint32_t*)destination;
	//On copie 4 octets par 4.
	for (uint32_t i = 0;i < size / 4;i++)
	{
		copy_destination[i] = copy_source[i];
	}

	return destination;
}

void vmem_copy_frame(uint32_t destination_frame, uint32_t source_frame)
{
	const uint32_t LAST_KERNEL_PAGE = ((uint32_t)&__kernel_heap_end__ + 1) / PAGE_SIZE;
	const uint32_t SECOND_LEVEL_FLAGS = 0x52;
	uint32_t source_page;
	uint32_t source_first_level_index;
	uint32_t source_second_level_index;
	uint32_t destination_page;
	uint32_t destination_first_level_index;
	uint32_t destination_second_level_index;

	//On ajoute les deux frames à la table des pages du noyau.
	source_page = find_free_pages_page_table(mmu_table_base, 1, LAST_KERNEL_PAGE, UP);
	source_first_level_index = source_page / SECOND_LVL_TT_COUNT;
    source_second_level_index = source_page - source_first_level_index * SECOND_LVL_TT_COUNT;
	add_entry_page_table(mmu_table_base, source_first_level_index, source_second_level_index, source_frame * PAGE_SIZE, SECOND_LEVEL_FLAGS);

	destination_page = find_free_pages_page_table(mmu_table_base, 1, source_page, UP);
	destination_first_level_index = destination_page / SECOND_LVL_TT_COUNT;
    destination_second_level_index = destination_page - destination_first_level_index * SECOND_LVL_TT_COUNT;
	add_entry_page_table(mmu_table_base, destination_first_level_index, destination_second_level_index, destination_frame * PAGE_SIZE, SECOND_LEVEL_FLAGS);
	//On effectue la copie entre les deux pages.
	memcpy((void*)(destination_page * PAGE_SIZE), (void*)(source_page * PAGE_SIZE), PAGE_SIZE);
	//On supprime les deux pages.
	free_page_page_table(mmu_table_base, source_first_level_index, source_second_level_index);
	free_page_page_table(mmu_table_base, destination_first_level_index, destination_second_level_index);
	//On invalide la TLB car deux pages ont été supprimées.
	INVALIDATE_TLB();
}

void vmem_free(uint32_t* page_table, uint8_t* address, uint32_t size)
{
	//On retouve la page de debut en fonction de l'adresse.
	uint32_t first_page = (uint32_t)address / PAGE_SIZE;
	//On calcule le nombre de pages nécéssaires.
	uint32_t page_nb = ((size - 1) / PAGE_SIZE) + 1;
	//On desalloue les pages.
	for (uint32_t page = first_page;page < first_page + page_nb;page++)
	{
		//On retrouve pour la page, les index de niveau 1 et de niveau 2.
	    uint32_t first_level_index = page / SECOND_LVL_TT_COUNT;
	    uint32_t second_level_index = page - first_level_index * SECOND_LVL_TT_COUNT;

	    free_page_page_table(page_table, first_level_index, second_level_index);
	}
}

void __attribute__((naked)) data_handler()
{
	int* pile;
	uint32_t fault_cause;
	uint32_t fault_address;

	DISABLE_IRQ();
	//Sauvegarde du contexte d'execution.
	__asm("stmfd sp!, {r0-r12, lr}");
	//Memorisation du sommet de pile pour lecture des parametres.
	__asm("mov %0, sp" : "=r"(pile));

	//On passe sur la table de traduction du noyau.
	load_kernel_page_table();

	//On lit les informations de l'erreur mémoire.
	__asm("mrc p15, 0, %0, c5, c0, 0" : "=r"(fault_cause));
	__asm("mrc p15, 0, %0, c6, c0, 0" : "=r"(fault_address));

	//On passe en mode SVC pour le changement de contexte.
	__asm("cps 0x13");
	//On quitte le processus courant.
	exit_process(pile);
	//On reviens au précédent mode d'exécution.
	__asm("cps 0x17");

	//On repasse sur la table des pages du processus suivant.
	load_page_table(get_current_process_page_table());

	//On reactive le timer.
	ENABLE_TIMER_IRQ();

	//Restauration du contexte d'execution et retour .
	__asm("ldmfd sp!, {r0-r12, pc}^");
}
