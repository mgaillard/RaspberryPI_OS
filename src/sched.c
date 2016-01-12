#include "sched.h"
#include "kheap.h"
#include "hw.h"
#include "asm_tools.h"
#include "syscall.h"
#include "vmem.h"
#include "page_table.h"
#include "util.h"

//----------------------------------------------------Variables globales

struct pcb_s *current_process;
struct pcb_s kmain_process;
uint32_t total_weight;

//------------------------------------------------------Fonction privées

//Lance le processus courant.
void start_current_process();
//Choisit le prochain processus a executer et on fait pointer current_process dessus.
void elect();
//Impose le prochain processus a executer.
void change_process(struct pcb_s* next_process);
//Sauvegarde le contexte a partir des valeurs des registres presents dans la pile.
//Sauvegarde aussi la valeur des registres lr et sp du mode user.
//Le contexte est sauvegardé dans le current_process.
void save_context(int* pile);
//Restaure le contexte dans la pile a partir des valeurs presents dans le current_process.
//Restaure aussi les valeurs des registres lr et sp du mode user.
void restore_context(int* pile);
//Convertit une niceness en poids.
uint32_t niceness_to_weight(int niceness);
//Convertit le poids d'un processus en temps d'execution.
uint32_t weight_to_timeslice(uint32_t weight);


//-----------------------------------------------------------Réalisation

void sched_init()
{
	kheap_init();
	#if VMEM
		vmem_init();
	#else
		timer_init();
	#endif
	//Initialisation du kmain_process.
	//On fait pointer son son next_process sur lui meme
	//car il est le seul processus a etre execute.
	kmain_process.previous_process = &kmain_process;
	kmain_process.next_process = &kmain_process;
	kmain_process.parent_process = 0;
	//On initialise l'etat du processus dans la PCB.
	kmain_process.state = RUNNING;
	//Initialisation de la table des pages du processus.
	kmain_process.page_table = init_process_translation_table();
	//On passe sur la table des pages de kmain
	load_page_table(kmain_process.page_table);
	//On initialise le code de retour.
	kmain_process.returnCode = -1;
	//On initialise la priorité du processus kmain.
	kmain_process.weight = niceness_to_weight(20);
	total_weight += kmain_process.weight;
	//On precise que c'est le processus courant.
	current_process = &kmain_process;
	//On configure la duree avant le prochain changement de contexte.
    set_next_tick(weight_to_timeslice(current_process->weight));
}

void elect()
{
	//On cherche le prochain processus qui est READY.
	struct pcb_s* next_process = current_process->next_process;
	//On fait attention a ne pas faire un tour entier de la liste circulaire des processus.
	while (next_process->state != READY && next_process != current_process)
	{
		next_process = next_process->next_process;
	}
	//Si on a trouve un processus READY qui n'est pas le processus courant.
	if (next_process != current_process)
	{
		//On passe au processus suivant.
		change_process(next_process);
	}
	else if (current_process->state == TERMINATED)
	{
		//Aucun processus n'est READY et le processus courant est termine.
		//On arrete le noyau.
		terminate_kernel();
	}
	//On ne change rien, on continue a executer le processus courant, car aucun autre processus n'est pret.
}

void change_process(struct pcb_s* next_process)
{
	//Si le processus courant est execute, on le met dans l'etat READY.
	if (current_process->state == RUNNING)
	{
		current_process->state = READY;
	}
	//Le nouveau processus a executer est celui qui suit le processus courant.
	current_process = next_process;
	//On met le nouveau processus courant dans l'etat RUNNING.
	current_process->state = RUNNING;
    //On configure la duree avant le prochain changement de contexte.
    set_next_tick(weight_to_timeslice(current_process->weight));
}

void yieldto(int* pile)
{
	struct pcb_s* dest = (struct pcb_s*)pile[1];
	//On sauvegarde le contexte d'execution.
	save_context(pile);
	//On passe au processus suivant.
	change_process(dest);
	//On restaure le contexte d'execution.
	restore_context(pile);
}

void yield(int* pile)
{
	//On sauvegarde le contexte d'execution.
	save_context(pile);
	//On passe au processus suivant.
	elect();
	//On restaure le contexte d'execution.
	restore_context(pile);
}

void exit_process(int* pile)
{
	//On sauvegarde le contexte d'execution.
	save_context(pile);
	//On retire le weight du processus du total_weight.
    total_weight -= current_process->weight;
	//On marque le current_process comme termine.
	//La PCB reste dans la liste circulaire dans l'etat TERMINATED.
	//Grace a un autre appel systeme on pourra recuperer son status et liberer la pcb.
	//On marque le processus comme TERMINATED.
	current_process->state = TERMINATED;
	//On enregistre son code retour.
	current_process->returnCode = pile[1];
	//On libere la pile de ce processus.
	vmem_free(current_process->page_table, current_process->debut_sp - PROCESS_STACK_SIZE, PROCESS_STACK_SIZE);
	//On libère le tas de ce processus.
	heap_free_all(current_process->heap, current_process->page_table);
	//On libère toute la mémoire de ce processus.
	free_page_table(current_process->page_table);
	//On passe au process suivant.
	elect();
	//On restaure le contexte d'execution.
	restore_context(pile);
}

