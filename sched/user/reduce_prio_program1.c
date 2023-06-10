#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t envid = sys_getenvid();
	sys_reduce_priority(envid);

	cprintf("Soy reduce priority 1,pid %d - La prioridad es: %d\n",
	        envid,
	        sys_get_process_priority(envid));
}
