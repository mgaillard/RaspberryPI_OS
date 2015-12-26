#include "page_table.h"
#include "kheap.h"
#include "vmem.h"
#include "config.h"

//-----------------------------------------------------Variables privees
uint32_t frame_occupancy_table_size;
uint32_t frame_occupancy_table_last_free_frame;
uint8_t* frame_occupancy_table;

//-----------------------------------------------------Fonctions privees
/**
 * Retourne l'adresse de la table de niveau 2 en fonction de l'index de niveau 1.
 * Si la table de niveau 2 n'est pas définie, retourne la constante FORBIDDEN_ADDRESS.
 */
uint32_t* second_level_page_table(const uint32_t* page_table, uint32_t first_level_index);

/**
 * Retourne 1 si une page est libre, 0 sinon.
 * @param page_table La table des pages dans laquelle rechercher la page.
 * @param first_level_index L'index de niveau 1 de la page.
 * @param second_level_index L'index de niveau 2 de la page.
 */
int is_free_page_table(const uint32_t* page_table, uint32_t first_level_index, uint32_t second_level_index);

/**
 * Crée une table de niveau 2 vide.
 */
uint32_t* create_second_level_page_table();

/**
 * Libère une table de niveau 2.
 */
void free_second_level_page_table(uint32_t* second_level_table);

/**
 * Change l'état d'une frame.
 * @param frame Le numéro de la frame.
 * @param state L'état de la frame. 1 Si la frame est occupée, 0 sinon.
 */
void set_frame_occupancy_table(uint32_t frame, uint32_t state);


uint32_t* second_level_page_table(const uint32_t* page_table, uint32_t first_level_index)
{   
    /* 1st and 2nd table addresses*/
    uint32_t table_base;
    uint32_t second_level_table;
    
    /*Descriptors*/
    uint32_t first_level_descriptor;
    uint32_t* first_level_descriptor_address;
    
    table_base = (uint32_t)page_table & 0xFFFFC000;
    
    /*First level descriptor*/
    first_level_descriptor_address = (uint32_t*) (table_base | (first_level_index << 2));
    first_level_descriptor = *(first_level_descriptor_address);
    
    /*Translation fault*/
    if(! (first_level_descriptor & 0x3)) {
        return (uint32_t*)FORBIDDEN_ADDRESS;
    }
    
    second_level_table = first_level_descriptor & 0xFFFFFC00;
    
    return (uint32_t*)second_level_table;
}

int is_free_page_table(const uint32_t* page_table, uint32_t first_level_index, uint32_t second_level_index)
{
    uint32_t* second_level_table = second_level_page_table(page_table, first_level_index);
    if (second_level_table != FORBIDDEN_ADDRESS)
    {
        //On explore la table de niveau 2 a la recherche de la page.
        if (second_level_table[second_level_index] != 0)
        {
            return 0;
        }
    }
    //La page n'a pas été trouvée, elle est libre.
    return 1;
}

uint32_t* create_second_level_page_table()
{
	//On alloue une table de niveau 2.
	uint32_t* table_niveau2 = (uint32_t*)kAlloc_aligned(SECOND_LVL_TT_SIZE, SECOND_LVL_INDEX_SIZE + 2);
	
	//On initialise les entrées de cette table a 0.
	for (int i = 0;i < SECOND_LVL_TT_COUNT;i++)
	{
		table_niveau2[i] = 0;
	}
	
	return table_niveau2;
}

void free_second_level_page_table(uint32_t* second_level_table)
{
    //Pour chaque frame de la table de niveau 2.
    for (uint32_t second_level_index = 0;second_level_index < SECOND_LVL_TT_COUNT;second_level_index++)
    {
        uint32_t frame = (second_level_table[second_level_index] & 0xFFFFF000) / PAGE_SIZE;
        second_level_table[second_level_index] = 0;
        //On précise que la frame n'est plus occupée par cette table.
        set_frame_occupancy_table(frame, 0);
    }
    //On libère la mémoire.
    kFree((void*)second_level_table, SECOND_LVL_TT_SIZE);
}

void set_frame_occupancy_table(uint32_t frame, uint32_t state)
{
    if (state > 0) {
        frame_occupancy_table[frame] += 1;
    } else {
        frame_occupancy_table[frame] -= 1;
    }
}

//----------------------------------------------------------Realisations
void init_frame_occupancy_table(uint32_t size)
{
    frame_occupancy_table_size = size;
	//On alloue la table d'occupation des pages.
	frame_occupancy_table = kAlloc(frame_occupancy_table_size);

    frame_occupancy_table[frame_occupancy_table_size - 1] = 0;
    //On initialise toutes les pages comme occupees.
	for (int i = 0;i < frame_occupancy_table_size;i++)
	{
		frame_occupancy_table[i] = 0;
	}
    //On initialise la derniere frame libre.
    frame_occupancy_table_last_free_frame = 0;
}

void free_frame_occupancy_table()
{
	kFree(frame_occupancy_table, frame_occupancy_table_size);
}

uint32_t get_frame_occupancy_table(uint32_t frame)
{
    return frame_occupancy_table[frame];
}

