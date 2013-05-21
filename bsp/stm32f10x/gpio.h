/*
 * File      : gpio.h
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

#ifndef __GPIO_H__
#define __GPIO_H__

#include <rtthread.h>
#include <stdint.h>

#define GPIO_NUM(port, pin) ((port << 8) + pin)
#define GET_PORT(gpio) ((gpio >> 8 )& 0xFF)
#define GET_PIN(gpio)  (gpio & 0xFF)

#define OUTPUT_DIRECTION 1
#define INPUT_DIRECTION 0


void rt_hw_gpio_init(uint32_t port, uint32_t pin,uint32_t dir);

void gpio_direction_output(uint32_t gpio, uint32_t value);
uint32_t gpio_direction_input(uint32_t gpio);
uint32_t gpio_get_value(uint32_t gpio);
void gpio_set_value(uint32_t gpio, uint32_t value);

#endif
