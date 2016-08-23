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
			__scheduler();
			
		}
		__NOP();
		
		if (count == 0x20)
		{
			__SEV();
			scheduler_pause();
		}
		if (count == 0x41)
			scheduler_continue();
	}
	
	return 0;
}