struct pcb_s* create_process(func_t* entry, int32_t niceness)
{
	//Allocation dynamique d'un struct pcb_s pour le nouveau processus.
	struct pcb_s* process_pcb = (struct pcb_s*)kAlloc(sizeof(struct pcb_s));
	//Initialisation de lr au debut de la fonction
	process_pcb->lr_user = entry;
	process_pcb->lr_svc = (func_t*)&start_current_process;
	//Initialisation de la table des pages du processus.
	process_pcb->page_table = init_process_translation_table();
	//Initialisation de la pile du processus : 12ko.
	//La pile du processus est alloué dans le tas qui grossit vers les adresse hautes.
	//La pile grandira vers le bas, donc il faut mettre le pointeur de pile en haut de la zone allouée.
	process_pcb->debut_sp = vmem_alloc_for_userland(process_pcb->page_table, PROCESS_STACK_SIZE, UINT32_MAX, DOWN) + PROCESS_STACK_SIZE;
	process_pcb->sp = process_pcb->debut_sp;
	//On initialise le tas.
	process_pcb->heap = heap_init(0);
	//Par defaut le CPSR est 0x60000150
	process_pcb->cpsr = 0x60000150;
	//Par defaut le processus est dans l'état READY.
	process_pcb->state = READY;
	//On initialise le code de retour.
	process_pcb->returnCode = -1;
	//Calcul du poids en fonction de la niceness
    process_pcb->weight = niceness_to_weight(niceness);
    total_weight += process_pcb->weight;
    //On definit le parent de ce processus.
    process_pcb->parent_process = current_process;
	//On insere la PCB dans la liste circulaire du round-robin, juste apres current_process.
	process_pcb->next_process = current_process->next_process;
	process_pcb->previous_process = current_process;
	current_process->next_process->previous_process = process_pcb;
	current_process->next_process = process_pcb;
	//On retourne la pcb initialisee.
	return process_pcb;
}

struct pcb_s* fork_current_process(int* pile)
{
	//On sauvegarde le contexte du processus courant.
	save_context(pile);
	//Allocation dynamique d'un struct pcb_s pour le nouveau processus.
	struct pcb_s* child_pcb = (struct pcb_s*)kAlloc(sizeof(struct pcb_s));
	//On copie la pcb du processus courant dans celle de l'enfant.
	for (uint32_t i = 0;i < 13;i++)
	{
		child_pcb->registers[i] = current_process->registers[i];
	}
	child_pcb->lr_user = current_process->lr_user;
	child_pcb->lr_svc = current_process->lr_svc;
	child_pcb->debut_sp = current_process->debut_sp;
	child_pcb->cpsr = current_process->cpsr;
	child_pcb->weight = current_process->weight;
	//On initialise d'autres champs.
	child_pcb->state = READY;
	child_pcb->returnCode = -1;
	total_weight += child_pcb->weight;
	child_pcb->parent_process = current_process;
	child_pcb->page_table = init_process_translation_table();
	//On insere la PCB dans la liste circulaire du round-robin, juste apres current_process.
	child_pcb->next_process = current_process->next_process;
	child_pcb->previous_process = current_process;
	current_process->next_process->previous_process = child_pcb;
	current_process->next_process = child_pcb;
	//On alloue une pile.
	child_pcb->debut_sp = vmem_alloc_for_userland(child_pcb->page_table, PROCESS_STACK_SIZE, UINT32_MAX, DOWN) + PROCESS_STACK_SIZE;
	child_pcb->sp = current_process->sp;
	//On copie le contenu de la pile.
	uint32_t stack_first_page = ((uint32_t)child_pcb->debut_sp - PROCESS_STACK_SIZE)/PAGE_SIZE;
	uint32_t stack_page_size = PROCESS_STACK_SIZE / PAGE_SIZE;
	for (uint32_t page = 0;page < stack_page_size;page++)
	{
		uint32_t address = (stack_first_page + page)*PAGE_SIZE;
		//On regarde le numero de frame pour chaque processus.
		uint32_t frame_current_process = vmem_translate(address, current_process->page_table) / PAGE_SIZE;
		uint32_t frame_child_process = vmem_translate(address, child_pcb->page_table) / PAGE_SIZE;
		//On copie le contenu d'une frame dans l'autre.
		vmem_copy_frame(frame_child_process, frame_current_process);
	}

	//On alloue le tas.
	uint32_t h_size = heap_size(current_process->heap);
	uint32_t h_address = (uint32_t)current_process->heap->address;
	if (h_size > 0)
	{
		vmem_alloc_for_userland(child_pcb->page_table, h_size, h_address, UP);
	}

