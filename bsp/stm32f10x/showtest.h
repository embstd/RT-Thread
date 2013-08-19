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

#ifndef __SHOWTEST_H__
#define __SHOWTEST_H__

#include <rtthread.h>
#include <stdint.h>

enum SHOW_TEST
{
	SHOW_TEST_OK = 0, SHOW_TEST_ERROR = 1,
	SHOW_TEST_ING = 2,SHOW_TEST_INC = 3,SHOW_TEST_NC = 4
};

void show_test_set(enum SHOW_TEST s, uint8_t c);

enum SHOW_TEST show_test_get(void);


#endif
