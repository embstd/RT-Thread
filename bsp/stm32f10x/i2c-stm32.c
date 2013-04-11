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
 * 2012-04-25     weety         first version
 */

#include <rtdevice.h>
#include <stm32f10x.h>
#include <stm32f10x_i2c.h>
#include <stm32f10x_rcc.h>
#include <i2c.h>
#include <i2c_dev.h>

#define EEPROM_ADDRESS 0xA0

#define I2C_Speed              400000
#define I2C1_SLAVE_ADDRESS7    0xA0


#if 1
#define stm32_dbg(fmt, ...)   do{rt_kprintf("stm i2c:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(1)
#endif
#define stm32_err(fmt, ...)   do{rt_kprintf("[ERR] stm i2c:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(1)

struct rt_i2c_stm32_ops
{
    void *data;            /* private data for lowlevel routines */
};


static void i2c_start(struct rt_i2c_stm32_ops *ops)
{
  /* Send STRAT condition */
    I2C_GenerateSTART(I2C1, ENABLE);
}

static void i2c_restart(struct rt_i2c_stm32_ops *ops)
{
    /* Send STRAT condition */
    I2C_GenerateSTART(I2C1, ENABLE);  
}

static void i2c_stop(struct rt_i2c_stm32_ops *ops)
{
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
  /* Send STOP condition */
  I2C_GenerateSTOP(I2C1, ENABLE);
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
  rt_int32_t count = msg->len;
  rt_uint16_t ignore_nack = msg->flags & RT_I2C_IGNORE_NACK;

  stm32_dbg("send_bytes\n");
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  while (count --) 
  {
    stm32_dbg("send_bytes: count=%d, data=0x%x\n",count,*ptr);
  /* Send the current byte */
    I2C_SendData(I2C1, *ptr); 

  /* Point to the next byte to be written */
    ptr++; 

  /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  };

  return msg->len-count;
}

static rt_err_t i2c_send_ack_or_nack(struct rt_i2c_bus_device *bus, int ack)
{
  if (ack)
    I2C_AcknowledgeConfig(I2C1, ENABLE);
  else
    I2C_AcknowledgeConfig(I2C1, DISABLE);
  return RT_EOK;
}

static rt_size_t i2c_recv_bytes(struct rt_i2c_bus_device *bus,
                                struct rt_i2c_msg        *msg)
{
  rt_int32_t val;
  rt_int32_t bytes        = 0;   /* actual bytes */
  rt_uint8_t *ptr         = msg->buf;
  rt_int32_t count        = msg->len;
  const rt_uint32_t flags = msg->flags;
  stm32_dbg("recv_bytes\n");
/* While there is data to be read */
  while(count--)  
  {

    if (!(flags & RT_I2C_NO_READ_ACK))
    {
      val                     = i2c_send_ack_or_nack(bus, count);
      if (val < 0)
      {
        stm32_err("recv_bytes:No Read Ack.\n");
        return val;
      }
    }

/* Test on EV7 and clear it */
    if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))  
    {      
/* Read a byte from the EEPROM */
      *ptr                    = I2C_ReceiveData(I2C1);

/* Point to the next location where the byte read will be saved */
      ptr++; 
    }   
  }
/* Enable Acknowledgement to be ready for another reception */
  I2C_AcknowledgeConfig(I2C1, ENABLE);

  return msg->len-count;
}

static rt_int32_t i2c_send_address(struct rt_i2c_bus_device *bus,
                                   rt_uint8_t                addr,
                                   rt_int32_t                retries)
{

  stm32_dbg("send_address\n");
    /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));  

    /* Send EEPROM address for write */
  I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Transmitter);

    /* Test on EV6 and clear it */
    //while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    /* Send the EEPROM's internal address to write to */
    //I2C_SendData(I2C1, addr);

  return 0;
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
/*
    if (flags & RT_I2C_ADDR_10BIT)
    {
        addr1 = 0xf0 | ((msg->addr >> 7) & 0x06);
        addr2 = msg->addr & 0xff;

        stm32_dbg("addr1: %d, addr2: %d\n", addr1, addr2);

        ret = i2c_send_address(bus, addr1, retries);
        if ((ret != 1) && !ignore_nack)
        {
            stm32_dbg("NACK: sending first addr\n");

            return -RT_EIO;
        }

        ret = i2c_writeb(bus, addr2);
        if ((ret != 1) && !ignore_nack)
        {
            stm32_dbg("NACK: sending second addr\n");

            return -RT_EIO;
        }
        if (flags & RT_I2C_RD)
        {
            stm32_dbg("send repeated start condition\n");
            i2c_restart(ops);
            addr1 |= 0x01;
            ret = i2c_send_address(bus, addr1, retries);
            if ((ret != 1) && !ignore_nack)
            {
                stm32_dbg("NACK: sending repeated addr\n");

                return -RT_EIO;
            }
        }
    } */
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

    stm32_dbg("send start condition\n");
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
    stm32_dbg("send stop condition\n");
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
 * 描述  ：I2C1 I/O配置
 * 输入  ：无
 * 输出  ：无
 * 调用  ：内部调用
 */
static void I2C_GPIO_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure; 

  /* 使能与 I2C1 有关的时钟 */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);  
    
  /* PB6-I2C1_SCL、PB7-I2C1_SDA*/
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;        // 开漏输出
  GPIO_Init(GPIOB, &GPIO_InitStructure);
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
  I2C_InitStructure.I2C_OwnAddress1 = I2C1_SLAVE_ADDRESS7;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_Speed;
  
  /* 使能 I2C1 */
  I2C_Cmd(I2C1, ENABLE);

  /* I2C1 初始化 */
  I2C_Init(I2C1, &I2C_InitStructure);

  /*允许1字节1应答模式*/
  I2C_AcknowledgeConfig(I2C1, ENABLE);    
}

struct rt_i2c_bus_device stm32_bus;
const char stm32_bus_name[] = "I2C1";

rt_err_t rt_i2c_stm32_add_bus(void)
{
  struct rt_i2c_stm32_ops *stm32_ops = stm32_bus.priv;
  //RT_ASSERT(stm32_ops != RT_NULL);

  rt_i2c_core_init();

  stm32_bus.ops = &i2c_stm32_bus_ops;

  I2C_GPIO_Config(); 

  I2C_Mode_Config();


  stm32_dbg("add_bus:%s\n", stm32_bus_name);
  return rt_i2c_bus_device_register(&stm32_bus, stm32_bus_name);
}