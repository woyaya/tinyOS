#include "stm32f10x.h"
#include "config.h"

#include "semaphore.h"
#include "scheduler.h"
#include "common.h"
#ifdef CONFIG_HAVE_SPINLOCK
/**
 * @brief  initial spinlock
 *
 * @param  lock      spinlock handler
 */
void spin_lock_init(spinlock_t *lock)
{
	*lock = UNLOCKED;
}
#endif
#ifdef CONFIG_HAVE_MUTEX
/**
 * @brief  initial mutex
 *
 * @param  mutex      mutex handler
 */
void mutex_init(mutex_t *mutex)
{
	mutex->lock = UNLOCKED;
	INIT_LIST_HEAD(&mutex->list);
}
void __mutex_wakeup(mutex_t *mutex)
{
	scheduler_pause();
	if (likely(!list_empty(&mutex->list)))
	{
		task_t *task = list_entry(mutex->list.next, task_t, event);
		//list_del(&task->event);
		__task_ready(task);
	}	
	scheduler_continue();
}
void __mutex_waiting(mutex_t *mutex, uint32_t delay)
{
	scheduler_pause();
	if (likely(LOCKED == mutex->lock))
		__current_waiting(&mutex->list, delay);
	scheduler_continue();
}
#endif
#ifdef CONFIG_HAVE_SEMAPHORE
/**
 * @brief  initial semaphore
 *
 * @param  semaphore      semaphore handler
 * @param  count          initial value of semaphore
 */
void semaphore_init(semaphore_t *sem, int count)
{
	sem->count = count;
#ifdef CONFIG_OS_DEBUG
	sem->range = count;
#endif
	INIT_LIST_HEAD(&sem->list);
}
void __semaphore_wakeup(semaphore_t *sem)
{
	scheduler_pause();
	if (likely(!list_empty(&sem->list)))
	{
		task_t *task = list_entry(sem->list.next, task_t, event);
		//list_del_init(&task->event);
		__task_ready(task);
	}	
	scheduler_continue();
}
void __semaphore_waiting(semaphore_t *sem, uint32_t delay)
{
	scheduler_pause();
	if (likely(0 == sem->count))
		__current_waiting(&sem->list, delay);
	scheduler_continue();	
}
#endif
