#include "stm32f10x.h"
#include "semaphore.h"
#include "scheduler.h"

void spin_lock_init(spinlock_t *lock)
{
	*lock = UNLOCKED;
}
void mutex_init(mutex_t *mutex)
{
	mutex->lock = UNLOCKED;
	INIT_LIST_HEAD(&mutex->list);
}
void semaphore_init(semaphore_t *sem, int count)
{
	sem->count = count;
	INIT_LIST_HEAD(&sem->list);
}

#if 0
#pragma push
#pragma O2
static void spin_lock(spinlock_t *lock)
{
#if 0
	__asm__ volatile(
		"ldr r0, %0\n"
		"ldr r1, $LOCKED\n"
	"retry_lock:\n"
		"ldrex r2, [r0]\n"
		"cmp r2, $LOCKED\n"
		"itt ne\n"
		"strexne r2, r1, [r0]\n"
		"cmpne r2, $0\n"
		"bne retry_lock\n"
		:
		:"m"(lock)
		:"cc","r0","r1","r2"
	);
#else
	spinlock_t val;
	while(1)
	{
		val = (spinlock_t)__LDREXW(lock);
		if (UNLOCKED == val)
		{
			int ret = __STREXW(LOCKED, lock);
			if (0 == ret)	//success
				break;
		}
	}
#endif
}

static void spin_unlock(spinlock_t *lock)
{
	int ret;
	spinlock_t val;
	while(1)
	{
		val = (spinlock_t)__LDREXW(lock);
		ret = __STREXW(UNLOCKED, lock);
		if (0 == ret)	//success
			break;
	}
}
#pragma pop
#endif


void tesss(void)
{
	spinlock_t lock;
	semaphore_t sem;
	spin_lock_init(&lock);
	semaphore_init(&sem, 2);
	semaphore_dec(&sem);
	semaphore_dec(&sem);
	semaphore_inc(&sem);
	semaphore_inc(&sem);
	semaphore_dec(&sem);
	semaphore_dec(&sem);
//	semaphore_dec(&sem);
}
