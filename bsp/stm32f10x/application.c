
/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <board.h>
#include <rtthread.h>

#ifdef RT_USING_DFS
/* dfs init */
#include <dfs_init.h>
/* dfs filesystem:ELM filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#endif

#ifdef RT_USING_LWIP
#include <lwip/sys.h>
#include <lwip/api.h>
#include <netif/ethernetif.h>
#endif

#ifdef RT_USING_RTGUI
#include <rtgui/rtgui.h>
#include <rtgui/rtgui_server.h>
#include <rtgui/rtgui_system.h>
#include <rtgui/driver.h>
#endif

#include "gpio.h"

ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t app_stack[ 2048 ];


#define LED1_GPIO GPIO_NUM(2,6) //GPIOC_6
#define LED1_ON()  gpio_direction_output(LED1_GPIO, 1)
#define LED1_OFF() gpio_direction_output(LED1_GPIO, 0)

#define LED2_GPIO GPIO_NUM(2,7) //GPIOC_7
#define LED2_ON()  gpio_direction_output(LED2_GPIO, 1)
#define LED2_OFF() gpio_direction_output(LED2_GPIO, 0)

#define KEY1_GPIO GPIO_NUM(4,0) //GPIOE_0
#define GETKEY1()  gpio_direction_input(KEY1_GPIO)
#define KEY2_GPIO GPIO_NUM(4,1) //GPIOE_1
#define GETKEY2()  gpio_direction_input(KEY2_GPIO)

static struct rt_thread app_thread;
static void app_thread_entry(void* parameter)
{
    unsigned int count=0;
    int i;

    while (1)
    { 

    	if (!GETKEY1()) //Key Down
    	{
    		/* code */
    		for(i=3; i>0; i--)
    		{
    			LED1_ON();
    			rt_thread_delay(RT_TICK_PER_SECOND/2);
    			LED1_OFF();
    			rt_thread_delay(RT_TICK_PER_SECOND/2);
    		}
    		LED1_ON();

    		// at24c16b_write(0x0, 128);
    		// at24c16b_read(0x0, 128);

    			if (dfs_mount("W25X10BV", "/", "elm", 0, 0) == 0)
		rt_kprintf("SPI File System initialized!\n");
	else
		rt_kprintf("SPI File System init failed!\n");

    	}
    	else
    	{
    		LED1_OFF();
    	}


		LED2_ON();
		rt_thread_delay(RT_TICK_PER_SECOND/2);
		LED2_OFF();
		rt_thread_delay(RT_TICK_PER_SECOND/2);

    }
}

void rt_init_thread_entry(void* parameter)
{
/* Filesystem Initialization */
#ifdef RT_USING_DFS
	{
		/* init the device filesystem */
		dfs_init();

#ifdef RT_USING_DFS_ELMFAT
		/* init the elm chan FatFs filesystam*/
		elm_init();

		/* mount sd card fat partition 1 as root directory */
		// if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
		// {
		// 	rt_kprintf("File System initialized!\n");
		// }
		// else
		// 	rt_kprintf("File System initialzation failed!\n");
#endif
	}
#endif

/* LwIP Initialization */
#ifdef RT_USING_LWIP
	{
		extern void lwip_sys_init(void);

		/* register ethernetif device */
		eth_system_device_init();

#ifdef STM32F10X_CL
		rt_hw_stm32_eth_init();
#else
	/* STM32F103 */
	#if STM32_ETH_IF == 0
			rt_hw_enc28j60_init();
	#elif STM32_ETH_IF == 1
			rt_hw_dm9000_init();
	#endif
#endif

		/* re-init device driver */
		rt_device_init_all();

		/* init lwip system */
		lwip_sys_init();
		rt_kprintf("TCP/IP initialized!\n");
	}
#endif

#ifdef RT_USING_RTGUI
	{
	    extern void rtgui_system_server_init(void);
	    extern void rt_hw_lcd_init();
	    extern void rtgui_touch_hw_init(void);

		rt_device_t lcd;

		/* init lcd */
		rt_hw_lcd_init();

		/* init touch panel */
		rtgui_touch_hw_init();

		/* re-init device driver */
		rt_device_init_all();

		/* find lcd device */
		lcd = rt_device_find("lcd");

		/* set lcd device as rtgui graphic driver */
		rtgui_graphic_set_device(lcd);

		/* init rtgui system server */
		rtgui_system_server_init();
	}
#endif /* #ifdef RT_USING_RTGUI */

#ifdef RT_USING_I2C	
	rt_i2c_stm32_add_bus();
#endif	

#ifdef RT_USING_SPI
	rt_stm32f10x_spi_init();
	rt_kprintf("spi bus\n");

	w25qxx_init("W25X10BV","spi10");
	rt_kprintf("W25X10BV devices\n");

#ifdef RT_USING_DFS
	#ifdef RT_USING_DFS_ELMFAT	
	/* mount spi flash fat as root directory */
	// if (dfs_mount("W25X10BV", "/", "elm", 0, 0) == 0)
	// 	rt_kprintf("SPI File System initialized!\n");
	// else
	// 	rt_kprintf("SPI File System init failed!\n");
	#endif
#endif

#if defined(RT_USING_DFS_DEVFS)
		devfs_init();
		if (dfs_mount(RT_NULL, "/devfs", "devfs", 0, 0) == 0)
			rt_kprintf("Device File System  devfs initialized!\n");
		else
			rt_kprintf("Device File System devfs initialzation failed!\n");
#endif


#endif

	rt_adc_device_init();
}

int rt_application_init()
{
	rt_thread_t init_thread;

	rt_err_t result;
#if 0
    /* init app thread */
	result = rt_thread_init(&app_thread,
		"app",
		app_thread_entry, RT_NULL,
		(rt_uint8_t*)&app_stack[0], sizeof(app_stack), 20, 5);
	if (result == RT_EOK)
	{
        rt_thread_startup(&app_thread);
	}
#else
	 rt_test_init();
#endif
	 
#if (RT_THREAD_PRIORITY_MAX == 32)
	init_thread = rt_thread_create("init",
								rt_init_thread_entry, RT_NULL,
								2048, 8, 20);
#else
	init_thread = rt_thread_create("init",
								rt_init_thread_entry, RT_NULL,
								2048, 80, 20);
#endif

	if (init_thread != RT_NULL)
		rt_thread_startup(init_thread);

	return 0;
}

/*@}*/
