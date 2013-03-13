
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

#include "led.h"

ALIGN(RT_ALIGN_SIZE)
/*LED1�߳̿���ģ����߳���ں���*/
static rt_uint8_t led1_stack[ 512 ];//LED1�߳�ջ
static struct rt_thread led1_thread;//����LED1�߳̿��ƿ�
/*LED1�߳���ں���*/
static void led1_thread_entry(void* parameter)
{
  /*��LED1�߳��г�ʼ��LED��GPIO����*/

  LED_GPIO_Config();
  while(1)
  {
   LED1(ON);//����LED1
   rt_thread_delay(200);//����RTT��API����ǰ�̹߳���100ticks��Ҳ����1sec
   LED1(OFF);//�ر�LED1
   rt_thread_delay(200);
  }
}

/*LED2�߳̿��ƿ����߳���ں���*/
static rt_uint8_t led2_stack[ 512 ];//LED2�߳�ջ
static struct rt_thread led2_thread;//LED2�߳̿��ƿ�
/*LED2�߳���ں���*/
static void led2_thread_entry(void* parameter)
{
 while(1)
 {
  LED3(ON);//����LED3
  rt_thread_delay(50);//����RTT��API����ǰ�̹߳���50tick��Ҳ����0.5sec
  LED3(OFF);//�ر�LED3
  rt_thread_delay(50);
 }
}
//void rt_init_thread_entry(void* parameter)
//{
///* Filesystem Initialization */
//#ifdef RT_USING_DFS
//	{
//		/* init the device filesystem */
//		dfs_init();
//
//#ifdef RT_USING_DFS_ELMFAT
//		/* init the elm chan FatFs filesystam*/
//		elm_init();
//
//		/* mount sd card fat partition 1 as root directory */
//		if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
//		{
//			rt_kprintf("File System initialized!\n");
//		}
//		else
//			rt_kprintf("File System initialzation failed!\n");
//#endif
//	}
//#endif
//
///* LwIP Initialization */
//#ifdef RT_USING_LWIP
//	{
//		extern void lwip_sys_init(void);
//
//		/* register ethernetif device */
//		eth_system_device_init();
//
//#ifdef STM32F10X_CL
//		rt_hw_stm32_eth_init();
//#else
//	/* STM32F103 */
//	#if STM32_ETH_IF == 0
//			rt_hw_enc28j60_init();
//	#elif STM32_ETH_IF == 1
//			rt_hw_dm9000_init();
//	#endif
//#endif
//
//		/* re-init device driver */
//		rt_device_init_all();
//
//		/* init lwip system */
//		lwip_sys_init();
//		rt_kprintf("TCP/IP initialized!\n");
//	}
//#endif
//
//#ifdef RT_USING_RTGUI
//	{
//	    extern void rtgui_startup();
//	    extern void rt_hw_lcd_init();
//	    extern void rtgui_touch_hw_init(void);
//
//		rt_device_t lcd;
//
//		/* init lcd */
//		rt_hw_lcd_init();
//
//		/* init touch panel */
//		rtgui_touch_hw_init();
//
//		/* re-init device driver */
//		rt_device_init_all();
//
//		/* find lcd device */
//		lcd = rt_device_find("lcd");
//
//		/* set lcd device as rtgui graphic driver */
//		rtgui_graphic_set_device(lcd);
//
//		/* startup rtgui */
//		rtgui_startup();
//	}
//#endif /* #ifdef RT_USING_RTGUI */
//}

int rt_application_init()
{
//	rt_thread_t init_thread;

	rt_err_t result;

   /*��ʼ��LED1�߳�*/
	result = rt_thread_init(&led1_thread,//LED1�߳̿��ƿ�
		                    "led1",//LED1�߳�����
		                    led1_thread_entry,//LED1�߳���� 
							RT_NULL,//LED1�������
		                    &led1_stack[0],//LED1�߳�ջ�׵�ַ
							sizeof(led1_stack),//LED1�߳�ջ��С
							20,//LED1�߳����ȼ�
							5//LED1�߳�ʱ��Ƭ�ĳ���
							);
	/*�����ʼ��LED1�̳߳ɹ�*/
	if (result == RT_EOK)
	{	/*����LED1�߳�*/
        rt_thread_startup(&led1_thread);
	}


	/*��ʼ��LED1�߳�*/
	result = rt_thread_init(&led2_thread,//LED1�߳̿��ƿ�
		                    "led2",//LED1�߳�����
		                    led2_thread_entry,//LED1�߳���� 
							RT_NULL,//LED1�������
		                    &led2_stack[0],//LED1�߳�ջ�׵�ַ
							sizeof(led2_stack),//LED1�߳�ջ��С
							20,//LED1�߳����ȼ�
							10//LED1�߳�ʱ��Ƭ�ĳ���
							);
	/*�����ʼ��LED1�̳߳ɹ�*/
	if (result == RT_EOK)
	{	/*����LED1�߳�*/
        rt_thread_startup(&led2_thread);
	}
//#if (RT_THREAD_PRIORITY_MAX == 32)
//	init_thread = rt_thread_create("init",
//								rt_init_thread_entry, RT_NULL,
//								2048, 8, 20);
//#else
//	init_thread = rt_thread_create("init",
//								rt_init_thread_entry, RT_NULL,
//								2048, 80, 20);
//#endif
//
//	if (init_thread != RT_NULL)
//		rt_thread_startup(init_thread);

	return 0;
}

/*@}*/
