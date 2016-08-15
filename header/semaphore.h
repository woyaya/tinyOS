#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include "spinlock.h"
#ifdef __cplusplus
 extern "C" {
#endif 

typedef enum{
	SPINLOCK_LOCKED = __SPIN_LOCKED,
	SPINLOCK_UNLOCKED = __SPIN_UNLOCKED,
}spinlock_t __attribute__((aligned (4)));
typedef enum{
	MUTEX_LOCKED = __SPIN_LOCKED,
	MUTEX_UNLOCKED = __SPIN_UNLOCKED,
}mutex_t __attribute__((aligned (4)));
typedef struct{
	int count;
}semaphore_t;

extern void spin_lock_init(spinlock_t *lock);
extern void spin_lock(spinlock_t *lock);
extern void spin_unlock(spinlock_t *lock);
extern uint32_t spin_lock_irqsave(spinlock_t *lock);
extern void spin_unlock_irqrestore(spinlock_t *lock, uint32_t flag);

extern void mutex_init(mutex_t *mutex);
extern void mutex_lock(mutex_t *mutex);
extern void mutex_unlock(mutex_t *mutex);

extern void semaphore_init(semaphore_t *sem, int count);
extern void semaphore_inc(semaphore_t *sem);	//function semaphore_inc() do not check real semaphore range
extern void semaphore_dec(semaphore_t *sem);


#ifdef __cplusplus
}
#endif

#endif
