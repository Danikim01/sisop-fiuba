#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

void sched_halt(void);
int prioridad = 3;
int scheduler_calls = 0;
// Choose a user environment to run and run it.
void
sched_yield(void)
{
	scheduler_calls++;
	struct Env *idle = curenv;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

#ifdef R_R
	int start_index = 0;

	if (idle != NULL) {
		start_index = ENVX(idle->env_id) + 1;
	}

	int i = start_index;

	while (i < NENV) {
		if (envs[i].env_status == ENV_RUNNABLE) {
			env_run(&envs[i]);
		}
		i++;
	}

	int j = 0;

	while (j < start_index) {
		if (envs[j].env_status == ENV_RUNNABLE) {
			env_run(&envs[j]);
		}
		j++;
	}

#endif

#ifdef C_P
	size_t env_id = 0;

	if (curenv)  // Si no hay curenv, env_id = 0
		env_id = ENVX(curenv->env_id) +
		         1;  // Calculo el index al env_id actual con ENVX(), y tomo el siguiente

	int j = 0;

	int start_index = env_id;
	while (start_index < NENV) {
		if (envs[start_index].env_status == ENV_RUNNABLE &&
		    envs[start_index].priority > j)
			j = envs[start_index].priority;


		if (envs[start_index].env_status == ENV_RUNNABLE &&
		    envs[start_index].priority == prioridad) {
			env_run(&envs[start_index]);
		}
		start_index++;
	}

	int k = 0;
	while (k < start_index) {
		if (envs[k].env_status == ENV_RUNNABLE && envs[k].priority > j)
			j = envs[k].priority;

		if (envs[k].env_status == ENV_RUNNABLE &&
		    envs[k].priority == prioridad) {
			env_run(&envs[k]);
		}
		k++;
	}
	cprintf("La prioridad GLOBAL anterior es: %d\n", prioridad);
	prioridad = j;
	cprintf("La prioridad GLOBAL ahora es: %d\n", prioridad);
#endif

	if (curenv && curenv->env_status == ENV_RUNNING)
		env_run(curenv);

	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Once the scheduler has finishied it's work, print statistics on
	// performance. Your code here

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile("movl $0, %%ebp\n"
	             "movl %0, %%esp\n"
	             "pushl $0\n"
	             "pushl $0\n"
	             "sti\n"
	             "1:\n"
	             "hlt\n"
	             "jmp 1b\n"
	             :
	             : "a"(thiscpu->cpu_ts.ts_esp0));
}
