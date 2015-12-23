#include "vmem.h"
#include "kheap.h"
#include "asm_tools.h"
#include "hw.h"
#include "page_table.h"

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

/**
 * 
 */
uint8_t* vmem_alloc_for_userland(struct pcb_s* process, uint32_t size)
{
	return 0;
}
