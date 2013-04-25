/*
 * File      : rtc.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-01-29     aozima       first version.
 * 2012-04-12     aozima       optimization: find rtc device only first.
 * 2012-04-16     aozima       add scheduler lock for set_date and set_time.
 */

#include <time.h>
#include <string.h>
#include <rtthread.h>
#include <i2c.h>
#include <i2c_dev.h>
 
#if 1
#define i2c_dbg(fmt, ...)   do{rt_kprintf("i2c:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(0)
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


#define AT24CXX_ADDR 0x50
#define I2C_PageSize 16

rt_err_t at24cxx_page_write(rt_uint8_t * pBuffer, rt_uint32_t addr, rt_uint32_t num)
{
    struct rt_i2c_priv_data i2c_priv_data;
    struct rt_i2c_msg i2c_msg[2];
    rt_device_t device;
    rt_uint8_t *data, tmp;
    rt_err_t ret = -RT_ERROR;

    device = rt_device_find("I2C");
    if (device == RT_NULL)
    {
        i2c_err("Can't find I2C device.\n");
        return -RT_ERROR;
    }

    data=pBuffer-1;
    tmp=*data;
    *data=addr;

    // Write Action
    i2c_msg[0].addr=AT24CXX_ADDR;
    i2c_msg[0].flags=RT_I2C_WR ;
    i2c_msg[0].len=num;
    i2c_msg[0].buf=data;

    i2c_priv_data.msgs=i2c_msg;
    i2c_priv_data.number=1;
  
    if (rt_device_open(device, 0) == RT_EOK)
    {
        rt_device_control(device, RT_I2C_DEV_CTRL_RW, &i2c_priv_data);
        rt_device_close(device);
    }
    else
    {
        i2c_err("open:w err\n");
        goto l_err;
    }

l_err:
    *(pBuffer-1)=tmp;
    return ret;
}


rt_err_t at24c16b_write(rt_uint32_t WriteAddr, rt_uint32_t NumByteToWrite)
{

    rt_uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0;
    rt_uint32_t i=0;
    rt_uint8_t *pBuffer, *p;

    rt_kprintf("\n=======at24c16b write Test addr=0x%x, num=%d ========= \n", WriteAddr, NumByteToWrite);
    //pBuffer-1 = WriteAddr
    pBuffer= (rt_uint8_t*) rt_malloc (sizeof(rt_uint8_t) * (NumByteToWrite +1));
    if(pBuffer == RT_NULL)
    {
        i2c_err("Can't find malloc memory.\n");
        return RT_ERROR;
    }

    p=pBuffer++;

    for(i=0; i< NumByteToWrite; i++, pBuffer++)
    {
        *pBuffer = (rt_uint8_t)(i & 0xFF);   
        if((!(i%4)) && i)
        {
            rt_kprintf("\n");
        }
          rt_kprintf("0x%02x\t", *pBuffer); 
    }
    rt_kprintf("\n");
    //p-1 = WriteAddr
    pBuffer=p+1;

    Addr = WriteAddr % I2C_PageSize;
    count = I2C_PageSize - Addr;
    NumOfPage =  NumByteToWrite / I2C_PageSize;
    NumOfSingle = NumByteToWrite % I2C_PageSize;
    


      /* If WriteAddr is I2C_PageSize aligned  */
      if(Addr == 0) 
      {
        /* If NumByteToWrite < I2C_PageSize */
        if(NumOfPage == 0) 
        {
          at24cxx_page_write(pBuffer, WriteAddr, NumOfSingle);
        }
        /* If NumByteToWrite > I2C_PageSize */
        else  
        {
          while(NumOfPage--)
          {
            at24cxx_page_write(pBuffer, WriteAddr, I2C_PageSize); 
            WriteAddr +=  I2C_PageSize;
            pBuffer += I2C_PageSize;
          }

          if(NumOfSingle!=0)
          {
            at24cxx_page_write(pBuffer, WriteAddr, NumOfSingle);
          }
        }
      }
      /* If WriteAddr is not I2C_PageSize aligned  */
      else 
      {
        /* If NumByteToWrite < I2C_PageSize */
        if(NumOfPage== 0) 
        {
          at24cxx_page_write(pBuffer, WriteAddr, NumOfSingle);
        }
        /* If NumByteToWrite > I2C_PageSize */
        else
        {
          NumByteToWrite -= count;
          NumOfPage =  NumByteToWrite / I2C_PageSize;
          NumOfSingle = NumByteToWrite % I2C_PageSize;  
          
          if(count != 0)
          {  
            at24cxx_page_write(pBuffer, WriteAddr, count);
            WriteAddr += count;
            pBuffer += count;
          } 
          
          while(NumOfPage--)
          {
            at24cxx_page_write(pBuffer, WriteAddr, I2C_PageSize);
            WriteAddr +=  I2C_PageSize;
            pBuffer += I2C_PageSize;  
          }
          if(NumOfSingle != 0)
          {
            at24cxx_page_write(pBuffer, WriteAddr, NumOfSingle); 
          }
        }
      }
      rt_free(p);  
    
}

