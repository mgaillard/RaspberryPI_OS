#ifndef VMEM_H
#define VMEM_H

#include <inttypes.h>
#include "sched.h"

//Direction de l'allocation en mémoire.
#define UP 1
#define DOWN -1

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
#define FRAME_OCCUPANCY_TT_SIZE (DEVICE_SPACE_END + 1) / PAGE_SIZE

/**
 * Initialise la mémoire virtuelle.
 */
void vmem_init();

/**
 * Initialise une table des pages pour un processus.
 */
uint32_t* init_process_translation_table();

/**
 * Change la table des pages par celle du noyau.
 */
void load_kernel_page_table();

/**
 * Change la table des pages pointée par la MMU.
 */
void load_page_table(const uint32_t* table);

/**
 * Alloue des pages en espace utilisateur.
 * 
 */
uint8_t* vmem_alloc_for_userland(uint32_t* page_table, uint32_t size, uint32_t address, int direction);

/**
 * Copie une zone de mémoire dans une autre zone.
 * Attention la copie doit faire au moins 4 octets.
 * @param destination L'adresse de destination.
 * @param source L'adresse source.
 * @param size La taille de la zone mémoire à copier.
 * @return La destination.
 */
void* memcpy(void* destination, const void* source, uint32_t size);

/**
 * Copie le contenu d'une frame dans une autre frame.
 */
void vmem_copy_frame(uint32_t destination_frame, uint32_t source_frame);

/**
 * Libère une plage de pages mémoires dans une table de pages.
 * @param page_table La table des pages dans laquelle libérer la mémoire.
 * @param address L'adresse de début de la plage de pages.
 * @param size La taille de la mémoire à libérer.
 */
void vmem_free(uint32_t* page_table, uint8_t* address, uint32_t size);

/**
 * Handler de l'évenement data abort.
 */
void __attribute__((naked)) data_handler();

#endif
