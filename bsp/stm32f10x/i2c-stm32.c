/*
 * File      : i2c-stm32.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author        Notes
 * 2013-04-25     EmbStd@gmail.com         first version
 */

#include <rtdevice.h>
#include <stm32f10x.h>
#include <stm32f10x_i2c.h>
#include <stm32f10x_rcc.h>
#include <i2c.h>
#include <i2c_dev.h>

#define I2C_Speed              300000
#define I2C_BUS_NUM_SLAVE_ADDRESS7    0x61
#define I2C_BUS_NUM   I2C1   //I2C1, I2C2

#if 0
#define stm32_dbg(fmt, ...)   do{rt_kprintf("stm i2c:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(0)
#else
#define stm32_dbg(fmt, ...)
#endif
#define stm32_err(fmt, ...)   do{rt_kprintf("[ERR] stm i2c:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(0)

struct rt_i2c_stm32_ops
{
    void *data;            /* private data for lowlevel routines */
};

#define TIMEOUT 5
static rt_err_t stm32_i2c_check_timeout(I2C_TypeDef* I2Cx, uint32_t I2C_EVENT, int to, char * msg)
{
    uint32_t last_event;

  for(; to > 0; to--)
  {   
    if(I2C_CheckEvent(I2Cx, I2C_EVENT))
    {
      return RT_EOK;    
    }
    rt_thread_delay(RT_TICK_PER_SECOND/4); /* sleep 0.25 second and switch to other thread */
  }
  last_event=I2C_GetLastEvent(I2Cx);
  stm32_err("[%s] Time out to check event 0x%x vs last_event[0x%x] .\n", msg, I2C_EVENT, last_event);
  return RT_ERROR;
}

static void i2c_start(struct rt_i2c_stm32_ops *ops)
{
    stm32_dbg("send start condition\n");
  /* Send STRAT condition */
    I2C_GenerateSTART(I2C_BUS_NUM, ENABLE);
      /* Test on EV5 and clear it */
    stm32_i2c_check_timeout(I2C_BUS_NUM, I2C_EVENT_MASTER_MODE_SELECT,TIMEOUT, "i2c_start EV5");
}

static void i2c_restart(struct rt_i2c_stm32_ops *ops)
{
    stm32_dbg("send re-start condition\n");
    /* Send STRAT condition */
    I2C_GenerateSTART(I2C_BUS_NUM, ENABLE);  
    /* Test on EV5 and clear it */
    stm32_i2c_check_timeout(I2C_BUS_NUM, I2C_EVENT_MASTER_MODE_SELECT,TIMEOUT, "i2c_restart EV5");
}

static void i2c_stop(struct rt_i2c_stm32_ops *ops)
{
  stm32_dbg("send stop condition\n");
  /* Send STOP condition */
  I2C_GenerateSTOP(I2C_BUS_NUM, ENABLE);
}
/*
rt_inline rt_bool_t i2c_waitack(struct rt_i2c_stm32_ops *ops)
{
    rt_bool_t ack;

    return ack;
}
*/

static rt_size_t i2c_send_bytes(struct rt_i2c_bus_device *bus,
                                struct rt_i2c_msg        *msg)
{
  //rt_int32_t ret;
  //rt_size_t bytes = 0;
  const rt_uint8_t *ptr = msg->buf;
  rt_int32_t count =0;
  rt_uint16_t ignore_nack = msg->flags & RT_I2C_IGNORE_NACK;

  stm32_dbg("send_bytes, len=%d\n", msg->len);


/* Test on EV6 and clear it */ 
  //stm32_i2c_check_timeout(I2C_BUS_NUM, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, TIMEOUT,"i2c_send_bytes");

  while (count < msg->len) 
  {
    stm32_dbg("send_bytes: count=%d, data=0x%x\n",count,*ptr);
  /* Send the current byte */
    I2C_SendData(I2C_BUS_NUM, *ptr); 

  /* Point to the next byte to be written */
    ptr++; 

    if(count == (msg->len-1))
    {
        /* Test on EV8 and clear it */
        stm32_i2c_check_timeout(I2C_BUS_NUM, I2C_EVENT_MASTER_BYTE_TRANSMITTING, TIMEOUT, "i2c_send_bytes EV8");
    }
    else
    {
      /* Test on EV8_2 and clear it */
        stm32_i2c_check_timeout(I2C_BUS_NUM, I2C_EVENT_MASTER_BYTE_TRANSMITTED, TIMEOUT, "i2c_send_bytes EV8_2");
    }
      count ++; 
  };

