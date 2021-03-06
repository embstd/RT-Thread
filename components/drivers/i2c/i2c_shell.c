/*
 * File      : i2c.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-04-26     EmbStd@gmail.com       first version.
 */

#include <time.h>
#include <string.h>
#include <rtthread.h>
#include <i2c.h>
#include <i2c_dev.h>
 
#if 1
#define i2c_dbg(fmt, ...)   do{rt_kprintf("i2c:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(0)
#else
#define i2c_dbg(fmt, ...)
#endif
#define i2c_err(fmt, ...)   do{rt_kprintf("[ERR] i2c:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(0)


/** \brief set system time(date not modify).
 *
 * \param rt_uint32_t hour   e.g: 0~23.
 * \param rt_uint32_t minute e.g: 0~59.
 * \param rt_uint32_t second e.g: 0~59.
 * \return rt_err_t if set success, return RT_EOK.
 *
 */
rt_err_t i2c(rt_uint8_t wr, rt_uint32_t addr, rt_uint32_t reg, rt_uint32_t data)
{
    struct rt_i2c_priv_data i2c_priv_data;
    struct rt_i2c_msg i2c_msg[2];
    rt_uint8_t i2c_data[10];
    rt_device_t device;
    rt_err_t ret = -RT_ERROR;

    device = rt_device_find("I2C");
    if (device == RT_NULL)
    {
        i2c_err("Can't find I2C device.\n");
        return -RT_ERROR;
    }

    if(!wr)
    {
        // Write Action
        i2c_msg[0].addr=addr;
        i2c_msg[0].flags=RT_I2C_WR ;
        i2c_msg[0].len=2;
        i2c_data[0]=reg;
        i2c_data[1]=data;
        i2c_msg[0].buf=i2c_data;

        i2c_priv_data.msgs=i2c_msg;
        i2c_priv_data.number=1;
      
        if (rt_device_open(device, 0) == RT_EOK)
        {
            i2c_dbg("write msg addr=0x%x, len=%d, reg=0x%d,  data=0x%x\n",i2c_msg[0].addr, i2c_msg[0].len, i2c_data[0], i2c_data[1]);
            rt_device_control(device, RT_I2C_DEV_CTRL_RW, &i2c_priv_data);
            rt_device_close(device);
        }
        else
        {
            i2c_err("open:w err\n");
            goto l_err;
        }
    }
    else
    {
        // Read Action
        // Write reg addr msg
        i2c_msg[0].addr=addr;
        i2c_msg[0].flags=RT_I2C_WR;
        i2c_msg[0].len=1;
        i2c_data[0]=reg;
        i2c_msg[0].buf=i2c_data;
        // Read data msg
        i2c_msg[1].addr=addr;
        i2c_msg[1].flags=RT_I2C_RD;
        i2c_msg[1].len=1;
        i2c_data[1]=0;
        i2c_msg[1].buf=&i2c_data[1];

        i2c_priv_data.msgs=i2c_msg;
        i2c_priv_data.number=2;
      
        if (rt_device_open(device, 0) == RT_EOK)
        {
            rt_device_control(device, RT_I2C_DEV_CTRL_RW, &i2c_priv_data);
            i2c_dbg("read msg addr=0x%x, len=%d, addr=0x%d,  data=0x%x\n",i2c_msg[0].addr, i2c_msg[0].len, i2c_data[0], i2c_data[1]);
            rt_device_close(device);
        }
        else
        {

            i2c_err("open:r err\n");
            goto l_err;
        }
    }

l_err:
    return ret;
}


#ifdef RT_USING_FINSH
#include <finsh.h>
#include <rtdevice.h>

FINSH_FUNCTION_EXPORT(i2c, i2c(W[0]/R[1] Addr reg data))
#endif
