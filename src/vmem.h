#ifndef VMEM_H
#define VMEM_H

#include <inttypes.h>
#include "sched.h"

//Taille d'une page.
#define PAGE_SIZE 4096

//Nombre de bits sur lequel est codé l'index de niveau 1.
#define FIRST_LVL_INDEX_SIZE 12
//Nombre d'entrées dans une table de niveau 1.
#define FIRST_LVL_TT_COUNT 4096
//Taille en octets d'une table de niveau 1.
#define FIRST_LVL_TT_SIZE 16384

//Le nombre de bits sur lequel est codé l'index de niveau 2.
#define SECOND_LVL_INDEX_SIZE 8
//Nombre d'entrées dans une table de niveau 2.
#define SECOND_LVL_TT_COUNT 256
//Taille en octets d'une table de niveau 2.
#define SECOND_LVL_TT_SIZE 1024

//Adresse de fin de la ram.
#define RAM_LIMIT 0x1FFFFFFF
//Adresse de debut des devices.
#define DEVICE_SPACE_START 0x20000000
//Adresse de fin des devices.
#define DEVICE_SPACE_END 0x20FFFFFF

//Taille de la table d'occupation des frames.
#define FRAME_OCCUPANCY_TT_SIZE (RAM_LIMIT + 1) / PAGE_SIZE

/**
 * Initialise la mémoire virtuelle.
 */
void vmem_init();

/**
 * Initialise une table des pages pour un processus.
 */
uint32_t* init_process_translation_table();

/**
 * Libère la mémoire occupée par la table de traduction d'un process.
 */
void free_process_translation_table(uint32_t* table);

/**
 * Change la table des pages par celle du noyau.
 */
void load_kernel_page_table();

/**
 * Change la table des pages pointée par la MMU.
 */
void load_page_table(const uint32_t* table);

uint8_t* vmem_alloc_for_userland(struct pcb_s* process, uint32_t size);
uint32_t vmem_translate(uint32_t va, struct pcb_s* process);

#endif
