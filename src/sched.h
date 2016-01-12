#ifndef SCHED_H
#define SCHED_H

#include <inttypes.h>
#include "vmem.h"
#include "heap.h"

//La taille de la stack allouée aux processus en octets.
#define PROCESS_STACK_SIZE 3*PAGE_SIZE

//La période pendant laquelle tous les processus seront exécutés.
#define TIME_SLICE 256

//Des constantes pour acceder aux cases memoires de struct pcb_s.
#define PCB_OFFSET_LR_USER sizeof(((struct pcb_s *)0)->registers)
#define PCB_OFFSET_LR_SVC PCB_OFFSET_LR_USER + sizeof(((struct pcb_s *)0)->lr_user)
#define PCB_OFFSET_SP PCB_OFFSET_LR_SVC + sizeof(((struct pcb_s *)0)->lr_svc)
#define PCB_OFFSET_CPSR PCB_OFFSET_SP + sizeof(((struct pcb_s *)0)->sp)
#define PCB_OFFSET_PAGE_TABLE PCB_OFFSET_CPSR + sizeof(((struct pcb_s *)0)->cpsr)

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

//-----------------------------------------------------------------Types
typedef int(func_t) (void);

enum ProcessState
{
    RUNNING, READY, WAITING, TERMINATED
};
typedef enum ProcessState ProcessState;

struct pcb_s
{
	//Un tableau contenant les registres du contexte.
	//Les cases 0 a 12 contiennent les registres R0-R12.
	uint32_t registers[13];
	//Le contenu du registre lr du mode user.
	func_t* lr_user;
	//Le lr_svc
	func_t* lr_svc;
	//Le pointeur de pile.
	void* sp;
	//Le registre CPSR.
	uint32_t cpsr;
	//Pointeur vers la table des pages du processus.
	uint32_t* page_table;
	//Le debut de la pile.
	void* debut_sp;
	//Le tas du processus;
	MemoryBlock* heap;
	//L'état du processus.
	ProcessState state;
	//Le code retour du processus.
	int returnCode;
	//Le poids du processus.
	uint32_t weight;
	//Le père du processus.
	struct pcb_s* parent_process;
	//Le processus precedent dans l'ordre du round robin.
	struct pcb_s* previous_process;
	//Le processus suivant dans l'ordre du round robin.
	struct pcb_s* next_process;
};

//---------------------------------------------------Fonctions publiques

void sched_init();
//Sauvegarde/Restaure le contexte et passe au processus dest.
void yieldto(int* pile);
//Sauvegarde/Restaure le contexte et passe au processus suivant.
void yield(int* pile);
//Termine le processus et et passe au processus suivant.
void exit_process(int* pile);
//Cree et alloue la memoire pour un nouveau processus.
struct pcb_s* create_process(func_t* entry, int32_t niceness);
//Sauvegarde le contexte du processus courant. Puis le Fork.
struct pcb_s* fork_current_process(int* pile);
//Libere la PCB d'un processus et le retire de la liste circulaire.
void free_process(struct pcb_s* process);
//Handler d'interruption du timer.
void irq_handler();
//Retourne la table des pages du processus courant.
uint32_t* get_current_process_page_table();
//Retourne le tas du processus courant.
MemoryBlock* get_current_process_heap();

#endif
