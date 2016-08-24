#include "stm32f10x.h"
#include "scheduler.h"




int main()
{
	void *p;
	int count = 0;
	
	p = scheduler_initial();
	scheduler_start(p);
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
		
	}
	
	return 0;
}
