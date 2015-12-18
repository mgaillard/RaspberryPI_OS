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
 * virtuelle située entre 0x0 et __kernel_heap_end__ renvoies vers ces mêmes
 * adresses physique.
 * Dans cet intervalle, il y a 16 777 215 adresses. Une page contient 4096
 * adresses. On doit initialiser 4096 pages réparties dans 16 tables de niveau 2
 * et une table de niveau 1.
 */
uint32_t* init_kern_translation_table()
{
	uint32_t* table_niveau1;
	uint32_t* table_niveau2;
	//Le nombre de table de niveau 2 pour mapper l'espace du noyau.
	const uint32_t SECOND_LVL_TT_KERNEL_NB = ((uint32_t)&__kernel_heap_end__ + 1) / (SECOND_LVL_TT_COUNT*PAGE_SIZE);
	const uint32_t SECOND_LVL_TT_DEVICES_START = DEVICE_SPACE_START / (SECOND_LVL_TT_COUNT*PAGE_SIZE);
	const uint32_t SECOND_LVL_TT_DEVICES_END = (DEVICE_SPACE_END + 1) / (SECOND_LVL_TT_COUNT*PAGE_SIZE);
	const uint32_t NIVEAU1_FLAGS = 0x1;
	const uint32_t NIVEAU2_FLAGS = 0x52;
	const uint32_t NIVEAU2_DEVICE_FLAGS = 0x17;
	//On alloue la table de niveau 1.
	//L'index de niveau 1 est codé sur 12 bits, et il y a 2 bits à 0 au debut.
	table_niveau1 = (uint32_t*)kAlloc_aligned(FIRST_LVL_TT_SIZE, FIRST_LVL_INDEX_SIZE + 2);
	
	//On invalide toutes les entrees de la table de niveau 1. Avant de les initialiser correctement.
	for (int i = 0;i < FIRST_LVL_TT_COUNT;i++)
	{
		table_niveau1[i] = 0;
	}
	
	//On alloue les tables de niveau 2 correspondant aux adresses de 0x0 à 0xFFFFFF.
	for (int i = 0;i < SECOND_LVL_TT_KERNEL_NB;i++)
	{
		//L'index de niveau 2 est codé sur 8 bits, et il y a 2 bits à 0 au debut.
		table_niveau2 = (uint32_t*)kAlloc_aligned(SECOND_LVL_TT_SIZE, SECOND_LVL_INDEX_SIZE + 2);
		//On fait pointer une entrée de la table de niveau 1 vers cette table de niveau 2.
		table_niveau1[i] = (uint32_t)table_niveau2 + NIVEAU1_FLAGS;
		//On initialise les entrées de cette table.
		for (int j = 0;j < SECOND_LVL_TT_COUNT;j++)
		{
			//On calcule le debut de la page en fonction des index de niveau 1 et de niveau 2.
			uint32_t adresse_page = i*SECOND_LVL_TT_COUNT*PAGE_SIZE + j*PAGE_SIZE;
			table_niveau2[j] = adresse_page + NIVEAU2_FLAGS;
		}
	}
	
	//On alloue 16 tables de niveau 2 correspondant aux adresses de 0x20000000 à 0x20FFFFFF.
	for (int i = SECOND_LVL_TT_DEVICES_START;i < SECOND_LVL_TT_DEVICES_END;i++)
	{
		//L'index de niveau 2 est codé sur 8 bits, et il y a 2 bits à 0 au debut.
		table_niveau2 = (uint32_t*)kAlloc_aligned(SECOND_LVL_TT_SIZE, SECOND_LVL_INDEX_SIZE + 2);
		//On fait pointer une entrée de la table de niveau 1 vers cette table de niveau 2.
		table_niveau1[i] = (uint32_t)table_niveau2 + NIVEAU1_FLAGS;
		//On initialise les entrées de cette table.
		for (int j = 0;j < SECOND_LVL_TT_COUNT;j++)
		{
			//On calcule le debut de la page en fonction des index de niveau 1 et de niveau 2.
			uint32_t adresse_page = i*SECOND_LVL_TT_COUNT*PAGE_SIZE + j*PAGE_SIZE;
			table_niveau2[j] = adresse_page + NIVEAU2_DEVICE_FLAGS;
		}
	}
	
	return table_niveau1;
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
	
	const uint32_t NIVEAU1_FLAGS = 0x1;
	
	//On alloue la table de niveau 1.
	uint32_t* table_niveau1 = (uint32_t*)kAlloc_aligned(FIRST_LVL_TT_SIZE, FIRST_LVL_INDEX_SIZE + 2);
	//On invalide toutes les entrees de la table de niveau 1. Avant de les initialiser correctement.
	for (int i = 0;i < FIRST_LVL_TT_COUNT;i++)
	{
		table_niveau1[i] = 0;
	}
	
	//On alloue une table de niveau 2.
	uint32_t* table_niveau2 = (uint32_t*)kAlloc_aligned(SECOND_LVL_TT_SIZE, SECOND_LVL_INDEX_SIZE + 2);
	table_niveau1[0] = (uint32_t)table_niveau2 + NIVEAU1_FLAGS;
	//On initialise les entrées de cette table a 0.
	for (int i = 0;i < SECOND_LVL_TT_COUNT;i++)
	{
		table_niveau2[i] = 0;
	}
	//On map les entrees de la table de niveau 2 du processus sur celles du noyaux.
	uint32_t* mmu_table_niveau2 = (uint32_t*) (mmu_table_base[0] & 0xFFFFFC00);
	for (int second_level_index = 0;second_level_index < KERNEL_FRAME_NB;second_level_index++)
	{
		table_niveau2[second_level_index] = mmu_table_niveau2[second_level_index];
	}
	
	//On alloue les tables de niveau 2 correspondant aux adresses des devices.
	for (int i = SECOND_LVL_TT_DEVICES_START;i < SECOND_LVL_TT_DEVICES_END;i++)
	{
		table_niveau1[i] = mmu_table_base[i];
	}
	
	return table_niveau1;
}

