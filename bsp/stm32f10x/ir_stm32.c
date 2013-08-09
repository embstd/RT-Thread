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

#if 1
#define stm32_dbg(fmt, ...)   do{rt_kprintf("stm ir:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(0)
#else
#define stm32_dbg(fmt, ...)
#endif
#define stm32_err(fmt, ...)   do{rt_kprintf("[ERR] stm ir:"); rt_kprintf(fmt, ##__VA_ARGS__); }while(0)
 


static void RCC_Configuration( void );
static void GPIO_Configuration( void );
static void NVIC_Configuration( void );


void InputCaptureInit( void )
{
 TIM_ICInitTypeDef  TIM_ICInitStructure;
 TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
 
 RCC_Configuration();
 NVIC_Configuration();
 GPIO_Configuration();
 
 TIM_TimeBaseInitStructure.TIM_Period        = 0xffff;   // 16位计数
 TIM_TimeBaseInitStructure.TIM_Prescaler     = 72*2-1;   // 144分频 2us
 TIM_TimeBaseInitStructure.TIM_ClockDivision = 0;        // 不分割
 TIM_TimeBaseInitStructure.TIM_CounterMode   = TIM_CounterMode_Up; // 上升计数
 TIM_TimeBaseInit( TIM2, &TIM_TimeBaseInitStructure );
 TIM_ITConfig( TIM2, TIM_IT_Update, DISABLE );
 TIM_ClearFlag(TIM2, TIM_FLAG_Update);
 
 TIM_ICInitStructure.TIM_Channel     = TIM_Channel_2;   // 选择通道2
 //TIM_ICInitStructure.TIM_Channel     = TIM_Channel_1;   // 选择通道2
 TIM_ICInitStructure.TIM_ICPolarity  = TIM_ICPolarity_Falling;  // 下降沿触发
 TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;   //
 TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
 TIM_ICInitStructure.TIM_ICFilter    = 0x0; 
 TIM_ICInit( TIM2, &TIM_ICInitStructure );
 TIM_ITConfig( TIM2, TIM_IT_CC2, DISABLE );
 TIM_ClearFlag( TIM2, TIM_FLAG_CC2 );
 
 TIM_Cmd( TIM2, ENABLE );
 TIM_ITConfig( TIM2, TIM_IT_CC2 | TIM_IT_Update, ENABLE );

 stm32_dbg( "IR Initial.... DONE.\n\n");
}

// 使能TIM2和GPIOA的时钟
static void RCC_Configuration( void )
{
 RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );
 RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE );
}

