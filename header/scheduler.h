#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "stm32f10x.h"
#include "platform.h"
#include "list.h"

#ifdef __cplusplus
 extern "C" {
#endif 

typedef void (*task_func_t)(void *);
typedef enum{
	TASK_READY,	//task ready
	TASK_WAITING,	//waiting timeout or event
	TASK_DEAD,	//task dead
}task_state_t;
typedef struct{
	stack_frame_t *frame;
	void *sp;
	uint32_t priority;
	task_func_t task;
	struct list_head list;
	struct list_head time;
	struct list_head event;
}task_t;
#if 0
static inline void __attribute__((always_inline)) scheduler_start(void *psp)
{
	__set_PSP((uint32_t)psp);
	//switch to thread & privilege mode
	__set_CONTROL((1<<1) | (0<<0));
	__ISB();
	__DSB();
	/* raise scheduler */
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
	__DSB();
}
#endif 
extern void *next_task(void *curr_sp);
extern void *scheduler_initial(void);
extern void __scheduler(void);
extern void scheduler_start(void *);
extern void scheduler_pause(void);
extern void scheduler_continue(void);
extern void task_creat(task_t *task, void *stack, uint32_t stack_size, 
		uint32_t priority,
		task_func_t func, void *param);

#ifdef __cplusplus
}
#endif

#endif