  return count;
}

static rt_err_t i2c_send_ack_or_nack(struct rt_i2c_bus_device *bus, int ack)
{
  if (ack)
  {
    I2C_AcknowledgeConfig(I2C_BUS_NUM, ENABLE);
    stm32_dbg("send Ack.\n");
  }else
  {
     I2C_AcknowledgeConfig(I2C_BUS_NUM, DISABLE);
  }
  return RT_EOK;
}

static rt_size_t i2c_recv_bytes(struct rt_i2c_bus_device *bus,
                                struct rt_i2c_msg        *msg)
{
  rt_int32_t val;
  rt_int32_t bytes        = 0;   /* actual bytes */
  rt_uint8_t *ptr         = msg->buf;
  rt_int32_t count        = 0;
  const rt_uint32_t flags = msg->flags;
  stm32_dbg("recv_bytes, len=%d\n", count);
/* While there is data to be read */
  while(count < msg->len)  
  {

    if (!(flags & RT_I2C_NO_READ_ACK))
    {
      val                     = i2c_send_ack_or_nack(bus,  msg->len - count - 1);
      if (val < 0)
      {
        stm32_err("recv_bytes:No Read Ack.\n");
        return val;
      }
    }

/* Test on EV7 and clear it */
    //if(I2C_CheckEvent(I2C_BUS_NUM, I2C_EVENT_MASTER_BYTE_RECEIVED))  
    if(!stm32_i2c_check_timeout(I2C_BUS_NUM, I2C_EVENT_MASTER_BYTE_RECEIVED, TIMEOUT, "i2c_recv_bytes"))
    {      
/* Read a byte from the EEPROM */
      *ptr                    = I2C_ReceiveData(I2C_BUS_NUM);

      stm32_dbg("recv_bytes:data[%d]=0x%x.\n",count, *ptr);
/* Point to the next location where the byte read will be saved */
      ptr++; 
    } 
    count ++;  
  }
/* Enable Acknowledgement to be ready for another reception */
  I2C_AcknowledgeConfig(I2C_BUS_NUM, ENABLE);

  return count;
}

static rt_int32_t i2c_send_address(struct rt_i2c_bus_device *bus,
                                   rt_uint8_t                addr,
                                   rt_int32_t                retries)
{

  rt_int32_t ret;
  stm32_dbg("send_address:0x%x\n", addr);
  

  if(addr&0x1)
  {
    /* Send EEPROM address for read */
    I2C_Send7bitAddress(I2C_BUS_NUM, addr & 0xFE, I2C_Direction_Receiver);
    /* Test on EV6 and clear it */
    ret=stm32_i2c_check_timeout(I2C_BUS_NUM, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, TIMEOUT,"i2c_send_address rEV6");
  }else
  {
    /* Send EEPROM address for write */
    I2C_Send7bitAddress(I2C_BUS_NUM, addr& 0xFE, I2C_Direction_Transmitter);
    /* Test on EV6 and clear it */
    ret=stm32_i2c_check_timeout(I2C_BUS_NUM, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, TIMEOUT,"i2c_send_address wEV6");
  }
  return ret==RT_EOK;
}

static rt_err_t i2c_stm32_send_address(struct rt_i2c_bus_device *bus,
                                     struct rt_i2c_msg        *msg)
{
    rt_uint16_t flags = msg->flags;
    rt_uint16_t ignore_nack = msg->flags & RT_I2C_IGNORE_NACK;
    struct rt_i2c_stm32_ops *ops = bus->priv;

    rt_uint8_t addr1;
    rt_int32_t retries;
    rt_err_t ret;

    retries = ignore_nack ? 0 : bus->retries;
    stm32_dbg("send addr:retries=%d\n", retries);

    //Only support 7 bit addresss now.
    if (!(flags & RT_I2C_ADDR_10BIT))
    {
        /* 7-bit addr */
        addr1 = msg->addr << 1;
        if (flags & RT_I2C_RD)
            addr1 |= 1;
        ret = i2c_send_address(bus, addr1, retries);
        if ((ret != 1) && !ignore_nack)
            return -RT_EIO;
    }

    return RT_EOK;
}

