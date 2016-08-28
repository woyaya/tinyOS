#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include "lock_share.h"
#include "list.h"
#ifdef __cplusplus
 extern "C" {
#endif 

typedef enum{
	LOCKED = __LOCKED,
	UNLOCKED = __UNLOCKED,
}lock_e __attribute__((aligned (4)));
typedef lock_e spinlock_t;

typedef struct{
	lock_e lock;
	struct list_head list;
}mutex_t __attribute__((aligned (4)));

typedef struct{
	int count;
#ifdef CONFIG_OS_DEBUG
	int range;		//
#endif
	struct list_head list;
}semaphore_t __attribute__((aligned (4)));

#ifdef CONFIG_HAVE_SPINLOCK
/**
 * @brief  initial spinlock
 *
 * @param  lock      spinlock handler
 */
extern void spin_lock_init(spinlock_t *lock);
/**
 * @brief  lock spinlock
 *
 * @param  lock      spinlock handler
 */
extern void spin_lock(spinlock_t *lock);
/**
 * @brief  unlock spinlock
 *
 * @param  lock      spinlock handler
 */
extern void spin_unlock(spinlock_t *lock);
/**
 * @brief  lock spinlock and save IRQ-enable state
 *
 * @param  lock      spinlock handler
 * @return IRQ-enable state
 */
extern uint32_t spin_lock_irqsave(spinlock_t *lock);
/**
 * @brief  unlock spinlock and set IRQ-enable state
 *
 * @param  lock      spinlock handler
 * @param  flag      IRQ-enable state, which returned by function spin_lock_irqsave()
 */
extern void spin_unlock_irqrestore(spinlock_t *lock, uint32_t flag);
#endif
#ifdef CONFIG_HAVE_MUTEX
/**
 * @brief  initial mutex
 *
 * @param  mutex      mutex handler
 */
extern void mutex_init(mutex_t *mutex);
/**
 * @brief  lock mutex
 *
 * @param  mutex      mutex handler
 */
extern void mutex_lock(mutex_t *mutex);
/**
 * @brief  unlock mutex
 *
 * @param  mutex      mutex handler
 */
extern void mutex_unlock(mutex_t *mutex);
#endif
#ifdef CONFIG_HAVE_SEMAPHORE
/**
 * @brief  initial semaphore
 *
 * @param  semaphore      semaphore handler
 * @param  count          initial value of semaphore
 */
extern void semaphore_init(semaphore_t *sem, int count);
/**
 * @brief  increase semaphore count
 *
 * @param  sem      semaphore handler
 */
extern void semaphore_inc(semaphore_t *sem);
/**
 * @brief  decrease semaphore count
 *
 * @param  sem      semaphore handler
 *
 * If count reached 0 before decrease it, task will block until 
 * other task/ISR increase it(by call semaphore_inc())
 */
extern void semaphore_dec(semaphore_t *sem);
#endif

#ifdef __cplusplus
}
#endif

#endif
