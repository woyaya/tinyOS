#include "stm32f10x.h"
#include "misc.h"
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "scheduler.h"
#include "semaphore.h"
#include "list.h"

static task_t *current = NULL;
static task_t task_idle;
static task_t task_main;
static struct list_head task_ready;
static struct list_head task_waiting;
static mutex_t lock;

static void task_delete(void)
{
	
}

static void stack_frame_initial(stack_frame_t *frame)
{
	memset(frame, 0, sizeof(*frame));
	frame->xPSR = DEFAULT_PSR;
	frame->PC = 0;
	frame->LR = (uint32_t)task_delete;
}
//rise PendSV interrupt for scheduler
void __rise_scheduler(void)
{
//	SCB->SHCSR |= 1<<10;
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
	__DSB();
}
void idle(void *param)
{
	int count = 0;
	while(1)
	{
		//__WFE();
		if (++count & 0x7)
			__NOP();
		else
			__rise_scheduler();
		__NOP();
	}
	
	
}
void scheduler(task_state_t new_state)
{
	switch(new_state)
	{
		case TASK_READY:	//task ready, add it to the ready list
		
		case TASK_WAITING:	//waiting timeout or event
			
		case TASK_DEAD:
		
		default:
			
		break;
	}
	
}

void *next_task(void *curr_sp)
{
static int i = 0;
	current->sp = curr_sp;
	//select a new task
	
	if (i++ & 1)
		current = &task_main;
	else
		current = &task_idle;
	
	return current->sp;
}
void task_creat(task_t *task, void *stack, uint32_t stack_size, task_func_t func, void *param)
{
	task->frame = (stack_frame_t*)((uint32_t)stack/* + stack_size*/) - 1;
	stack_frame_initial(task->frame);
	task->frame->R0 = (uint32_t)param;
	task->frame->PC = (uint32_t)func;
	task->sp = task->frame;
	task->task = func;
	//add to ready list
	
	
}

extern int main(int argc, char *argv[]);
void *scheduler_initial(void)
{
	mutex_init(&lock);
	INIT_LIST_HEAD(&task_ready);
	INIT_LIST_HEAD(&task_waiting);

	//Initial PENDSV
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	/* Configure the preemption priority and subpriority:
		Lower values gives higher priority  */
	NVIC_SetPriority(PendSV_IRQn, CONFIG_PENDSV_PRIORITY);
	//enable PENDSV
//	NVIC_EnableIRQ(PendSV_IRQn);
	
	task_creat(&task_idle, (void*)IDLE_STACK_BASE, IDLE_STACK_SIZE, idle, (void*)1);
	task_creat(&task_main, (void*)MAIN_STACK_BASE, MAIN_STACK_SIZE, (task_func_t)main, (void*)2);
	current = &task_main;
	return  task_main.sp;
}

#if 0
void scheduler_start(void)
{
	
	__rise_scheduler();
	
}
#endif
