/*
 * File      : adc_stm32.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author        Notes
 * 2013-04-25     studyboy_3w         first version
 */

#include <rtdevice.h>
#include <stm32f10x.h>


#if 1
#define stm32_dbg(fmt, ...)   do{rt_kprintf("stm adc:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(0)
#else
#define stm32_dbg(fmt, ...)
#endif
#define stm32_err(fmt, ...)   do{rt_kprintf("[ERR] stm adc:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(0)

#define ADC1_DR_Address    ((u32)0x4001244C)
#define SAMPLE_COUNT    (16)
static volatile uint16_t ADC_DMA_BUFFER[SAMPLE_COUNT];

#define MAX_VOLTAGE 3300  //3.3V input Vef+

static struct rt_mutex adc_lock;
static int user_cnt=0;
struct rt_adc_cfg
{
    int mode;
};

static rt_err_t rt_adc_init(rt_device_t dev)
{
    //struct rt_adc *bus = (struct rt_adc *)dev->user_data;
    //RT_ASSERT(bus != RT_NULL);

    stm32_dbg("adc init\n");
    return RT_EOK;
}

static rt_err_t rt_adc_cfg(struct rt_device *dev, struct rt_adc_cfg * cfg)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    // RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
    // /* Configure PC.04 (ADC Channel—14) as analog input*/
    // GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;             
    // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;         
    // GPIO_Init(GPIOC, &GPIO_InitStructure);                



    // RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE );
    // /* Time Base configuration */
    // ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;//独立工作模式
    // ADC_InitStructure.ADC_ScanConvMode = ENABLE;//扫描方式
    // ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;//连续转换
    // ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;//外部触发禁止
    // ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//数据右对齐
    // ADC_InitStructure.ADC_NbrOfChannel = 1;//用于转换的通道数
    // ADC_Init(ADC1, &ADC_InitStructure);
    // ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 1, ADC_SampleTime_55Cycles5);  //采样时间为55.5周期 



    ///////////////////////////
    /* Enable DMA clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* Enable ADC1 and GPIOC clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOC, ENABLE);

    /* Configure PC.01  as analog input */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOC, &GPIO_InitStructure);                // PC1,输入时不用设置速率

    /* DMA channel1 configuration */
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)(ADC_DMA_BUFFER);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = SAMPLE_COUNT;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);

    /* Enable DMA channel1 */
    DMA_Cmd(DMA1_Channel1, ENABLE);

    /* ADC1 configuration */
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* ADC1 regular channel11 configuration */ 
    ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 1, ADC_SampleTime_55Cycles5);

    return RT_EOK;
}

static rt_err_t rt_adc_start(struct rt_device *dev)
{

    /* Enable ADC1 DMA */
    ADC_DMACmd(ADC1, ENABLE);

    /* Enable ADC1 */
    ADC_Cmd(ADC1, ENABLE);

    /* Enable ADC1 reset calibaration register */   
    ADC_ResetCalibration(ADC1);
    /* Check the end of ADC1 reset calibration register */
    while(ADC_GetResetCalibrationStatus(ADC1));

    /* Start ADC1 calibaration */
    ADC_StartCalibration(ADC1);
    /* Check the end of ADC1 calibration */
    while(ADC_GetCalibrationStatus(ADC1));

    /* Start ADC1 Software Conversion */ 
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    stm32_dbg("adc start\n");
    return RT_EOK;
}

static rt_err_t rt_adc_stop(struct rt_device *dev)
{

    /* Enable ADC1 DMA */
    ADC_DMACmd(ADC1, DISABLE);
    ADC_Cmd(ADC1, DISABLE);      
    stm32_dbg("adc stop\n");
    return RT_EOK;
}

static rt_err_t rt_adc_open(struct rt_device *dev, rt_uint16_t oflag)
{

    rt_mutex_take(&adc_lock, RT_WAITING_FOREVER);
    if(!user_cnt)
    {
        rt_adc_cfg(dev, RT_NULL);
        rt_adc_start(dev);
    }
    user_cnt++;
    rt_mutex_release(&adc_lock);
    return RT_EOK;
}

static rt_err_t rt_adc_close(struct rt_device *dev)
{

    rt_mutex_take(&adc_lock, RT_WAITING_FOREVER);
    user_cnt--;
    if(!user_cnt)
    {
        rt_adc_stop(dev);
    }
    rt_mutex_release(&adc_lock);
    return RT_EOK;
}

static uint32_t adc_get_value(uint8_t ch)
{
    uint32_t sum = 0;
    uint32_t i;

    for(i = 0; i < SAMPLE_COUNT; i++)
    {
        sum += ADC_DMA_BUFFER[i];
    }

    return (uint32_t ) (sum / SAMPLE_COUNT);
}

static rt_size_t rt_adc_read(rt_device_t dev,
                                     rt_off_t    pos,
                                     void       *buffer,
                                     rt_size_t   count)
{
    rt_size_t   i;
    uint32_t *p=(uint32_t *) buffer;
    RT_ASSERT(buffer != RT_NULL);

    rt_mutex_take(&adc_lock, RT_WAITING_FOREVER);
    for(i=0; i<count; i++, p++)
    {
        *p=adc_get_value(0)*3300/4096;
        stm32_dbg("adc[%d] value=%d\n", i,*p);
    }   
    rt_mutex_release(&adc_lock);

    return count;
}


static rt_err_t rt_adc_control(rt_device_t dev,
                                       rt_uint8_t  cmd,
                                       void       *args)
{
    //rt_err_t ret;


    // switch (cmd)
    // {
    // /* set 10-bit addr mode */
    // case RT_adc_DEV_CTRL_10BIT:
    //     bus->flags |= RT_adc_ADDR_10BIT;
    //     break;
    // case RT_adc_DEV_CTRL_ADDR:
    //     bus->addr = *(rt_uint16_t *)args;
    //     break;
    // case RT_adc_DEV_CTRL_TIMEOUT:
    //     bus->timeout = *(rt_uint32_t *)args;
    //     break;
    // case RT_adc_DEV_CTRL_RW:
    //     priv_data = (struct rt_adc_priv_data *)args;
    //     ret = rt_adc_transfer(bus, priv_data->msgs, priv_data->number);
    //     if (ret < 0)
    //     {
    //         return -RT_EIO;
    //     }
    //     break;
    // default:
    //     break;
    // }
    stm32_dbg("adc control\n");
    return RT_EOK;
}
struct rt_device stm32_adc_device;
rt_err_t rt_adc_device_init(void)
{
    struct rt_device *device=&stm32_adc_device;

    //device->user_data = bus;

    /* set device type */
    device->type    = RT_Device_Class_Char;
    /* initialize device interface */
    device->init    = rt_adc_init;
    device->open    = rt_adc_open;
    device->close   = rt_adc_close;
    device->read    = rt_adc_read;
    device->write   = RT_NULL;
    device->control = rt_adc_control;

    /* register to device manager */
    rt_device_register(device, "ADC1", RT_DEVICE_FLAG_RDWR);

    rt_mutex_init(&adc_lock, "adc_lock", RT_IPC_FLAG_FIFO);
    return RT_EOK;
}
