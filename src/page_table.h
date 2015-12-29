#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <inttypes.h>

#define FIRST_LEVEL_FLAGS 0x1
#define UP 1
#define DOWN -1

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
 * Retourne le numero d'une frame libre.
 * Si aucune frame n'est libre, retourne UINT32_MAX.
 */
uint32_t find_free_frame_occupancy_table();

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
 * @param page_table La table des pages dans laquelle ajouter l'entrée.
 * @param first_index L'index de niveau 1 de la page dans la table des pages.
 * @param second_index L'index de niveau 2 de la page dans la table des pages.
 * @param frame_address L'adresse de debut de la frame.
 * @param frame_flags Les flags à appliquer à la frame.
 */
void add_entry_page_table(uint32_t* page_table, uint32_t first_level_index, uint32_t second_level_index, uint32_t frame_address, uint32_t frame_flags);

/**
 * Trouve page_nb pages libres consecutives dans une table des pages.
 * Si les pages ne peuvent pas etre trouvées, retourne UINT32_MAX.
 * @param page_table La table des pages a laquelle ajouter l'entrée.
 * @param page_nb Le nombre de pages à trouver.
 * @param start_page Page à partir de laquelle chercher les pages libres.
 * @param direction La direction dans laquelle rechercher les pages libres. Positif ou nul pour rechercher vers les adresses hautes, négatif pour les adresses basses.
 * @return Le numéro de la page de la plage de page dont l'adresse est la plus basse.
 */
uint32_t find_free_pages_page_table(const uint32_t* page_table, uint32_t page_nb, uint32_t start_page, int direction);

/**
 * Libère une page dans une table des pages.
 * @param page_table La table des pages dans laquelle supprimer la page.
 * @param first_index L'index de niveau 1 de la page dans la table des pages.
 * @param second_index L'index de niveau 2 de la page dans la table des pages.
 */
void free_page_page_table(uint32_t* page_table, uint32_t first_level_index, uint32_t second_level_index);

/**
 * Traduit une adresse logique selon une table de pagination.
 * @param va L'adresse logique à traduire.
 * @param page_table L'adresse de la table de pagination.
 */
uint32_t vmem_translate(uint32_t va, uint32_t* page_table);

#endif
