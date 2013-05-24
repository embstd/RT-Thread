
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

#include <time.h>
#include <string.h>
#include <rtthread.h>
#include "gpio.h"
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t led_stack[ 512 ];
static rt_uint8_t app_stack[ 512 ];

int test_ret=-1;

#define A10_PWRKEY GPIO_NUM(3,8) //GPIO D8

#define PWRKEY_ON()  gpio_direction_output(A10_PWRKEY, 0)
#define PWRKEY_OFF() gpio_direction_output(A10_PWRKEY, 1)


#define A10_BL_EN  GPIO_NUM(3,9) //GPIO D9    //Check LCD BL EN
#define Is_suspend()  gpio_direction_input(A10_BL_EN)


#define LED2_GPIO GPIO_NUM(2,7) //GPIOC_7
#define LED2_ON()  gpio_direction_output(LED2_GPIO, 1)
#define LED2_OFF() gpio_direction_output(LED2_GPIO, 0)

#define KEY2_GPIO GPIO_NUM(4,1) //GPIOE_1
#define GETKEY2()  gpio_direction_input(KEY2_GPIO)


/*
	Return:
	0: IO Voltage is 0-0.5V
	1: IO Voltage is 2.8-3.3V
	-1: IO Voltage is 0.8-1.8V
*/
int check_io_voltage(rt_device_t device)
{
    rt_uint32_t data[32], i;
    rt_uint32_t cnt=8;
    int ret;
        
        rt_device_read(device, 0, data, cnt);
       
        
        rt_kprintf("ADC TEST: \n");
        for(i=0; i<cnt; i++)
        {
            if((!(i%4)) && i)
            {
                rt_kprintf("\n");
            }
            rt_kprintf("%d\t", data[i]);
        	if(data[i] > 2800)
        	{
        		ret=1;
        		break;
        	}
        	else if(data[i]<500)
        	{
        		ret=0;
				break;
        	}
        	else if(data[i]> 800 && data[i] < 2000)
        	{
        		ret=-1;
        	}
        	else
        	{
        		ret=-2;
        		break;
        	}

        }
        rt_kprintf("\n");

	rt_kprintf("check_io_voltage ret=%d\n", ret);
    return  ret;
}

#define A10_PWRENUP 0
#define A10_PWRSHUTDOWN 1
#define A10_SUSPEND 2

//GPIO_E1
void A10_PWR_CTL(int op)
{
	int i;

	if(op==0) //Short PWREN to power up system.
	{
		PWRKEY_OFF();  
		PWRKEY_ON();
		rt_thread_delay(RT_TICK_PER_SECOND*2);
		PWRKEY_OFF(); 
	}
	else if(op==1) // Long long PWREN to shutdown system
	{
		PWRKEY_OFF(); 
		PWRKEY_ON(); 
		rt_thread_delay(RT_TICK_PER_SECOND*10);
		PWRKEY_OFF(); 
	}
	else if(op==2) // short to suspen/resume system
	{
		PWRKEY_OFF();
		PWRKEY_ON();
		rt_thread_delay(RT_TICK_PER_SECOND/2);
		PWRKEY_OFF();
	}

	return ;
}


static struct rt_thread led_thread;
static void led_thread_entry(void* parameter)
{
    int ledstate=0;
    while(1)
    {
//Led off
        ledstate=!ledstate;

        if(ledstate)
            LED2_ON();
        else
            LED2_OFF();
        if(test_ret>0)
        {
            rt_thread_delay(RT_TICK_PER_SECOND/4);
        }
        else
        {
            rt_thread_delay(RT_TICK_PER_SECOND*3);
        }
    }
}

static struct rt_thread test_IO3V3_thread;
static void test_IO3V3_thread_entry(void* parameter)
{
    unsigned int count=0;
    int i,j,state,test_times=0, err_times=0;
    rt_device_t f_adc;

    f_adc = rt_device_find("ADC1");
    if (f_adc == RT_NULL)
    {
        rt_kprintf("Can't find ADC1 device.\n");
        return ;
    }
    if (rt_device_open(f_adc, 0) != RT_EOK)
    {
        rt_kprintf("Can't open ADC1 device.\n");
        return ;
    }

    A10_PWR_CTL(A10_PWRENUP);

    while (1)
    { 
//Key GPIOE_14, LED GPIOE_0
if ((!GETKEY2()) || (!test_ret)) //Key Down
{
    test_ret=0;
    test_times++;
    rt_kprintf("Test IO ADC....: Times=%d, err=%d\n", test_times, err_times);
    l_powerup:
    state=0;
    rt_kprintf("Test IO ADC....: power up state=%d and delay 1 min\n", state);
//Power UP
    A10_PWR_CTL(A10_PWRENUP);
// Delay 1 Min
    for(i=0; i<60; i++)
        rt_thread_delay(RT_TICK_PER_SECOND);

//Check IO is 3.3V
    if(check_io_voltage(f_adc) !=1)
    {
//If Not powerup state
        rt_kprintf("ERR: Not Powerup State\n");
        goto l_powerup;
    }    	

    state=1;
    rt_kprintf("Test IO ADC....: power off state=%d and delay 10 sec\n", state);
//Long powerkey shutdown sys
    A10_PWR_CTL(A10_PWRSHUTDOWN);
    rt_thread_delay(RT_TICK_PER_SECOND*2);

//Check IO is 0V
    if(check_io_voltage(f_adc) !=0)
    {
//If Not poweroff state
        rt_kprintf("ERR: Not Poweroff State\n");

        state=2;
        rt_kprintf("Test IO ADC....: bug state=%d\n", state);
        i=10;
        do
        {
            rt_thread_delay(RT_TICK_PER_SECOND);
//Check IO is 0.8-1.8V
            if(check_io_voltage(f_adc)!=-1)
            {
//If Not bug state
                rt_kprintf("ERR: Not Bug State\n");
                goto l_powerup;
            }

        }while(i--);

//OK. There is a BUG now.
        test_ret=1;
        err_times++;
    }
}
else
{
    check_io_voltage(f_adc);
}
rt_thread_delay(RT_TICK_PER_SECOND/2);

}
rt_device_close(f_adc);
}


