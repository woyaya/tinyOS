#include "stm32f10x.h"
#include "config.h"
#include "scheduler.h"
#include "semaphore.h"

static mutex_t lock; 

static void task_test(void *arg)
{
	int index = (int)arg;
	while(1)
	{
		mutex_lock(&lock);
		msleep(10*index);
		mutex_unlock(&lock);
		msleep(30*index);
	}
}


int main()
{
	void *p;
	int count = 0;
	task_t task1;
	task_t task2;
	mutex_init(&lock);
	
	p = os_initial();
	os_start(p);
	
	task_creat(&task1, (void*)TASK_STACK_BASE, TASK_STACK_SIZE_MIN, 10, task_test, (void*)1);
	task_creat(&task2, (void*)(TASK_STACK_BASE-TASK_STACK_SIZE_MIN), 
		TASK_STACK_SIZE_MIN, 10, task_test, (void*)2);
	
	
	while(1)
	{
		//__WFE();
		if (++count & 0x7)
			__NOP();
		else
		{
			scheduler(TASK_READY);
			
		}
		__NOP();
		if (0x20 == count)
			sleep(1);

		scheduler_pause();
		scheduler_continue();


		scheduler_pause();
		scheduler_pause();
		scheduler_continue();
		scheduler_continue();
	}
	
	return 0;
}
