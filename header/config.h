#ifndef __CONFIG_H__
#define __CONFIG_H__

/*****************************************************************************
***********************hardware infor setting*********************************
*****************************************************************************/
#ifndef IRAM_BASE
  #define CONFIG_IRAM_BASE	0x20000000
#else
  #define CONFIG_IRAM_BASE	IRAM_BASE
#endif
#ifndef IRAM_SIZE
  #define CONFIG_IRAM_SIZE	0x00005000
#else
  #define CONFIG_IRAM_SIZE	IRAM_SIZE
#endif
/*****************************************************************************
***********************interrupt infor setting********************************
*****************************************************************************/
//#define CONFIG_NVIC_PRIORITY_GROUP	NVIC_PriorityGroup_4

#define CONFIG_PENDSV_PRIORITY		0x0F	//Max exception priority value
#define CONFIG_SYSTICK_PRIORITY		0x0E
/*The processor does not process any exception 
 *with a priority value greater than or equal to BASEPRI.
 *Any exception handler which may access task list, 
 *should set greater or equal priority then CONFIG_BASE_PRIORITY */
#define CONFIG_BASE_PRIORITY		0x08

#define CONFIG_SYSTICK_PERIOD		10	//mS
/*****************************************************************************
***********************OS component setting***********************************
*****************************************************************************/
#define CONFIG_OS_DEBUG		TRUE

//#define CONFIG_HAVE_SPINLOCK	TRUE
#define CONFIG_HAVE_MUTEX	TRUE
#define CONFIG_HAVE_SEMAPHORE	TRUE
/*****************************************************************************
**************************memory setting**************************************
*****************************************************************************/
#define TASK_STACK_SIZE_MIN	(0x80 * sizeof(uint32_t))

#define ISR_STACK_BASE		(CONFIG_IRAM_BASE+CONFIG_IRAM_SIZE)
#define ISR_STACK_SIZE		TASK_STACK_SIZE_MIN

#define MAIN_STACK_BASE		(ISR_STACK_BASE-ISR_STACK_SIZE)
#define MAIN_STACK_SIZE		TASK_STACK_SIZE_MIN

#define IDLE_STACK_BASE		(MAIN_STACK_BASE - MAIN_STACK_SIZE)
#define IDLE_STACK_SIZE		(0x20 * sizeof(uint32_t))

#define TASK_STACK_BASE		(IDLE_STACK_BASE-IDLE_STACK_SIZE)


#endif