static struct rt_thread test_suspend_thread;
static void test_suspend_thread_entry(void* parameter)
{
	    unsigned int count=0;
    int i,j,state,test_times=0, err_times=0;
    rt_device_t f_adc;

    f_adc = rt_device_find("ADC1");
    if (f_adc == RT_NULL)
    {
        rt_kprintf("Can't find ADC1 device.\n");
        return ;
    }
   if (rt_device_open(f_adc, 0) != RT_EOK)
    {
        rt_kprintf("Can't open ADC1 device.\n");
        return ;
    }

    A10_PWR_CTL(A10_PWRENUP);
    while(1)
    {

    	if ((!GETKEY2()) || (!test_ret)) //Key Down
    	{
    		test_ret=0;
    		test_times++;
    		rt_kprintf("Test suspend....: Times=%d, err=%d\n", test_times, err_times);
l_suspend:
    		state=0;
    		rt_kprintf("Test suspend....: suspend state=%d and delay 20 sec\n", state);
    		//To suspend
    		A10_PWR_CTL(A10_SUSPEND);
    		rt_thread_delay(RT_TICK_PER_SECOND*20);
    		//Check is suspend state.
    		if (Is_suspend())
    		{
    			//resume?
    			rt_kprintf("ERR: Not suspend State\n");
               //test_ret++;
                err_times++;
    			goto l_suspend;
    		}
            if(check_io_voltage(f_adc))
            {
                //resume?
                rt_kprintf("ERR: Not suspend State, There are voltage in 3V for A31S.\n");

                //goto l_suspend;
            }

    		// OK, now is suspend. Pls sleep 1 min
    		rt_kprintf("Test suspend....: Now is suspend state and delay 1 min\n");
    		rt_thread_delay(RT_TICK_PER_SECOND*20);
l_resume:
    		//To resume
    		rt_kprintf("Test suspend....: resume state=%d and delay 20 sec\n", state);
    		A10_PWR_CTL(A10_SUSPEND);
    		rt_thread_delay(RT_TICK_PER_SECOND*20);
    		//Check is resume state.
    		if (!Is_suspend())
    		{
    			//resume?
    			rt_kprintf("ERR: Not resume State\n");
                //test_ret++;
                err_times++;
    			goto l_resume;
    		}
    		// OK, now is resume. Pls sleep 1 min
    		rt_kprintf("Test resume....: Now is resume state and delay 1 min\n");
    		rt_thread_delay(RT_TICK_PER_SECOND*20);    		

    	}

		rt_thread_delay(RT_TICK_PER_SECOND/2);

    }
         rt_device_close(f_adc);
}

int rt_test_init(void)
{
	rt_thread_t init_thread;

	rt_err_t result;

    /* init led thread */
	result = rt_thread_init(&led_thread,
		"led",
		led_thread_entry, RT_NULL,
		(rt_uint8_t*)&led_stack[0], sizeof(led_stack), 20, 5);
	if (result == RT_EOK)
	{
        rt_thread_startup(&led_thread);
	}
////////////////////////////////////////////////////
////////////////////////////////////////////////////	
#if 1 
//Test IO3V3
    /* init test thread */
	result = rt_thread_init(&test_IO3V3_thread,
		"app",
		test_IO3V3_thread_entry, RT_NULL,
		(rt_uint8_t*)&app_stack[0], sizeof(app_stack), 20, 5);
	if (result == RT_EOK)
	{
        rt_thread_startup(&test_IO3V3_thread);
	}
#else	
//Test suspend
	    /* init test thread */
	result = rt_thread_init(&test_suspend_thread,
		"app",
		test_suspend_thread_entry, RT_NULL,
		(rt_uint8_t*)&app_stack[0], sizeof(app_stack), 20, 5);
	if (result == RT_EOK)
	{
        rt_thread_startup(&test_suspend_thread);
	}
#endif
////////////////////////////////////////////////////
////////////////////////////////////////////////////	

	return 0;
}

/*@}*/
