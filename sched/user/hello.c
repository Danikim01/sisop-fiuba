// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", thisenv->env_id);
	cprintf("my pid is %d and mi priority is %d\n",
	        sys_getenvid(),
	        sys_get_process_priority(sys_getenvid()));
}
