#include "heap.h"
#include "kheap.h"
#include "vmem.h"
#include "config.h"

//-----------------------------------------------------Variables privees

//-----------------------------------------------------Fonctions privees

//----------------------------------------------------------Realisations
MemoryBlock* heap_init(void* address)
{
	//Allocation du premier bloc de la liste.
	MemoryBlock* heap = (MemoryBlock*)kAlloc(sizeof(MemoryBlock));
	//Initialisation du bloc.
	heap->next = NULL;
	heap->previous = NULL;
	heap->address = address;
	heap->size = 0;
	heap->type = FREE;

	return heap;
}

void* heap_alloc(MemoryBlock* heap, uint32_t* page_table, uint32_t size)
{
	//On recherche un bloc libre assez grand pour accueillir ce nouveau bloc.
	MemoryBlock* last_block;
	MemoryBlock* current_block = heap;
	while (current_block != NULL && (current_block->type != FREE || current_block->size < size))
	{
		last_block = current_block;
		current_block = current_block->next;
	}

	//On n'a pas trouvé un bloc dans la liste. On alloue des pages en plus.
	if (current_block == NULL && last_block != NULL)
	{
		uint32_t start_address = (uint32_t)last_block->address + last_block->size;
		//On calcule la taille des pages nécessaires.
		uint32_t pages_size = (((size - 1) / PAGE_SIZE) + 1) * PAGE_SIZE;
		uint8_t* pages_address = vmem_alloc_for_userland(page_table, pages_size, start_address, UP);
		//Si l'allocation de page a réussie.
		if (pages_address != NULL)
		{
			//On ajoute un bloc libre à la fin.
			if (last_block->type == FREE)
			{
				//Si le dernier bloc est déjà libre, on l'agrandi.
				last_block->address = pages_address - last_block->size;
				last_block->size += pages_size;
				current_block = last_block;
			}
			else
			{
				//Sinon on crée un bloc libre à la fin.
				MemoryBlock* new_block = (MemoryBlock*)kAlloc(sizeof(MemoryBlock));
				//Initialisation du bloc.
				new_block->next = NULL;
				new_block->previous = last_block;
				new_block->address = pages_address;
				new_block->size = pages_size;
				new_block->type = FREE;
				//On relit le bloc au precedent.
				last_block->next = new_block;
				//On change de current_block maintenant qu'il y en a un nouveau à la fin.
				current_block = new_block;
			}
		}
	}

	//On alloue un bloc dans le bloc vide si on en a trouvé un.
	if (current_block != NULL && current_block->size >= size)
	{
		if (current_block->size == size)
		{
			//La taille du bloc libre est déjà adaptée, il suffit de passer ce bloc en occupé.
			current_block->type = OCCUPIED;
		}
		else
		{
			//On coupe le bloc courant en deux. La première partie sera occupée, la deuxième libre.
			MemoryBlock* new_block = (MemoryBlock*)kAlloc(sizeof(MemoryBlock));
			new_block->address = current_block->address + size;
			new_block->size = current_block->size - size;
			new_block->type = FREE;
			//On relie les blocs pour insérer le nouveau bloc.
			new_block->next = current_block->next;
			current_block->next->previous = new_block;
			new_block->previous = current_block;
			current_block->next = new_block;
			//On redimensionne le bloc courant.
			current_block->size = size;
			current_block->type = OCCUPIED;
		}
		//On retourne l'adresse de début du bloc.
		return current_block->address;
	}
	else
	{
		return NULL;
	}
}

void heap_free(MemoryBlock* heap, void* address)
{
	//On recherche le bloc débutant à cette adresse et on le libère.
	MemoryBlock* current_block = heap;
	while (current_block != NULL && current_block->address != address)
	{
		current_block = current_block->next;
	}
	//Si le bloc débutant à cette adresse a été trouvé.
	if (current_block != NULL)
	{
		//On libère le bloc.
		current_block->type = FREE;
		//Si il y a un bloc vide avant, on fusionne les deux blocs.
		if (current_block->previous != NULL && current_block->previous->type == FREE)
		{
			current_block->previous->size += current_block->size;
			//On reconstitue les liens.
			current_block->previous->next = current_block->next;
			//Si il y a effectivement un noeud suivant.
			if (current_block->next != NULL)
			{
				current_block->next->previous = current_block->previous;
			}
			//On libère le bloc courant.
			MemoryBlock* previous_block = current_block->previous;
			kFree((void*)current_block, sizeof(MemoryBlock));
			current_block = previous_block;
		}
		//Si il y a un bloc vide après, on fusionne les deux blocs.
		if (current_block->next != NULL && current_block->next->type == FREE)
		{
			current_block->size += current_block->next->size;
			MemoryBlock* next_block = current_block->next;

			current_block->next = current_block->next->next;
			//S'il y a un bloc après le suivant.
			if (current_block->next != NULL)
			{
				current_block->next->previous = current_block;
			}
			kFree((void*)next_block, sizeof(MemoryBlock));
		}
	}
}

void heap_free_all(MemoryBlock* heap, uint32_t* page_table)
{
	//On retient la taille totale du tas pour libérer les pages mémoire.
	uint32_t size = 0;
	//On libère toute la liste des blocs en la parcourant.
	MemoryBlock* next_bloc;
	MemoryBlock* current_block = heap;
	while (current_block != NULL)
	{
		next_bloc = current_block->next;

		size += current_block->size;
		kFree((void*)current_block, sizeof(MemoryBlock));

		current_block = next_bloc;
	}
	//On libère les pages mémoire réservées au tas, si le tas a été alloué.
	if (size > 0)
	{
		vmem_free(page_table, (uint8_t*)heap->address, size);
	}
}