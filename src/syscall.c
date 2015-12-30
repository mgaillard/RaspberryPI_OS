#include "syscall.h"
#include "util.h"
#include "hw.h"
#include "kheap.h"
#include "asm_tools.h"
#include "vmem.h"

//------------------------------------------------------Fonction privées
void swi_handler();
void do_sys_reboot();
void do_sys_nop();
void do_sys_settime(int* pile);
void do_sys_gettime(int* pile);
void do_sys_free_process(int* pile);
void do_sys_create_process(int* pile);
void do_sys_process_state(int* pile);
void do_sys_process_return_code(int* pile);
void do_sys_malloc(int* pile);
void do_sys_free(int* pile);

//-----------------------------------------------------------Réalisation

void sys_reboot()
{
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #1");
	//On fait une interruption logicielle.
	__asm("swi #0");
}

void sys_nop()
{
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #2");
	//On fait une interruption logicielle.
	__asm("swi #0");
}

void sys_settime(uint64_t date_ms)
{
	//On met les parametres dans les registres.
	//Un uint64_t occupe deux registres. On met date_ms dans R1 et R2.
	__asm("mov r2,r1" : : : "r0");
	__asm("mov r1,r0" : : : "r2");
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #3" : : : "r1", "r2");
	
	//On fait une interruption logicielle.
	__asm("swi #0");
}

uint64_t sys_gettime()
{
	uint32_t date_lowbits, date_highbits;
	uint64_t date_ms;
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #4");
	//On fait une interruption logicielle.
	__asm("swi #0");
	//On recupère le résultat de l'appel système depuis les registres R0 et R1.
	__asm("mov %0, r0" : "=r"(date_lowbits) : : "r1");
	__asm("mov %0, r1" : "=r"(date_highbits));
	//On transforme les deux entiers de 32bits en un entier de 64 bits.
	date_ms = date_highbits;
	date_ms = date_ms << 32;
	date_ms += date_lowbits;

	return date_ms;
}

void sys_yieldto(struct pcb_s* dest)
{
	//On met le parametre dans le registre R1.
	__asm("mov r1,r0");
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #5" : : : "r1");
	//On fait une interruption logicielle.
	__asm("swi #0");
}

void sys_yield()
{
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #6" : : : "r1");
	//On fait une interruption logicielle.
	__asm("swi #0");
}

void sys_exit(int status)
{
	//On met le parametre dans le registre R1.
	__asm("mov r1,r0");
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #7" : : : "r1");
	//On fait une interruption logicielle.
	__asm("swi #0");
}

int sys_wait(struct pcb_s* dest)
{
	int status = -1;
	//Tant que le code de retour est -1, le processus n'est pas termine.
	while(sys_process_state(dest) != TERMINATED)
	{
		//On laisse notre tour, le temps d'attendre 
		sys_yield();
	}
	//On retient le code de retour du processus.
	status = sys_process_return_code(dest);
	//On supprime proprement la memoire du processus.
	//On met le parametre dans le registre R1.
	__asm("mov r1, %0" : : "r"(dest));
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #8" : : : "r1");
	//On fait une interruption logicielle.
	__asm("swi #0");
	
	return status;
}

struct pcb_s* sys_create_process(func_t* entry)
{
	struct pcb_s* process;
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #9");
	//Le parametre est dans le registre R1.
	__asm("mov r1, %0" : : "r"(entry) : "r0");
	//On fait une interruption logicielle.
	__asm("swi #0");
	//On recupère le résultat de l'appel système depuis le registre R0.
	__asm("mov %0, r0" : "=r"(process));

	return process;
}

ProcessState sys_process_state(struct pcb_s* process)
{
	ProcessState state;
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #10");
	//Le parametre est dans le registre R1.
	__asm("mov r1, %0" : : "r"(process) : "r0");
	//On fait une interruption logicielle.
	__asm("swi #0");
	//On recupère le résultat de l'appel système depuis le registre R0.
	__asm("mov %0, r0" : "=r"(state));

	return state;
}

int sys_process_return_code(struct pcb_s* process)
{
	int returnCode;
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #11");
	//Le parametre est dans le registre R1.
	__asm("mov r1, %0" : : "r"(process) : "r0");
	//On fait une interruption logicielle.
	__asm("swi #0");
	//On recupère le résultat de l'appel système depuis le registre R0.
	__asm("mov %0, r0" : "=r"(returnCode));

	return returnCode;
}

void* sys_malloc(uint32_t size)
{
	void* address;
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #12");
	//Le parametre est dans le registre R1.
	__asm("mov r1, %0" : : "r"(size) : "r0");
	//On fait une interruption logicielle.
	__asm("swi #0");
	//On recupère le résultat de l'appel système depuis le registre R0.
	__asm("mov %0, r0" : "=r"(address));

	return address;
}

void sys_free(void* address)
{
	//On donne le numero d'appel système dans R0.
	__asm("mov r0, #13");
	//Le parametre est dans le registre R1.
	__asm("mov r1, %0" : : "r"(address) : "r0");
	//On fait une interruption logicielle.
	__asm("swi #0");
}

