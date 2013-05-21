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

static uint32_t stm32_ports[]=
{(uint32_t)GPIOA,(uint32_t)GPIOB,(uint32_t)GPIOC,
(uint32_t)GPIOD,(uint32_t)GPIOE,(uint32_t)GPIOF,(uint32_t)GPIOG};

static uint32_t RCC_ports[]=
{RCC_APB2Periph_GPIOA,RCC_APB2Periph_GPIOB,
 RCC_APB2Periph_GPIOC,RCC_APB2Periph_GPIOD,
 RCC_APB2Periph_GPIOE,RCC_APB2Periph_GPIOF,
 RCC_APB2Periph_GPIOG
};


void rt_hw_gpio_init(uint32_t port, uint32_t pin,uint32_t dir)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_ports[port],ENABLE);

	if(dir)
    	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    else
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin   = 1<<pin;
    GPIO_Init((GPIO_TypeDef *)stm32_ports[port], &GPIO_InitStructure);
}

void rt_hw_gpio_set(uint32_t port, uint32_t pin,uint32_t value)
{
    
	if(value)  //Output H
	{
		GPIO_SetBits((GPIO_TypeDef *)stm32_ports[port], 1<<pin);
	}
	else   //Output L
	{
		GPIO_ResetBits((GPIO_TypeDef *)stm32_ports[port], 1<<pin);
	}
	
}

uint32_t rt_hw_gpio_get(uint32_t port, uint32_t pin)
{
    return GPIO_ReadInputDataBit((GPIO_TypeDef *)stm32_ports[port], 1<<pin);
}


void gpio_direction_output(uint32_t gpio, uint32_t value)
{
	rt_hw_gpio_init(GET_PORT(gpio), GET_PIN(gpio),OUTPUT_DIRECTION);
	rt_hw_gpio_set(GET_PORT(gpio), GET_PIN(gpio), value);
}

uint32_t gpio_direction_input(uint32_t gpio)
{
	rt_hw_gpio_init(GET_PORT(gpio), GET_PIN(gpio),INPUT_DIRECTION);
	return rt_hw_gpio_get(GET_PORT(gpio), GET_PIN(gpio));
}

uint32_t gpio_get_value(uint32_t gpio)
{
	return rt_hw_gpio_get(GET_PORT(gpio), GET_PIN(gpio));
}

void gpio_set_value(uint32_t gpio, uint32_t value)
{
	rt_hw_gpio_set(GET_PORT(gpio), GET_PIN(gpio), value);
}



#ifdef RT_USING_FINSH
#include <finsh.h>
uint32_t gpio(uint32_t port, uint32_t pin,uint32_t dir,uint32_t value)
{
    rt_hw_gpio_init(port,pin,dir);
	if(dir) //Output
	{
		rt_hw_gpio_set(port,pin,value);	
		return 0;
	}
	else
	{
	 	return 	rt_hw_gpio_get(port,pin);
	}
}
FINSH_FUNCTION_EXPORT(gpio, set gpio[port] [pin] output[1] or in[0] H[1] or L[0])
#endif

