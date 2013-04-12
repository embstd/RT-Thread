/*
 * File      : startup.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-08-31     Bernard      first implementation
 */

#include <rthw.h>
#include <rtthread.h>

#include "stm32f10x.h"
#include "board.h"
#include "rtc.h"

/**
 * @addtogroup STM32
 */

/*@{*/

extern int  rt_application_init(void);
#ifdef RT_USING_FINSH
extern void finsh_system_init(void);
extern void finsh_set_device(const char* device);
#endif

extern rt_err_t rt_i2c_stm32_add_bus(void);

#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Limit;
#elif __ICCARM__
#pragma section="HEAP"
#else
extern int __bss_end;
#endif

/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(u8* file, u32 line)
{
	rt_kprintf("\n\r Wrong parameter value detected on\r\n");
	rt_kprintf("       file  %s\r\n", file);
	rt_kprintf("       line  %d\r\n", line);

	while (1) ;
}

u32 ChipUniqueID[3];
u16 ChipFlashSize;
/*
 * 函数名：Get_ChipInfo
 * 描述  ：获取芯片ID,Flash size
 * 输入  ：无
 * 输出  ：无
 */
void Get_ChipInfo(void)
{
	ChipUniqueID[0] = *(__IO u32 *)(0X1FFFF7F0); // 高字节
	ChipUniqueID[1] = *(__IO u32 *)(0X1FFFF7EC); // 
	ChipUniqueID[2] = *(__IO u32 *)(0X1FFFF7E8); // 低字节

	ChipFlashSize   = *(__IO u16 *)(0X1FFFF7E0);
}	

/**
 * This function will startup RT-Thread RTOS.
 */
void rtthread_startup(void)
{
	int i=0;
	/* init board */
	rt_hw_board_init();

	/* show version */
	rt_show_version();

	Get_ChipInfo();
	rt_kprintf("Device DevID=%X-%X-%X\n, Flash Size=%dK\n",
			ChipUniqueID[0],ChipUniqueID[1],ChipUniqueID[2],ChipFlashSize);
	/* init tick */
	rt_system_tick_init();

	/* init kernel object */
	rt_system_object_init();

	/* init timer system */
	rt_system_timer_init();

#ifdef RT_USING_HEAP
#if STM32_EXT_SRAM

	rt_system_heap_init((void*)STM32_EXT_SRAM_BEGIN, (void*)STM32_EXT_SRAM_END);
#else
	#ifdef __CC_ARM

		rt_system_heap_init((void*)&Image$$RW_IRAM1$$ZI$$Limit, (void*)STM32_SRAM_END);
	#elif __ICCARM__

	    rt_system_heap_init(__segment_end("HEAP"), (void*)STM32_SRAM_END);
	#else

		/* init memory system */
		rt_system_heap_init((void*)&__bss_end, (void*)STM32_SRAM_END);
	#endif

#endif
#endif
	/* init scheduler system */
	rt_system_scheduler_init();

#ifdef RT_USING_DFS
	/* init sdcard driver */
#if STM32_USE_SDIO
	rt_hw_sdcard_init();
#else
	rt_hw_msd_init();
#endif
#endif

	rt_kprintf("==>%d",i++);
    rt_hw_rtc_init();
	//rt_i2c_stm32_add_bus();
	//rt_kprintf("==>add I2C bus %d\n",i++);
	/* init all device */
	rt_device_init_all();

	/* init application */
	rt_application_init();
	rt_kprintf("app ==>%d",i++);
#ifdef RT_USING_FINSH
	/* init finsh */
	finsh_system_init();
		rt_kprintf("finsh==>%d",i++);
	finsh_set_device("uart1");
#endif
	rt_kprintf("==>%d",i++);
    /* init timer thread */
    rt_system_timer_thread_init();

	/* init idle thread */
	rt_thread_idle_init();
	rt_kprintf("==>%d",i++);
	/* start scheduler */
	rt_system_scheduler_start();
	rt_kprintf("end ==>%d",i++);
	/* never reach here */
	return ;
}

int main(void)
{
	/* disable interrupt first */
	rt_hw_interrupt_disable();

	/* startup RT-Thread RTOS */
	rtthread_startup();

	return 0;
}

/*@}*/
