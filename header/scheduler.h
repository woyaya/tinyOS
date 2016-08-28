#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "stm32f10x.h"
#include "platform.h"
#include "share.h"
#include "list.h"

#ifdef __cplusplus
 extern "C" {
#endif 

typedef void (*task_func_t)(void *);
typedef enum{
	TASK_READY = __TASK_READY,	//task ready
	TASK_WAITING = __TASK_WAITING,	//waiting timeout or event
	TASK_DEAD = __TASK_DEAD,	//task dead
}task_state_t;
typedef struct{
	stack_frame_t *frame;
	void *sp;
	
	task_func_t task;
	
	uint32_t priority;
	uint64_t timeout;
	union{
		struct list_head list;	
		struct list_head event;
	};					//listed by priorty
	
	struct list_head time;			//listed by timeout
	
}task_t;

extern void __scheduler(void);
extern void __task_ready(task_t *task);
extern void __current_waiting(struct list_head *wait, uint32_t delay);
extern void __task_waiting(task_t *task, struct list_head *wait, uint32_t delay);

/**
 * @brief  take task into sleep mode for "s" second(s)
 *
 * @param  s      sleep time. unit: second
 * @return  none 
 */
extern void sleep(uint32_t s);
/**
 * @brief  take task into sleep mode for "ms" millisecond(s)
 *
 * @param  ms      sleep time. unit: millisecond
 * @return  none 
 */
extern void msleep(uint32_t ms);

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
extern void *os_initial(void);
/**
 * @brief  start OS
 *
 * @param  sp      stack address returned by os_initial
 * @return  0: success, others: fail
 *
 * Change run mode from MSP_privilege to PSP_privilege
 * And start OS sceduler
 *
 * Attention:
 *	This funtion should be called by main() only!
 */
extern int os_start(void *sp);

extern void scheduler(task_state_t new_state,...);
/**
 * @brief  pause scheduler
 *
 * Pause scheduler befor access critical regions
 * Nested call is permitted
 */
extern void scheduler_pause(void);
/**
 * @brief  remain scheduler
 *
 * remain scheduler after access critical regions
 * Nested call is permitted
 */
extern void scheduler_continue(void);

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
extern int task_creat(task_t *task, void *stack, uint32_t stack_size, 
		uint32_t priority,
		task_func_t func, void *param);

#ifdef __cplusplus
}
#endif

#endif
