#ifndef HEAP_H
#define HEAP_H

#include <inttypes.h>

//-----------------------------------------------------------------Types
enum MemoryBlockType
{
    FREE, OCCUPIED
};
typedef enum MemoryBlockType MemoryBlockType;

struct MemoryBlock
{
	struct MemoryBlock* previous;
	struct MemoryBlock* next;
	uint8_t* address;
	uint32_t size;
	MemoryBlockType type;
};
typedef struct MemoryBlock MemoryBlock;

//---------------------------------------------------Fonctions publiques
/**
 * Initialise un tas contenant les blocs de mémoire alloués d'un processus.
 * @param address L'adresse à laquelle commence le tas.
 * @return Un pointeur vers une structure decrivant le tas.
 */
MemoryBlock* heap_init(void* address);

/**
 * Alloue un bloc de size octets dans un tas d'un processus.
 * Si l'allocation n'est pas possible, retourne l'adresse 0.
 * @param heap Le tas dans lequel allouer le bloc.
 * @param page_table La table des pages dans laquelle allouer des pages si le tas est trop petit.
 * @param size La taille en octet à allouer.
 * @return L'adresse de debut de la zone allouée.
 */
void* heap_alloc(MemoryBlock* heap, uint32_t* page_table, uint32_t size);

/**
 * Retourne la taille occupée en mémoire par le tas.
 * @param heap Le tas dont on veut connaitre la taille.
 * @return La taille totale du tas.
 */
uint32_t heap_size(MemoryBlock* heap);

/**
 * Libère la mémoire du bloc commençant à une certaine adresse.
 * @param heap Le tas dans lequel libérer le bloc.
 * @param address L'adresse de début du bloc à libérer.
 */
void heap_free(MemoryBlock* heap, void* address);

/**
 * Libère la mémoire du tas d'un processus.
 * @param heap Le tas à libérer.
 * @param page_table La table des pages dans laquelle libérer les pages occupées par le tas.
 */
void heap_free_all(MemoryBlock* heap, uint32_t* page_table);



#endif