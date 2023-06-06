#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t who;
	int i;

	// fork a child process
	who = fork();

	if (who)
		sys_yield();

	// print a message and yield to the other a few times
	for (i = 0; i < (who ? 10 : 20); i++) {
		cprintf("%d: I am the %s!\n", i, who ? "parent" : "child");
		sys_yield();
	}

	// int i, id;

	// // fork the first prime process in the chain
	// envid_t parent = sys_getenvid();
	// envid_t hijo;
	// int priority = sys_get_process_priority(parent);
	// cprintf("La PRIORIDAD de padre es: %d su PID %d \n",
	//         priority,
	//         sys_getenvid());

	// // cprintf("La NUEVA PRIORIDAD de padre es: %d\n",
	// //         sys_get_process_priority(parent));

	// if ((id = fork()) < 0)
	// 	panic("fork: %e", id);
	// if (id == 0) {
	// 	hijo = sys_getenvid();
	// 	int priorityHijo = sys_get_process_priority(hijo);
	// 	cprintf("La PRIORIDAD PRIMER HIJO es: %d su PID %d \n",
	// 	        priorityHijo,
	// 	        sys_getenvid());
	// 	int id2;
	// 	if ((id2 = fork()) < 0)
	// 		panic("fork: %e", id2);
	// 	if (id2 == 0) {
	// 		envid_t nieto = sys_getenvid();
	// 		int prioritynieto = sys_get_process_priority(nieto);
	// 		cprintf("La PRIORIDAD NIETO: %d su PID %d \n",
	// 		        prioritynieto,
	// 		        sys_getenvid());
	// 	}
	// }
}