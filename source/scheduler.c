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
static struct list_head task_sleep;
/***********************************************/
//static volatile uint64_t jiffies_64 = 0;
static volatile uint32_t jiffies = 0;	//0xffffffff / 1000 / (60 * 60 * 24) = 49.7 days

void SysTick_Handler(void)
{
	jiffies++;

	__scheduler();
}
static uint32_t systick_init(void)
{
	INIT_LIST_HEAD(&task_sleep);
	NVIC_SetPriority(SysTick_IRQn, CONFIG_SYSTICK_PRIORITY);
	return SysTick_Config(SystemCoreClock / (1000/CONFIG_SYSTICK_PERIOD));
}

/***********************************************/
static void task_delete(void)
{
	mutex_lock(&lock);
	scheduler_pause();
	list_del_init(&current->list);
	list_del_init(&current->time);
	list_del_init(&current->event);
	scheduler_continue();
	mutex_unlock(&lock);
	__scheduler();
}

static void stack_frame_initial(stack_frame_t *frame)
{
	memset(frame, 0, sizeof(*frame));
	frame->xPSR = DEFAULT_PSR;
	frame->PC = 0;
	frame->LR = (uint32_t)task_delete;
}
//rise PendSV interrupt for scheduler
void idle(void *param)
{
	int count = 0;
	while(1)
	{
		//__WFE();
		if (++count & 0x7)
			__NOP();
		else
		{
			__scheduler();
		}
		__WFE();
		__NOP();
	}
	
	
}
void scheduler(task_state_t new_state)
{
	mutex_lock(&lock);
	scheduler_pause();
	switch(new_state)
	{
		case TASK_READY:	//task ready, add it to the ready list
		
		case TASK_WAITING:	//waiting timeout or event
			
		case TASK_DEAD:
		
		default:
			
		break;
	}
	scheduler_continue();
	mutex_unlock(&lock);
	__scheduler();
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
void task_creat(task_t *task, void *stack, uint32_t stack_size, 
		uint32_t priority,
		task_func_t func, void *param)
{
	task_t *tmp = NULL;
	struct list_head *list;
	task->frame = (stack_frame_t*)((uint32_t)stack/* + stack_size*/) - 1;
	stack_frame_initial(task->frame);
	task->frame->R0 = (uint32_t)param;
	task->frame->PC = (uint32_t)func;
	task->sp = task->frame;
	task->task = func;
	task->priority = priority;
	INIT_LIST_HEAD(&task->list);
	INIT_LIST_HEAD(&task->time);
	INIT_LIST_HEAD(&task->event);	
	//add to ready list
	mutex_lock(&lock);
	scheduler_pause();
#if 0
	list_for_each(list, &task_ready)
	{
		task_t *tmp = list_entry(list, task_t, list);
		
		
		
		if (tmp->priority > priority)
			break;
	}
	if (NULL == tmp)
		list = &task_ready;
	else
		list = &tmp->list;
	list_add(&task->list, list);
#endif
	scheduler_continue();
	mutex_unlock(&lock);	
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
	//NVIC_EnableIRQ(PendSV_IRQn);
	
	//Initial SysTick
	systick_init();
	
	//create default tasks
	task_creat(&task_idle, (void*)IDLE_STACK_BASE, IDLE_STACK_SIZE, ~0, idle, (void*)1);
	task_creat(&task_main, (void*)MAIN_STACK_BASE, MAIN_STACK_SIZE, 1, (task_func_t)main, (void*)2);
	current = &task_main;
	return  task_main.sp;
}