/**
 * Libère l'espace mémoire occupé par une table des pages.
 * Libère les pages de niveau 2 appartenant au processus.
 * C'est à dire celle qui n'appartiennent pas au noyau.
 * Libère la table de niveau 1.
 */
void free_process_translation_table(uint32_t* table)
{
	/*
	const uint32_t SECOND_LVL_TT_KERNEL_NB = ((uint32_t)kernel_heap_limit + 1) / (SECOND_LVL_TT_COUNT*PAGE_SIZE);
	const uint32_t SECOND_LVL_TT_DEVICES_START = DEVICE_SPACE_START / (SECOND_LVL_TT_COUNT*PAGE_SIZE);
	
	//On libère les tables de niveau 2 n'appartenant pas au noyau.
	for (int i = SECOND_LVL_TT_KERNEL_NB;i < SECOND_LVL_TT_DEVICES_START;i++)
	{
		//Pour chaque table de niveau 2, on libère les frames.
		
		//Enfin, on libère toute la table de niveau 2.
		//table[i];
	}
	*/
	//On libère la table de niveau 1.
	kFree((void*)(table), FIRST_LVL_TT_SIZE);
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

uint32_t vmem_translate(uint32_t va, struct pcb_s* process)
{
    uint32_t pa;/*The result */
    
    /* 1st and 2nd table addresses*/
    uint32_t table_base;
    uint32_t second_level_table;
    
    /*Indexes*/
    uint32_t first_level_index;
    uint32_t second_level_index;
    uint32_t page_index;
    
    /*Descriptors*/
    uint32_t first_level_descriptor;
    uint32_t* first_level_descriptor_address;
    uint32_t second_level_descriptor;
    uint32_t* second_level_descriptor_address;
    
    if(process == NULL)
    {
        __asm("mrc p15,0,%[tb],c2,c0,0": [tb] "=r"(table_base));
    }
    else
    {
        table_base = (uint32_t)process->page_table;
    }
    
    table_base = table_base & 0xFFFFC000;
    
    /*Indexes*/
    first_level_index = (va >> 20);
    second_level_index = ((va << 12) >> 24);
    page_index = (va & 0x00000FFF);
    
    /*First level descriptor*/
    first_level_descriptor_address = (uint32_t*) (table_base | (first_level_index << 2));
    first_level_descriptor = *(first_level_descriptor_address);
    
    /*Translation fault*/
    if(! (first_level_descriptor & 0x3)) {
        return(uint32_t) FORBIDDEN_ADDRESS;
    }
    
    /*Second level descriptor*/
    second_level_table = first_level_descriptor & 0xFFFFFC00;
    second_level_descriptor_address = (uint32_t*) (second_level_table | (second_level_index << 2));
    second_level_descriptor = *((uint32_t*)second_level_descriptor_address);
    
    /*Translation fault*/
    if(! (second_level_descriptor & 0x3)) {
        return (uint32_t) FORBIDDEN_ADDRESS;
    }
    
    /*Physical address*/
    pa = (second_level_descriptor & 0xFFFFF000) | page_index;
    return pa;
}