uint32_t find_free_frame_occupancy_table()
{
    //A partir de la dernière frame libre, on trouve une frame libre.
    uint32_t frame = frame_occupancy_table_last_free_frame + 1;
    //Tant que la frame considerée n'est pas libre et que l'on n'a pas testé toutes les frames de la table.
    while (frame_occupancy_table[frame] > 0 && frame != frame_occupancy_table_last_free_frame)
    {
        //On teste la frame suivante.
        frame += 1;
        //Si on dépasse la taille de la table d'ocupation, on revient au début.
        if (frame >= frame_occupancy_table_size)
        {
            frame = 0;
        }
    }

    //On vérifie que la table n'est pas pleine.
    if (frame_occupancy_table[frame] == 0 && frame != frame_occupancy_table_last_free_frame)
    {
        frame_occupancy_table_last_free_frame = frame;
    }
    else
    {
        //Si la table est pleine, aucune frame n'est libre. On retourne UINT32_MAX.
        frame = UINT32_MAX;
    }

    return frame;
}

void free_page_page_table(uint32_t* page_table, uint32_t first_level_index, uint32_t second_level_index)
{
    uint32_t* second_level_table = second_level_page_table(page_table, first_level_index);
    if (second_level_table != FORBIDDEN_ADDRESS)
    {
        //On explore la table de niveau 2 a la recherche de la page.
        if (second_level_table[second_level_index] != 0)
        {
            //On supprime l'entrée dans la table de niveau 2.
            uint32_t frame = (second_level_table[second_level_index] & 0xFFFFF000) / PAGE_SIZE;
            second_level_table[second_level_index] = 0;
            //On précise que la frame n'est plus occupée par cette table.
            set_frame_occupancy_table(frame, 0);
        }
    }
}

uint32_t* create_page_table()
{
	//On alloue la table de niveau 1.
	uint32_t* table_niveau1 = (uint32_t*)kAlloc_aligned(FIRST_LVL_TT_SIZE, FIRST_LVL_INDEX_SIZE + 2);
	//On invalide toutes les entrees de la table de niveau 1.
	for (int i = 0;i < FIRST_LVL_TT_COUNT;i++)
	{
		table_niveau1[i] = 0;
	}
	return table_niveau1;
}

void free_page_table(uint32_t* page_table)
{
    //On libère les pages de niveau 2.
    for (uint32_t first_level_index = 0;first_level_index < FIRST_LVL_TT_COUNT;first_level_index++)
    {
        uint32_t* second_level_table = second_level_page_table(page_table, first_level_index);
        if (second_level_table != FORBIDDEN_ADDRESS)
        {
            free_second_level_page_table(second_level_table);
        }
    }

	kFree((void*)(page_table), FIRST_LVL_TT_SIZE);
}

void add_entry_page_table(uint32_t* page_table, uint32_t first_level_index, uint32_t second_level_index, uint32_t frame_address, uint32_t frame_flags)
{
	uint32_t* second_level_table;
	//On vérifie si la table de niveau 2 correspondante est allouée.
	second_level_table = second_level_page_table(page_table, first_level_index);
	if (second_level_table == FORBIDDEN_ADDRESS)
	{
		//On alloue une table de niveau 2.
		second_level_table = create_second_level_page_table();
		//On fait le lien entre la table de niveau 1 et cette table de niveau 1.
		page_table[first_level_index] = (uint32_t)second_level_table | FIRST_LEVEL_FLAGS;
	}
	//On ajoute l'entrée à la table de niveau 2.
	second_level_table[second_level_index] = frame_address | frame_flags;
	//On note la frame comme occupée.
    set_frame_occupancy_table(frame_address / PAGE_SIZE, 1);
}

uint32_t find_free_pages_page_table(const uint32_t* page_table, uint32_t page_nb)
{
    const uint32_t MAX_PAGE = UINT32_MAX / PAGE_SIZE;

    uint32_t successive_free_page = 0;
    //On fait correspondre les adresses logiques et physiques du noyau.
    for (int page = 0;page < MAX_PAGE;page++)
    {
        //On retrouve pour la page, les index de niveau 1 et de niveau 2.
        uint32_t first_level_index = page / SECOND_LVL_TT_COUNT;
        uint32_t second_level_index = page - first_level_index * SECOND_LVL_TT_COUNT;

        if (is_free_page_table(page_table, first_level_index, second_level_index))
        {
            successive_free_page++;
        }
        else
        {
            successive_free_page = 0;
        }

        //On a trouve un enchainement de page_nb pages libres consecutives.
        if (successive_free_page >= page_nb)
        {
            return page + 1 - page_nb;
        }
    }
    //On ne peut pas trouver assez de pges libres.
    return UINT32_MAX;
}

uint32_t vmem_translate(uint32_t va, uint32_t* page_table)
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
    
    if(page_table == NULL)
    {
        __asm("mrc p15,0,%[tb],c2,c0,0": [tb] "=r"(table_base));
    }
    else
    {
        table_base = (uint32_t)page_table;
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