	//TODO: Copier les frames du tas, puis copier les MemoryBlock.

	//Pour le processus enfant, on retourne 0 comme pcb.
	child_pcb->registers[0] = 0;

	return child_pcb;
}

void __attribute__((naked)) start_current_process()
{
	//On saute vers la fonction principale du processus.
	__asm volatile("blx lr");
	//Le retour du processus est contenu dans le registre R0, et est repassé à la fonction sys_exit.
	__asm volatile("b sys_exit");
}

void free_process(struct pcb_s* process)
{
	//On supprime la PCB de la liste circulaire.
	process->next_process->previous_process = process->previous_process;
	process->previous_process->next_process = process->next_process;
	
	//On libere la pcb de ce processus.
	kFree((void*)(process), sizeof(struct pcb_s));
}

void save_context(int* pile)
{
	int i;
	//--------------------------------------------------------Sauvegarde
	//On sauvegarde les registres de la pile courante dans current_process.
	for (i = 0;i < 13;i++)
	{
		current_process->registers[i] = pile[i];
	}
	//On sauvegarde le registre lr_svc
	current_process->lr_svc = (func_t*)pile[13];
	//On sauvegarde les registres lr_user et sp_user.
	//Pour acceder au lr_user et au sp_user depuis le mode SVC, on doit passer en mode system.
	//On met une bonne fois pour toute l'adresse de current_process dans R0.
	__asm("mov r1, %0" : : "r"(current_process));
	//On passe en mode SYSTEM.
	__asm("cps 0x1f");
	//On sauvegarde le lr_user du processus courant.
	__asm("str lr, [r1, %0]" : : "I"(PCB_OFFSET_LR_USER) : "r1");
	//On sauvegarde le sp du processus courant.
	__asm("str sp, [r1, %0]" : : "I"(PCB_OFFSET_SP) : "r1");
	//Pour on revient en mode SVC.
	__asm("cps 0x13");
	//On sauvegarde le SPSR.
	__asm("mrs r2, spsr");
	__asm("str r2, [r1, %0]" : : "I"(PCB_OFFSET_CPSR) : "r1");
}

void restore_context(int* pile)
{
	int i;
	//------------------------------------------------------Restauration
	//On ecrase la pile de swi_handler par la pile du nouveau processus.
	for (i = 0;i < 13;i++)
	{
		pile[i] = current_process->registers[i];
	}
	//On restaure le registre lr_svc
	pile[13] = (int)current_process->lr_svc;
	//On restaure les registres lr_user et sp_user.
	//Pour acceder au lr_user et au sp_user depuis le mode SVC, on doit passer en mode system.
	//On met une bonne fois pour toute l'adresse de current_process dans R0.
	__asm("mov r0, %0" : : "r"(current_process));
	//On passe en mode SYSTEM.
	__asm("cps 0x1f");
	//On restaure le lr_user.
	__asm("ldr	lr, [r0, %0]" : : "I"(PCB_OFFSET_LR_USER) : "r0");
	//On restaure le sp du processus destination.
	__asm("ldr	sp, [r0, %0]" : : "I"(PCB_OFFSET_SP) : "r0");
	//On revient en mode SVC.
	__asm("cps 0x13");
	//On restaure le SPSR.
	__asm("ldr r2, [r0, %0]" : : "I"(PCB_OFFSET_CPSR) : "r0");
	__asm("msr spsr, r2");
}

void __attribute__((naked)) irq_handler()
{
	int* pile;
	DISABLE_IRQ();
	//Sauvegarde du contexte d'execution.
	__asm("stmfd sp!, {r0-r12, lr}");
	//Memorisation du sommet de pile pour lecture des parametres.
	__asm("mov %0, sp" : "=r"(pile));
	
	//On passe sur la table de traduction du noyau.
	load_kernel_page_table();
	
	//On retire 4 au lr pour eviter de sauter une instruction au retour.
	pile[13] -= 4;
	//On passe en mode SVC pour le changement de contexte.
	__asm("cps 0x13");
	//On change le processus en cours d'execution.
	yield(pile);
	//On revient en mode IRQ.
	__asm("cps 0x12");
	
	//On repasse sur la table des pages du processus suivant.
	load_page_table(current_process->page_table);
	
	//On rearme le timer.
	ENABLE_TIMER_IRQ();
	
	//Restauration du contexte d'execution et retour .
	__asm("ldmfd sp!, {r0-r12, pc}^");
}

uint32_t* get_current_process_page_table()
{
	return current_process->page_table;
}

MemoryBlock* get_current_process_heap()
{
	return current_process->heap;
}

uint32_t niceness_to_weight(int niceness)
{
    return (21-niceness);
}

uint32_t weight_to_timeslice(uint32_t weight)
{
    uint32_t time = (uint32_t)divide((weight * TIME_SLICE),total_weight);
    if (time == 0)
    {
		time = 1;
	}
    return time;
}
