#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int prio_padre = 0;
	int prio_hijo = 0;
	cprintf("Soy el proceso padre %d y mi prioridad es: %d \n",
	        thisenv->env_id,
	        thisenv->priority);
	prio_padre = thisenv->priority;
	int id;
	if ((id = fork()) < 0)
		panic("fork: %e", id);
	if (id == 0) {
		prio_hijo = thisenv->priority;
		cprintf("Soy el proceso hijo %d, mi padre es %d y mi priority "
		        "es: %d \n",
		        thisenv->env_id,
		        thisenv->env_id,
		        thisenv->priority);
		return;
	}
	cprintf("priority hijo es: %d, priority padre es: %d\n",
	        prio_hijo,
	        prio_padre);
}