// 设置PA1
static void GPIO_Configuration( void )
{
 GPIO_InitTypeDef GPIO_InitStructure;
 
 GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_1;
 //GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
 GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

 GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static uint8_t check_ir_key(uint32_t ir)
{
    uint8_t factory_id, key_id;
    key_id=(ir>>8)&0xff;
    factory_id=(ir>>24)&0xff;
    if( (key_id+(ir&0xff)) != 0xff )
    {
        stm32_err("check key ID err, 0x%x,0x%x \n",key_id, ir&0xff);
        return 1;
    }
    //There are no factory id for some ir remote, in that case, check 16-24bit as id.
    if(factory_id)
    {
      if( (factory_id+(ir>>16)&0xff) != 0xff )
      {
          stm32_err("check factory ID err, 0x%x,0x%x \n",factory_id, ((ir>>16)&0xff));
          
      }
    }
    else
    {
      factory_id=(ir>>16)&0xff;
    }
    stm32_dbg( "IR Key is : factory_id=0x%x, key_id=0x%x\n\n", factory_id,key_id);
    return 0;
}

#define MAX_Pluse 60
uint16_t IR_PluseArry[MAX_Pluse], PluseInd=0;
int16_t IR_KEY_LED_ID=1;
int16_t GET_IR_KEY_LED_ID(void)
{

  if(IR_KEY_LED_ID)
  { 
    IR_KEY_LED_ID--;
    return IR_KEY_LED_ID+1;
  }
  else
    return IR_KEY_LED_ID;  
}


static void check_pluse_array(uint8_t id)
{
    uint8_t i,j;
    // for(i=0; i < MAX_Pluse; i++)
    // {
    //   if(!(i%4))
    //     stm32_dbg( "\n");
    //   stm32_dbg( "[%d]=%4d \t", i, IR_PluseArry[i]);
    // }
    // stm32_dbg( "\n");

    stm32_dbg( "IR_PluseArry DUMP [%d].\n", id);
    for(i=id+1, j=0; j < MAX_Pluse; i++,j++)
    {
      i=i%MAX_Pluse;
      if(!(j%4))
        stm32_dbg( "\n");
      stm32_dbg( "[%d]=%4d \t", i, IR_PluseArry[i]);
    }
    stm32_dbg( "\n");
}

// 设置NVIC

static void NVIC_Configuration( void )
{
 NVIC_InitTypeDef NVIC_InitStructure;

 /* Enable the TIM2 global Interrupt */
 NVIC_InitStructure.NVIC_IRQChannel        = TIM2_IRQn;
 NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
 NVIC_InitStructure.NVIC_IRQChannelSubPriority       = 10;
 NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
 NVIC_Init(&NVIC_InitStructure);
}

// static void delay_ms(rt_uint32_t ms)
// {
//   rt_uint32_t len;
//   for (;ms > 0; ms --)
//     for (len = 0; len < 100; len++ );
// }

//解码部分：遥控的键值存放在IR_Key变量中

volatile uint32_t IR_Key;
void TIM2_IRQHandler( void )
{
 static uint16_t IR_LastPluse = 0;
 static uint8_t  IR_Sta           = 0;
 static uint32_t IR_Code       = 0;
 static uint8_t  IR_PluseCnt   = 0;
 static uint8_t  IR_Up            = 0;
 uint16_t IR_ThisPluse;
 uint16_t IR_PluseSub;
 
//stm32_dbg( "TIM2 TIM2_IRQHandler ..........\n");
 if ( TIM_GetITStatus( TIM2, TIM_IT_CC2 ) == SET )
 {
  TIM_ClearFlag( TIM2, TIM_IT_CC2 );
  IR_Up = 0;
  IR_ThisPluse = TIM_GetCapture2( TIM2 );
  
  //stm32_dbg( "IR_ThisPluse is : %d, IR_LastPluse %d\n\n", IR_ThisPluse,IR_LastPluse );
  if ( IR_ThisPluse > IR_LastPluse )
  {
    IR_PluseSub = IR_ThisPluse - IR_LastPluse;
  }
  else {
    IR_PluseSub = 0xffff - IR_LastPluse + IR_ThisPluse;
  }
  
  //stm32_dbg( "IR_PluseSub is : %d IR_PluseCnt %d\n\n", IR_PluseSub,IR_PluseCnt);
  IR_LastPluse = IR_ThisPluse;
  IR_PluseCnt++;

  PluseInd++ ;
  PluseInd=PluseInd%MAX_Pluse;
  IR_PluseArry[PluseInd] = IR_PluseSub;

  if ( IR_PluseCnt == 2 )
  {
     //if (( IR_PluseSub > 5000 ) && ( IR_PluseSub < 8000 )) //NEC 引导码
      //if (( IR_PluseSub > 4000 ) && ( IR_PluseSub < 5000 )) //TC9012 引导码
    if (( IR_PluseSub > 4000 ) && ( IR_PluseSub < 80000 ))
     {
      IR_Sta = 0x01;
     }
  }
  else if ( IR_Sta & 0x01 ) // 如果引导成功，开始接码
  {
     if (( IR_PluseSub < 450 ) || ( IR_PluseSub > 1300 )) //error
     {
      IR_Sta          = 0;
      IR_PluseCnt  = 0;
      IR_Code       = 0;
     }
     else
     {
        IR_Code <<= 1;
        if (( IR_PluseSub > 900 ) && ( IR_PluseSub < 1300 )) //1
        {
         IR_Code |= 0x01;
        }
        
        if ( IR_PluseCnt == 34 )
        {
           IR_Key = IR_Code;
           IR_Sta = 0x02;
           stm32_dbg( "IR Down is : 0x%8x, \n\n", IR_Key );
           check_ir_key(IR_Key);
           IR_KEY_LED_ID++;
           //check_pluse_array(PluseInd);
        }
        // else if (IR_PluseCnt >= 33)
        // {
        //   stm32_dbg( "IR 33 ");
        //   check_pluse_array(PluseInd);
        // }
     }
  }
  else if ( IR_Sta & 0x02 )
  {
   switch ( IR_PluseCnt )
   {
    case 35:
    {
     if ( ( IR_PluseSub < 4500 ) || ( IR_PluseSub > 7000 ) )
     {
      IR_PluseCnt--;
     }
     break;
    }
    case 36:
    {
     IR_PluseCnt = 34;
     if ( ( IR_PluseSub > 45000) && ( IR_PluseSub < 60000 ) )
     { 
      IR_Key = IR_Code;
      stm32_dbg( "IR continue is : 0x%8x\n\n", IR_Key );
      check_ir_key(IR_Key);
      IR_KEY_LED_ID++;
      //check_pluse_array(PluseInd);
     }
     break;
    }
   }
  }
 }
 
 if ( TIM_GetITStatus( TIM2, TIM_IT_Update ) == SET )
 {
  TIM_ClearFlag( TIM2, TIM_IT_Update ); 
  if ( GPIO_ReadInputDataBit( GPIOA, GPIO_Pin_1 ) == SET )
  {
   IR_Up++;
   if ( IR_Up >= 2 )
   {
    IR_Code     = 0;
    IR_Sta      = 0;
    IR_PluseCnt = 0;
   }
  }
 } 
}



#ifdef RT_USING_FINSH
#include <finsh.h>
uint32_t ir(uint32_t f, uint32_t key)
{
    InputCaptureInit();

}
FINSH_FUNCTION_EXPORT(ir, ir remote)
#endif



