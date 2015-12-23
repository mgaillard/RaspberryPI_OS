#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <inttypes.h>

#define FIRST_LEVEL_FLAGS 0x1

//Table de pagination
//Ajouter une correspondance dans la table de pagination
//Supprimer une correspondance dans la table de pagination
//Recuperer l'adresse d'une table de niveau 2
//Allouer une table de niveau 1

/**
 * Initialise la table d'occupation des frames.
 * @param size Le nombre de frame gérées par la table d'occupation. 
 */
void init_frame_occupancy_table(uint32_t size);

/**
 * Libère la table d'occupation des frames.
 */
void free_frame_occupancy_table();

/**
 * Retourne l'état d'une frame.
 * @param frame Le numéro de la frame.
 */
uint32_t get_frame_occupancy_table(uint32_t frame);

/**
 * Crée une table de page vide.
 */
uint32_t* create_page_table();

/**
 * Libère une table de page.
 */
void free_page_table(uint32_t* page_table);

/**
 * Ajoute une entrée a la table des pages.
 * @param page_table La table des pages a laquelle ajouter l'entrée.
 * @param first_index L'index de niveau 1 de la frame dans la table des pages.
 * @param second_index L'index de niveau 2 de la frame dans la table des pages.
 * @param frame_address L'adresse de debut de la frame.
 * @param frame_flags Les flags à appliquer à la frame.
 */
void add_entry_page_table(uint32_t* page_table, uint32_t first_level_index, uint32_t second_level_index, uint32_t frame_address, uint32_t frame_flags);

/**
 * Traduit une adresse logique selon une table de pagination.
 * @param va L'adresse logique à traduire.
 * @param page_table L'adresse de la table de pagination.
 */
uint32_t vmem_translate(uint32_t va, uint32_t* page_table);

#endif
