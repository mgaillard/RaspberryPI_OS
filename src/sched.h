#ifndef SCHED_H
#define SCHED_H

#include <inttypes.h>

//La taille de la stack allouée aux processus en octets.
#define PROCESS_STACK_SIZE 10*1024

//Des constantes pour acceder aux cases memoires de struct pcb_s.
#define PCB_OFFSET_LR_USER sizeof(((struct pcb_s *)0)->registers)
#define PCB_OFFSET_LR_SVC PCB_OFFSET_LR_USER + sizeof(((struct pcb_s *)0)->lr_user)
#define PCB_OFFSET_SP PCB_OFFSET_LR_SVC + sizeof(((struct pcb_s *)0)->lr_svc)
#define PCB_OFFSET_CPSR PCB_OFFSET_SP + sizeof(((struct pcb_s *)0)->sp)

typedef int(func_t) (void);

typedef enum ProcessState ProcessState;
enum ProcessState
{
    RUNNING, READY, WAITING, TERMINATED
};

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
	//Le debut de la pile.
	void* debut_sp;
	//L'état du processus.
	ProcessState state;
	//Le code retour du processus.
	int returnCode;
	//Le processus precedent dans l'ordre du round robin.
	struct pcb_s* previous_process;
	//Le processus suivant dans l'ordre du round robin.
	struct pcb_s* next_process;
};

void sched_init();
//Choisit le prochain processus a executer et on fait pointer current_process dessus.
void elect();
//Impose le prochain processu a executer.
void change_process(struct pcb_s* next_process);
//Cree et alloue la memoire pour un nouveau processus.
struct pcb_s* create_process(func_t* entry);
//Lance un processus.
void start_current_process();
//Termine le current_process.
void exit_process(int status);
//Libere la PCB d'un processus et le retire de la liste circulaire.
void free_process(struct pcb_s* process);
//Sauvegarde le contexte a partir des valeurs des registres presents dans la pile.
//Sauvegarde aussi la valeur des registres lr et sp du mode user.
//Le contexte est sauvegardé dans le current_process.
void save_context_svcmode(int* pile);
void save_context_irqmode(int* pile);
//Restaure le contexte dans la pile a partir des valeurs presents dans le current_process.
//Restaure aussi les valeurs des registres lr et sp du mode user.
void restore_context_svcmode(int* pile);
void restore_context_irqmode(int* pile);
//Handler d'interruption du timer.
void irq_handler();

#endif
