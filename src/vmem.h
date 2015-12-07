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

//Taille de la table d'occupation des pages.
#define PAGE_OCCUPANCY_TT_SIZE 1048576

//Adresse de fin de la ram.
#define RAM_LIMIT 0x1FFFFFFF
//Adresse de debut des devices.
#define DEVICE_SPACE_START 0x20000000
//Adresse de fin des devices.
#define DEVICE_SPACE_END 0x20FFFFFF

void vmem_init();
uint32_t* init_kern_translation_table();
uint8_t* vmem_alloc_for_userland(struct pcb_s* process, uint32_t size);
uint32_t vmem_translate(uint32_t va, struct pcb_s* process);

#endif
