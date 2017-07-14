#include "delay.h"
#include "sys.h"
//���ʹ��OS,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"					//FreeRTOSʹ��	 
#include "task.h"
#endif

static u32 fac_us=0;							//us��ʱ������

#if SYSTEM_SUPPORT_OS		
    static u16 fac_ms=0;				        //ms��ʱ������,��os��,����ÿ�����ĵ�ms��
#endif

#define	SYSTEM_TIME_COF_US		(1000000/configTICK_RATE_HZ)
#define	SYSTEM_TIME_TICK_LOAD	(configCPU_CLOCK_HZ/configTICK_RATE_HZ)
#define	SYSTEM_TIME_TICK_US		(configCPU_CLOCK_HZ/1000000)

extern void xPortSysTickHandler(void);

//systick�жϷ�����,ʹ��OSʱ�õ�
void SysTick_Handler(void)
{	
    if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)//ϵͳ�Ѿ�����
    {
        xPortSysTickHandler();	
    }
    HAL_IncTick();
}


//��ʼ���ӳٺ���
//��ʹ��ucos��ʱ��,�˺������ʼ��ucos��ʱ�ӽ���
//SYSTICK��ʱ�ӹ̶�ΪAHBʱ�ӵ�1/8
//SYSCLK:ϵͳʱ��Ƶ��
void delay_init(u8 SYSCLK)
{
	u32 reload;
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);//SysTickƵ��ΪHCLK
	fac_us=SYSCLK;						    //�����Ƿ�ʹ��OS,fac_us����Ҫʹ��
	reload=SYSCLK;					        //ÿ���ӵļ������� ��λΪK	   
	reload*=1000000/configTICK_RATE_HZ;		//����delay_ostickspersec�趨���ʱ��
											//reloadΪ24λ�Ĵ���,���ֵ:16777216,��216M��,Լ��77.7ms����	
	fac_ms=1000/configTICK_RATE_HZ;			//����OS������ʱ�����ٵ�λ	   
	SysTick->CTRL|=SysTick_CTRL_TICKINT_Msk;//����SYSTICK�ж�
	SysTick->LOAD=reload; 					//ÿ1/OS_TICKS_PER_SEC���ж�һ��	
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk; //����SYSTICK
}								    

//��ʱnus
//nus:Ҫ��ʱ��us��.	
//nus:0~19884107(���ֵ��2^32/216)��19.884107��
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;				//LOAD��ֵ	    	 
	ticks=nus*fac_us; 						//��Ҫ�Ľ����� 
	told=SysTick->VAL;        				//�ս���ʱ�ļ�����ֵ
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;	//����ע��һ��SYSTICK��һ���ݼ��ļ������Ϳ�����.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;			//ʱ�䳬��/����Ҫ�ӳٵ�ʱ��,���˳�.
		}  
	};									    
}  
 
//��ʱnms,�������������
//nms:Ҫ��ʱ��ms��
//nms:0~65535
void delay_ms(u32 nms)
{	
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)//ϵͳ�Ѿ�����
	{		
		if(nms>=fac_ms)						//��ʱ��ʱ�����OS������ʱ������ 
		{ 
   			vTaskDelay(nms/fac_ms);	 		//FreeRTOS��ʱ
		}
		nms%=fac_ms;						//OS�Ѿ��޷��ṩ��ôС����ʱ��,������ͨ��ʽ��ʱ    
	}
	delay_us((u32)(nms*1000));				//��ͨ��ʽ��ʱ
}

//��ʱnms,���������������
//nms:Ҫ��ʱ��ms��
void delay_xms(u32 nms)
{
	u32 i;
	for(i=0;i<nms;i++) delay_us(1000);
}

uint64_t GetSystemTimeUs(void)
{
	uint32_t tempload = 0;
	uint64_t result = 0;
	tempload = xTaskGetTickCount();
	result = ((uint64_t)tempload)*SYSTEM_TIME_COF_US;
	tempload = SYSTEM_TIME_TICK_LOAD - SysTick->VAL; 
	tempload /= SYSTEM_TIME_TICK_US;
	result += tempload;
	return result;
}

uint32_t GetSystemTimeMs(void)
{
	uint32_t result = 0;
	result = GetSystemTimeUs()/1000;
	return result;
}
