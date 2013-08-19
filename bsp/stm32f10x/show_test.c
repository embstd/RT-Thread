/*
 * File      : led.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */
#include <rtthread.h>
#include <stm32f10x.h>
 
#include "gpio.h"
#include "showtest.h"

ALIGN(RT_ALIGN_SIZE)

#define BLUE_GPIO GPIO_NUM(2,6) //GPIOC_6
#define BLUE_ON()  gpio_direction_output(BLUE_GPIO, 1)
#define BLUE_OFF() gpio_direction_output(BLUE_GPIO, 0)

#define RED_GPIO GPIO_NUM(2,7) //GPIOC_7
#define RED_ON()  gpio_direction_output(RED_GPIO, 1)
#define RED_OFF() gpio_direction_output(RED_GPIO, 0)


static rt_uint8_t show_test_stack[ 512 ];


static enum SHOW_TEST show_test=SHOW_TEST_NC;
uint8_t cnt=0;

static struct rt_thread show_test_thread;
static void show_test_thread_entry(void* parameter)
{
	
	int duty=RT_TICK_PER_SECOND;
    while(1)
    {
    	switch (show_test)
    	{	

    		case SHOW_TEST_OK: 
    			RED_OFF();
    			BLUE_ON();
    			duty=RT_TICK_PER_SECOND;
    			break;
    		case SHOW_TEST_ERROR: 
    			BLUE_OFF();
    			RED_ON();
    			duty=RT_TICK_PER_SECOND;
    			break;	
    	    case SHOW_TEST_ING:
    	    	BLUE_OFF(); 
    	    	if(++cnt%2)
    	    		RED_ON();
    	    	else
    	    		RED_OFF();
    	    	duty=RT_TICK_PER_SECOND/4;
				break;

			case SHOW_TEST_INC:
    	    	RED_OFF();
    	    	if(cnt%2)
    	    		BLUE_ON();
    	    	else
    	    		BLUE_OFF(); 
    	    	duty=RT_TICK_PER_SECOND/4;
				if(!cnt--)
					show_test=SHOW_TEST_NC;
				break;

    		case SHOW_TEST_NC: 
    			BLUE_OFF();
    			RED_OFF();
    			duty=RT_TICK_PER_SECOND;
    			break;
    		default: 

    			break;
    	}
        rt_thread_delay(duty);
    }
}

void show_test_set(enum SHOW_TEST s, uint8_t c)
{
    rt_kprintf("show_test_set = 0x%x\n", s);
	show_test=s;
	cnt=c;
}

enum SHOW_TEST show_test_get(void)
{
	return show_test;
}


int rt_show_test_init(void)
{
	rt_err_t result;

    /* init show_test thread */
	result = rt_thread_init(&show_test_thread,
		"show_test",
		show_test_thread_entry, RT_NULL,
		(rt_uint8_t*)&show_test_stack[0], sizeof(show_test_stack), 20, 5);
	if (result == RT_EOK)
	{
        rt_thread_startup(&show_test_thread);
	}
	return 0;
}