static rt_size_t i2c_stm32_xfer(struct rt_i2c_bus_device *bus,
                              struct rt_i2c_msg         msgs[],
                              rt_uint32_t               num)
{
    struct rt_i2c_msg *msg;
    struct rt_i2c_stm32_ops *ops = bus->priv;
    rt_int32_t i, ret;
    rt_uint16_t ignore_nack;

    /* 使能 I2C_BUS_NUM */
    I2C_Cmd(I2C_BUS_NUM, ENABLE);

    i2c_start(ops);
    for (i = 0; i < num; i++)
    {
        msg = &msgs[i];
        ignore_nack = msg->flags & RT_I2C_IGNORE_NACK;
        if (!(msg->flags & RT_I2C_NO_START))
        {
            if (i)
            {
                i2c_restart(ops);
            }
            ret = i2c_stm32_send_address(bus, msg);
            if ((ret != RT_EOK) && !ignore_nack)
            {
                stm32_err("receive NACK from device addr 0x%02x msg %d\n",
                        msgs[i].addr, i);
                goto out;
            }
        }
        if (msg->flags & RT_I2C_RD)
        {
            ret = i2c_recv_bytes(bus, msg);
            if (ret >= 1)
                stm32_dbg("read %d byte%s\n", ret, ret == 1 ? "" : "s");
            if (ret < msg->len)
            {
                if (ret >= 0)
                    ret = -RT_EIO;
                goto out;
            }
        }
        else
        {
            ret = i2c_send_bytes(bus, msg);
            if (ret >= 1)
                stm32_dbg("write %d byte%s\n", ret, ret == 1 ? "" : "s");
            if (ret < msg->len)
            {
                if (ret >= 0)
                {
                    stm32_err("send bytes:i=%d,err=%d\n",i,ret);
                    ret = -RT_ERROR;
                }    
                goto out;
            }
        }
    }
    ret = i;

out:

    i2c_stop(ops);

    return ret;
}

static const struct rt_i2c_bus_device_ops i2c_stm32_bus_ops =
{
    i2c_stm32_xfer,
    RT_NULL,
    RT_NULL
};

/*
 * 函数名：I2C_GPIO_Config
 * 描述  ：I2C_BUS_NUM I/O配置
 * 输入  ：无
 * 输出  ：无
 * 调用  ：内部调用
 */
static void I2C_GPIO_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure; 
 
  if(I2C_BUS_NUM == I2C1)
  {
    stm32_dbg("Init I2C1 GPIO\n");
        /* 使能与 I2C_BUS_NUM 有关的时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);  
      
    /* PB6-I2C1_SCL、PB7-I2C1_SDA*/
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;        // 开漏输出
    GPIO_Init(GPIOB, &GPIO_InitStructure);
  }
  else
  {
     stm32_dbg("Init I2C2 GPIO\n");
        /* 使能与 I2C_BUS_NUM 有关的时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2,ENABLE);  
      
    /* PB10-I2C2_SCL、PB11-I2C2_SDA*/
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;        // 开漏输出
    GPIO_Init(GPIOB, &GPIO_InitStructure);

  }

}

/*
 * 函数名：I2C_Configuration
 * 描述  ：I2C 工作模式配置
 * 输入  ：无
 * 输出  ：无
 * 调用  ：内部调用
 */
static void I2C_Mode_Config(void)
{
  I2C_InitTypeDef  I2C_InitStructure; 

  /* I2C 配置 */
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = I2C_BUS_NUM_SLAVE_ADDRESS7;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_Speed;
  
  /* 使能 I2C_BUS_NUM */
  I2C_Cmd(I2C_BUS_NUM, ENABLE);

  /* I2C_BUS_NUM 初始化 */
  I2C_Init(I2C_BUS_NUM, &I2C_InitStructure);

  /*允许1字节1应答模式*/
  I2C_AcknowledgeConfig(I2C_BUS_NUM, ENABLE);    
}

struct rt_i2c_bus_device stm32_bus;
const char stm32_bus_name[] = "I2C";

rt_err_t rt_i2c_stm32_add_bus(void)
{
  rt_err_t ret;
  struct rt_i2c_stm32_ops *stm32_ops = stm32_bus.priv;
  //RT_ASSERT(stm32_ops != RT_NULL);

  ret=rt_i2c_core_init();
  if (ret != RT_EOK)
  {
    stm32_err("core_init err.\n");
    return RT_ERROR;
  }

  stm32_bus.ops = &i2c_stm32_bus_ops;

  I2C_GPIO_Config(); 

  I2C_Mode_Config();


  stm32_dbg("add_bus:%s, bus register=0x%x\n", stm32_bus_name, I2C_BUS_NUM);
  return rt_i2c_bus_device_register(&stm32_bus, stm32_bus_name);
}