rt_err_t at24c16b_read(rt_uint32_t addr, rt_uint32_t num)
{

    struct rt_i2c_priv_data i2c_priv_data;
    struct rt_i2c_msg i2c_msg[2];
    rt_device_t device;
    rt_err_t ret = -RT_ERROR;
    rt_uint32_t i=0;
    rt_uint8_t *pBuffer, *p;

    rt_kprintf("\n=======at24c16b read Test addr=0x%x, num=%d ========= \n", addr, num);
    pBuffer= (rt_uint8_t*) rt_malloc (sizeof(rt_uint8_t) * num);
    if(pBuffer == RT_NULL)
    {
        i2c_err("Can't find malloc memory.\n");
        return RT_ERROR;
    }

    device = rt_device_find("I2C");
    if (device == RT_NULL)
    {
        i2c_err("Can't find I2C device.\n");
        return RT_ERROR;
    }
    // Read Action
    // Write reg addr msg
    i2c_msg[0].addr=AT24CXX_ADDR;
    i2c_msg[0].flags=RT_I2C_WR;
    i2c_msg[0].len=1;
    i2c_msg[0].buf=(rt_uint8_t *)&addr;
    // Read data msg
    i2c_msg[1].addr=AT24CXX_ADDR;
    i2c_msg[1].flags=RT_I2C_RD;
    i2c_msg[1].len=num;
    i2c_msg[1].buf=pBuffer;

    i2c_priv_data.msgs=i2c_msg;
    i2c_priv_data.number=2;
  
    if (rt_device_open(device, 0) == RT_EOK)
    {
        rt_device_control(device, RT_I2C_DEV_CTRL_RW, &i2c_priv_data);
        rt_device_close(device);
    }
    else
    {

        i2c_err("open:r err\n");
        goto l_err;
    }

    for(i=0, p=pBuffer; i< num; i++, p++)
    {
         if((!(i%4)) && i)
        {
                  rt_kprintf("\n");
        }
          rt_kprintf("0x%02x\t", *p);
    }
    rt_kprintf("\n");
    rt_free(pBuffer);  
    
l_err:
    return ret;

}

rt_err_t test_usage(void)
{
    rt_kprintf("\n================ \n"
    "0: For help\n "
    "1: For test at24c16b write IIC.\n "
    "2: For test at24c16b read IIC.\n "
    "=======================\n");
    return RT_EOK;
}


#define AT_ADDR 0x0
#define AT_NUM  128

rt_err_t test(rt_uint32_t op)
{

    switch(op)
    {
        case 0: 
            test_usage();
            break;
        case 1:
            at24c16b_write(AT_ADDR, AT_NUM);
            break;
        case 2:
            at24c16b_read(AT_ADDR, AT_NUM);
            break;
        default:
            rt_kprintf("mem: Unknow option: %d\n", op);
            return RT_ERROR;
    }
    return RT_EOK;
}


#ifdef RT_USING_FINSH
#include <finsh.h>
#include <rtdevice.h>

// void list_date(void)
// {
//     time_t now;

//     now = time(RT_NULL);
//     rt_kprintf("%s\n", ctime(&now));
// }
// FINSH_FUNCTION_EXPORT(list_date, show date and time.)

FINSH_FUNCTION_EXPORT(i2c, i2c(W[0]/R[1] Addr reg data))
FINSH_FUNCTION_EXPORT(test, test example. e.g: test(0) for help)
#endif
