#include "stm32f10x.h"
#include "misc.h"
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "scheduler.h"
#include "semaphore.h"
#include "list.h"
#include "common.h"

static task_t *current = NULL;
static task_t task_idle;
static task_t task_main;
static struct list_head ready;
static struct list_head waiting;
static struct list_head sleeping;
static struct list_head dead;
/***********************************************/
static volatile uint64_t jiffies_64 = 0;
//static volatile uint32_t jiffies = 0;	//0xffffffff / 1000 / (60 * 60 * 24) = 49.7 days
/***********************************************/
static void __task_add_by_priority(struct list_head *head, task_t *task)
{
	task_t *tmp = NULL;
	struct list_head *list;
	list_for_each(list, head)
	{
		tmp = list_entry(list, task_t, list);
		if (tmp->priority > task->priority)
			break;
	}
	if (NULL == tmp)
		list = head;
	else
		list = tmp->list.prev;
	list_add(&task->list, list);
}
static void __task_add_by_timeout(struct list_head *head, task_t *task)
{
	task_t *tmp = NULL;
	struct list_head *list;
	list_for_each(list, head)
	{
		tmp = list_entry(list, task_t, list);
		if (tmp->timeout > task->timeout)
			break;
	}
	if (NULL == tmp)
		list = head;
	else
		list = tmp->list.prev;
	list_add(&task->list, list);
}
static void task_sleep(task_t *task, uint32_t delay)
{
	scheduler_pause();
	list_del_init(&task->list);
	task->timeout = jiffies_64 + delay;
	__task_add_by_timeout(&sleeping, task);
	scheduler_continue();
	__scheduler();
}
static void task_ready(task_t *task)
{
	scheduler_pause();
	__task_add_by_priority(&ready, task);
	scheduler_continue();
}
/***********************************************/
void sleep(uint32_t s)
{
	task_sleep(current, s * (1000/CONFIG_SYSTICK_PERIOD));
}
void msleep(uint32_t ms)
{
	task_sleep(current, ms/CONFIG_SYSTICK_PERIOD);
}
/***********************************************/
void SysTick_Handler(void)
{
	jiffies_64++;
	if (unlikely(! list_empty(&sleeping)))
	{
		uint64_t j64 = jiffies_64;
		struct list_head *list;
		struct list_head *n;
		list_for_each_safe(list, n, &sleeping)
		{
			task_t *tmp;
			tmp = list_entry(list, task_t, list);
			if (tmp->timeout <= j64)
			{
				list_del_init(&tmp->time);
				__task_add_by_priority(&ready, tmp);
			}else
				break;
		}
	}
	__scheduler();
}
static uint32_t systick_init(void)
{
	INIT_LIST_HEAD(&sleeping);
	NVIC_SetPriority(SysTick_IRQn, CONFIG_SYSTICK_PRIORITY);
	return SysTick_Config(SystemCoreClock / (1000/CONFIG_SYSTICK_PERIOD));
}

/***********************************************/
static void task_delete(void)
{
	scheduler_pause();
	list_del_init(&current->list);
	list_del_init(&current->time);
	list_del_init(&current->event);
	scheduler_continue();
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
	scheduler_pause();
	switch(new_state)
	{
		case TASK_READY:	//task ready, add it to the ready list
			list_del_init(&current->list);
			task_ready(current);
		break;
		case TASK_WAITING:	//waiting timeout or event
			list_del_init(&current->list);
			list_add_tail(&current->list, &waiting);
		break;
		case TASK_DEAD:
			list_del_init(&current->list);
			list_add_tail(&current->list, &dead);
		break;
		default:
			
		break;
	}
	scheduler_continue();
	__scheduler();
}
//This function should be called @ PendSV ISR only
void *next_task(void *curr_sp)
{
	current->sp = curr_sp;
	//select a new task
//	scheduler_pause();
	if (! list_empty(&ready))
		current = list_entry(ready.next, task_t, list);
//	scheduler_continue();
	return current->sp;
}
void task_creat(task_t *task, void *stack, uint32_t stack_size, 
		uint32_t priority,
		task_func_t func, void *param)
{
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
	task_ready(task);
}

extern int main(int argc, char *argv[]);
void *scheduler_initial(void)
{
	INIT_LIST_HEAD(&ready);
	INIT_LIST_HEAD(&waiting);
	INIT_LIST_HEAD(&sleeping);
	INIT_LIST_HEAD(&dead);

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

