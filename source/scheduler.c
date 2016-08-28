#include "stm32f10x.h"
#include "misc.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "config.h"
#include "scheduler.h"
#include "semaphore.h"
#include "list.h"
#include "common.h"

static task_t *current = NULL;
static task_t task_idle;
static task_t task_main;
static struct list_head ready;
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
		tmp = list_entry(list, task_t, time);
		if (tmp->timeout > task->timeout)
			break;
	}
	if (NULL == tmp)
		list = head;
	else
		list = tmp->time.prev;
	list_add(&task->time, list);
}
static void task_sleep(task_t *task, uint32_t delay)
{
	scheduler_pause();
	list_del_init(&task->list);
	task->timeout = jiffies_64 + delay;
	__task_add_by_timeout(&sleeping, task);
	__scheduler();
	scheduler_continue();
}
void __task_ready(task_t *task)
{
	//scheduler_pause();
	list_del(&task->list);
	__task_add_by_priority(&ready, task);
	//scheduler_continue();
}
static inline void task_ready(task_t *task)
{
	scheduler_pause();
	__task_ready(task);
	__scheduler();
	scheduler_continue();
}
void __task_waiting(task_t *task, struct list_head *wait, uint32_t delay)
{
	//scheduler_pause();
	list_del_init(&task->list);
	if (delay != 0)
	{
		task->timeout = jiffies_64 + delay;
		__task_add_by_timeout(&sleeping, task);
	}
	__task_add_by_priority(wait, task);
	//scheduler_continue();
	__scheduler();
}
void __current_waiting(struct list_head *wait, uint32_t delay)
{
	__task_waiting(current, wait, delay);
}
static void task_delete(void)
{
	scheduler_pause();
	list_del_init(&current->list);
	list_del_init(&current->time);
	list_del_init(&current->event);
	__scheduler();
	scheduler_continue();
}
/***********************************************/
/**
 * @brief  take task into sleep mode for "s" second(s)
 *
 * @param  s      sleep time. unit: second
 * @return  none 
 */
void sleep(uint32_t s)
{
	if (unlikely(0 == s))
		return;
	task_sleep(current, s * (1000/CONFIG_SYSTICK_PERIOD));
}
/**
 * @brief  take task into sleep mode for "ms" millisecond(s)
 *
 * @param  ms      sleep time. unit: millisecond
 * @return  none 
 */
void msleep(uint32_t ms)
{
	if (unlikely(0 == ms))
		return;
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
			tmp = list_entry(list, task_t, time);
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
void idle(void *param)
{
	while(1)
	{
		__WFE();
	}
	
	
}
void scheduler(task_state_t new_state,...)
{
	scheduler_pause();
	switch(new_state)
	{
		case TASK_READY:	//task ready, add it to the ready list
			//list_del_init(&current->list);
			//__task_ready(current);
		break;
		case TASK_WAITING:	//waiting timeout or event
		{
			va_list argp;
			struct list_head *dst;
			dst = va_arg(argp, struct list_head *);
			va_end(argp);
			list_del_init(&current->list);
			__task_add_by_priority(dst, current);
		}
		break;
		case TASK_DEAD:
			list_del_init(&current->list);
			list_add_tail(&current->list, &dead);
		break;
		default:
#ifdef CONFIG_OS_DEBUG
			while(1)
				__NOP();
#endif
		break;
	}
	__scheduler();
	scheduler_continue();
}
#pragma push
#pragma O2
/**
 * @brief  choose next task which ready for run
 *
 * @param  curr_sp      stack point of current task
 * @return  stack point of choosed task 
 *
 * Save stack point of current task and
 * choose a new task ready for run
 *
 * Attention:
 * 	This function should be called @ PendSV ISR only
*/
void *__next_task(void *curr_sp)
{
	current->sp = curr_sp;
	//select a new task
//	scheduler_pause();
	if (likely(! list_empty(&ready)))
		current = list_entry(ready.next, task_t, list);
//	scheduler_continue();
	return current->sp;
}
#pragma pop
static inline void stack_frame_initial(stack_frame_t *frame)
{
	memset(frame, 0, sizeof(*frame));
	frame->xPSR = DEFAULT_PSR;
	frame->PC = 0;
	frame->LR = (uint32_t)task_delete;
}
/**
 * @brief  create new task
 *
 * @param  task		struct handler of new task
 * @param  stack	stack address of new task
 * @param  stack_size	stack size
 * @param  priority	task priority, range: [0, 0xffffffff]. Greater value = lower priority
 * @param  func		task function
 * @param  param	param of task function
 * @return  0: success, others: fail
 * 
 * This function create a new task with stack address range [@stack, @stack+@stack_size)
 * The new task priority set by @priority: '0' is the highest priority, and 0xffffffff is lowest
 */
int task_creat(task_t *task, void *stack, uint32_t stack_size, 
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
	return 0;
}

extern int main(int argc, char *argv[]);
/**
 * @brief  Initial OS run enviroment
 *
 * @return  NULL: fail
 *		others: stack address of current task. 
 *
 * Initial OS run enviroment, and the returned stack address
 * will be used for funtion os_start()
 *
 * Attention:
 *	This function may change NVIC priority group setting.
 *	And initial PendSV & SysTick
 */
void *os_initial(void)
{
	INIT_LIST_HEAD(&ready);
	INIT_LIST_HEAD(&sleeping);
	INIT_LIST_HEAD(&dead);

	//Initial PENDSV
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	/* Configure the preemption priority and subpriority:
		Lower values gives higher priority  */
	NVIC_SetPriority(PendSV_IRQn, CONFIG_PENDSV_PRIORITY);
	//enable PENDSV
	//NVIC_EnableIRQ(PendSV_IRQn);
	
	//pause scheduler
	//scheduler will start by function os_start()
	scheduler_pause();
	
	//Initial SysTick
	systick_init();
	
	//create default tasks
	task_creat(&task_idle, (void*)IDLE_STACK_BASE, IDLE_STACK_SIZE, ~0, idle, (void*)1);
	task_creat(&task_main, (void*)MAIN_STACK_BASE, MAIN_STACK_SIZE, 11, (task_func_t)main, (void*)2);
	current = &task_main;
	return  task_main.sp;
}