void __attribute__((naked)) swi_handler()
{
	int numeroAppelSysteme;
	int* pile;
	//On desactive les interruptions pour ne pas être interrompu.
	DISABLE_IRQ();
	//Sauvegarde du contexte d'execution.
	__asm("stmfd sp!, {r0-r12, lr}");
	//Memorisation du sommet de pile pour lecture des parametres.
	__asm("mov %0, sp" : "=r"(pile) : : "r0");
	//Lecture du numero d'appel systeme dans R0.
	__asm("mov %0, r0" : "=r"(numeroAppelSysteme) : : "r0");
	
	//On passe sur la table de traduction du noyau.
	load_kernel_page_table();
	
	switch(numeroAppelSysteme)
	{
		case 1:
			do_sys_reboot();
			break;
		case 2:
			do_sys_nop();
			break;
		case 3:
			do_sys_settime(pile);
			break;
		case 4:
			do_sys_gettime(pile);
			break;
		case 5:
			yieldto(pile);
			break;
		case 6:
			yield(pile);
			break;
		case 7:
			exit_process(pile);
			break;
		case 8:
			do_sys_free_process(pile);
			break;
		case 9:
			do_sys_create_process(pile);
			break;
		case 10:
			do_sys_process_state(pile);
			break;
		case 11:
			do_sys_process_return_code(pile);
			break;
		case 12:
			do_sys_malloc(pile);
			break;
		case 13:
			do_sys_free(pile);
			break;
		default:
			//L'appel système demande n'est pas connu.
			PANIC();
			break;
	}
	//On repasse sur la table des pages du processus.
	load_page_table(get_current_process_page_table());
	
	//On reactive les interruptions.
	ENABLE_IRQ();
	
	//Restauration du contexte d'execution et retour .
	__asm("ldmfd sp!, {r0-r12, pc}^");
}

void do_sys_reboot()
{
	#if RPI
		const int PM_RSTC = 0x2010001c;
		const int PM_WDOG = 0x20100024;
		const int PM_PASSWORD = 0x5a000000;
		const int PM_RSTC_WRCFG_FULL_RESET = 0x00000020;
		Set32(PM_WDOG, PM_PASSWORD | 1);
		Set32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);
		while(1);
	#else
		__asm("b 0x8000");
	#endif
}

void do_sys_nop()
{
	//Ne fait rien.
}

void do_sys_settime(int* pile)
{
	//on recupere la date dans la pile de swi_handler.
	uint32_t date_lowbits = pile[1];
    uint32_t date_highbits = pile[2];
    uint64_t date_ms = date_highbits;
    //On décale les bits de date_highbits vers la gauche pour qu'ils occupent bien les bits de poids fort.
    date_ms = date_ms << 32;
    date_ms += date_lowbits;
    //On appelle set_date_ms de la librairie hw.
    set_date_ms(date_ms);
}

void do_sys_gettime(int* pile)
{
	//On recupère la date depuis la librairie hw.
	uint64_t date_ms = get_date_ms();
	//On separe la date en deux entiers de 32bits.
	//On met les bits de poids faibles dans la pile de swi_handler dans la case de R0.
	//On met les bits de poids forts dans la pile de swi_handler dans la case de R1.
	pile[0] = date_ms & (0x0FFFFFFFF);
	pile[1] = date_ms >> 32;
}

void do_sys_free_process(int* pile)
{
	free_process((struct pcb_s*)pile[1]);
}

void do_sys_create_process(int* pile)
{
	func_t* entry;
	struct pcb_s* process;
	//On recupere les arguments depuis la pile.
	entry = (func_t*)pile[1];
	process = create_process(entry);
	//On retourne la PCB par le registre R0 de la pile.
	pile[0] = (int)process;
}

void do_sys_process_state(int* pile)
{
	struct pcb_s* process = (struct pcb_s*)pile[1];
	//On retourne l'etat par le registre R0 de la pile.
	pile[0] = (int)process->state;
}

void do_sys_process_return_code(int* pile)
{
	struct pcb_s* process = (struct pcb_s*)pile[1];
	//On retourne le code par le registre R0 de la pile.
	pile[0] = (int)process->returnCode;
}

void do_sys_malloc(int* pile)
{
	uint8_t* address;
	uint32_t size = (uint32_t)pile[1];
	//On recupère le tas et la table des pages du processus courant.
	MemoryBlock* process_heap = get_current_process_heap();
	uint32_t* process_page_table = get_current_process_page_table();
	//On alloue le bloc pour le processus courant.
	address = heap_alloc(process_heap, process_page_table, size);
	//On retourne l'adresse du bloc alloué.
	pile[0] = (int)address;
}

void do_sys_free(int* pile)
{
	void* address = (void*)pile[1];
	//On recupère le tas du processus courant.
	MemoryBlock* process_heap = get_current_process_heap();
	//On libère ce bloc.
	heap_free(process_heap, address);
}