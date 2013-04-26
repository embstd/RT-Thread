/*
 * File      : mem.c
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


#ifdef RT_USING_FINSH
#include <finsh.h>
uint32_t mem(char *op, uint32_t reg,uint32_t num_data)
{
    __IO uint32_t *addr;
    int i;
    addr=(__IO uint32_t *)reg;
    //rt_kprintf("address =0x%x\t", addr);
    switch(*op)
    {

    	case 'r':
    	case 'R':
			for(i=0; i<num_data; i++)
			{
				if((!(i%4)) && i)
					rt_kprintf("\n");
    			rt_kprintf("0x%08x\t", *(__IO uint32_t *)(addr+i*4));
    			//rt_kprintf("0-x%x\t", *(__IO uint32_t *)(reg+i));
			}
    		break;
    	case 'w':
    	case 'W':
    		*(__IO uint32_t *)(addr) = num_data;
    		break;
    	default:
    		rt_kprintf("mem: Unknow option: %c\n", *op);
    		return RT_ERROR;
    }
    rt_kprintf("\n");
    return RT_EOK;
}

// Get/Set memory/reg.
FINSH_FUNCTION_EXPORT(mem, mem [r/w] [reg] [num/data])
#endif

