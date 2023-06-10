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
envs_execution_rr(int start, int end)
{
	int i = start;

	while (i < end) {
		if (envs[i].env_status == ENV_RUNNABLE) {
			env_run(&envs[i]);
		}
		i++;
	}
}

void
envs_execution_cp(int start_index, int end, int *j)
{
	while (start_index < end) {
		if (envs[start_index].env_status == ENV_RUNNABLE &&
		    envs[start_index].priority > (*j)) {
			(*j) = envs[start_index].priority;
		}

		if (envs[start_index].env_status == ENV_RUNNABLE &&
		    envs[start_index].priority == prioridad) {
			amount_context_switch++;
			env_run(&envs[start_index]);
		}
		start_index++;
	}
}

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
	int start_index = 0;

	if (idle != NULL) {
		start_index = ENVX(idle->env_id) + 1;
	}

#ifdef R_R

	envs_execution_rr(start_index, NENV);
	envs_execution_rr(0, start_index);

#endif

#ifdef C_P
	int j = 0;
	envs_execution_cp(start_index, NENV, &j);
	envs_execution_cp(0, start_index, &j);
	// cprintf("La prioridad GLOBAL anterior es: %d\n", prioridad);
	prioridad = j;
	// cprintf("La prioridad GLOBAL ahora es: %d\n", prioridad);
#endif

	if (curenv && curenv->env_status == ENV_RUNNING &&
	    (cpunum() == curenv->env_cpunum))
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

	cprintf("\nMostrando estadisticas scheduler: \n\n");
	cprintf("Llamadas al scheduler: %d \n", scheduler_calls);
	cprintf("Cantidad de context switches:%d\n", amount_context_switch);


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
