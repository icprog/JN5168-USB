

/*****************************************************************************
      Include files
*****************************************************************************/
/* Stack includes */
#include <Api.h>
#include <JIP.h>
#include <6LP.h>
/* JenOS includes */
#include <dbg.h>
#include <dbg_uart.h>
/* Application includes */
#include "Config.h"
#include "DeviceDefs.h"
#include "Exception.h"
#include "MibDio.h"
#include "MibDioControl.h"

/**********************************************************************************************************
   MIB structures
**********************************************************************************************************/
#if MK_BLD_MIB_DIO_CONTROL
extern tsMibDioControl	 sMibDioControl;
extern thJIP_Mib		 hMibDioControl;
#endif


/**********************************************************************************************************
    Local Variables
**********************************************************************************************************/
PRIVATE uint8   u8TickQueue;											/* Tick timer queue 			*/
PRIVATE uint8   u8Tick;	   												/* Tick counter 				*/
PUBLIC  uint32 u32Second;  												/* Second counter 				*/
PRIVATE uint8  turn_flag = 0;
PRIVATE uint8  SleepNum = 0;
PUBLIC  uint32 ED_SLEEP = 5000;

/**********************************************************************************************************
    Other data
**********************************************************************************************************/
PRIVATE bool_t   bInitialised;
PRIVATE bool_t   bSleep;
/**********************************************************************************************************
    Exported Functions
**********************************************************************************************************/
PUBLIC void Device_vInit(bool_t bWarmStart);
PUBLIC void Device_vTick(void);
PUBLIC void Node_eJipInit(void);
PUBLIC void Device_vSleep(void);

/**********************router code********************************/
PRIVATE uint16   buf[10]={0,0,0,0,0,0,0,0,0,0};
PRIVATE int16   x,y,z;
//PRIVATE	int16   angle;
PRIVATE int16   u16AdcReading;
PRIVATE	int16   u16BattLevelmV;
//PRIVATE float   angle_float = 0.0;

PRIVATE float temperature ;
PRIVATE int32 pressure ;
PRIVATE int16 ac1 = 9002;
PRIVATE int16 ac2 = -72;
PRIVATE int16 ac3 = -14383;
PRIVATE uint16 ac4 = 32741;
PRIVATE uint16 ac5 = 32757;
PRIVATE uint16 ac6 = 23153;
PRIVATE int16 b1 = 6190;
PRIVATE int16 b2 = 4;
PRIVATE int16 mb = -32767;
PRIVATE int16 mc = -8711;
PRIVATE int16 md = 2868;
PRIVATE int32 b5;
PRIVATE uint16 chip_id ;
PRIVATE const uint8 OSS = 0;  // Oversampling Setting
PRIVATE uint16 ut;
PRIVATE uint32 up = 0;
PRIVATE void bmp085Calibration(void);
PRIVATE float bmp085GetTemperature(uint16 ut);
PRIVATE int32 bmp085GetPressure(uint32 up);
PRIVATE uint16 bmp085ReadUT();
PRIVATE uint32 bmp085ReadUP();
PRIVATE void delay_nms(uint16 time);
PRIVATE uint16 T_trans = 0;
PRIVATE uint16 P_trans = 0;

//PRIVATE bool_t   bLight=TRUE;

PUBLIC uint8 I2C_ReadByte(uint8 addr,uint8 reg_addr);
PUBLIC void I2C_WriteByte(uint8 addr,uint8 reg_addr,uint8 data);

//PRIVATE void vSpiInit(void);
//PRIVATE uint8 u8SpiRwByte(uint8 ucDat);
//PRIVATE void vSpiSendCmd(uint8 ucCmd);

#define UART E_AHI_UART_1
#define adress 0x77
//PRIVATE void vPutChar(unsigned char c) ;
//PRIVATE void vUartInit(void);
PRIVATE void vGetADXL335value1(void);
PRIVATE void vGetADXL335value2(void);
PRIVATE void vGetADXL335value3(void);
PRIVATE void HMC5883_Init(void);

PRIVATE uint32 light_bit = 0x0400 ;
PRIVATE uint8 AD_flag = 0;
PRIVATE uint16 i = 0;
PRIVATE uint16 j = 0;
PRIVATE uint8 delay_num = 0;
/**********************router code********************************/

// 使用6lp相应API发送数据， 发送端只需要调用6LP相应的数据发送API向指定的地址端口发送数据即可。

#define PROTOCOL_PORT				 	 (1190)
#define EndNum         0x5301

PRIVATE bool_t          	  bProtocolSocket;
PRIVATE int					  iProtocolSocket;
PRIVATE ts6LP_SockAddr 		  s6lpSockAddrLocal;
PRIVATE void Protocol_vOpenSocket(void);
PRIVATE uint16  UserData[100] = {0} ;

/**********************************************************************************************************
	远程设备地址
**********************************************************************************************************/
MAC_ExtAddr_s sCommNodeAddr;

/**********************************************************************************************************
** Function name:     SendDataToRemote
** Descriptions:      发送数据范例函数
**********************************************************************************************************/

PRIVATE void Protocol_vOpenSocket(void)
{

		iProtocolSocket = i6LP_Socket(E_6LP_PF_INET6,
									  E_6LP_SOCK_DGRAM,
									  E_6LP_PROTOCOL_ONLY_ONE);

		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\n iProtocolSocket %d", iProtocolSocket);

		if (iProtocolSocket == -1)
		{
			DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\n get iProtocolSocket failed ");
		}
		/* Registered socket ? */
		else
		{
			int			   iBindResult;
			int 		   iSocketAddGroupAddr;

			/* Socket has been opened */
			bProtocolSocket = TRUE;

			/* Get own address */
			(void) i6LP_GetOwnDeviceAddress(&s6lpSockAddrLocal, FALSE);
			/* Set socket port in address */
			s6lpSockAddrLocal.sin6_port = PROTOCOL_PORT;

			/* Bind socket to address (including application port) */
			iBindResult = i6LP_Bind(iProtocolSocket, &s6lpSockAddrLocal, sizeof(ts6LP_SockAddr));

			/* Failed ? */
			if (iBindResult != 0)
			{
				DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\n i6LP_Bind Failed");

			}

			/* Bind the group address to the socket */
//			iSocketAddGroupAddr = i6LP_SocketAddGroupAddr(iProtocolSocket, &s6lpSockAddrNetwork.sin6_addr);
//
//			if (iSocketAddGroupAddr == -1)
//			{
//				DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\n i6LP_SocketAddGroupAddr failed");
//			}
		}


}


PRIVATE void Protocol_vSendTo(ts6LP_SockAddr *ps6lpSockAddr, uint8 *pu8Data, uint16 u16DataLen)
{
	int iGetDataBufferResult;
	int iSendToResult;
	uint8 *pu8Buffer;


	/* Try to get a data buffer */
	iGetDataBufferResult = i6LP_GetDataBuffer(&pu8Buffer);

	if (iGetDataBufferResult == -1)
	{

	}
	else
	{

		memcpy(pu8Buffer, pu8Data, u16DataLen);

		/* Transmit packet */
		iSendToResult = i6LP_SendTo(iProtocolSocket, pu8Buffer, u16DataLen, 0, ps6lpSockAddr, sizeof(ts6LP_SockAddr));

		if (iSendToResult == -1)
		{

		}
		else
		{
			 if(turn_flag == 0)
					{
						vAHI_DioSetOutput(0 , 0x0400);
						turn_flag = 1;
					}
					else
					{
						vAHI_DioSetOutput(0x0400 , 0);
						turn_flag = 0;
					}
		}
	}
}

PRIVATE void SendDataToRemote(uint32 *sendData)
{
	ts6LP_SockAddr s6LP_SockAddr;										/* 定义ts6LP_SockAddr 		    */
	EUI64_s  sIntAddr;
	uint32 val;
	MAC_ExtAddr_s *psMacAddr;

//	sCommNodeAddr.u32H = 0x158d00;
//	sCommNodeAddr.u32L = 0x35ccc1;

	psMacAddr = &sCommNodeAddr;											/* sCommNodeAddr在 				*/
																		/* vJIP_StackEvent赋值    			*/
	memset(&s6LP_SockAddr, 0, sizeof(ts6LP_SockAddr));					/* 清空s6LP_SockAddr结构体内存值 	*/
	i6LP_CreateInterfaceIdFrom64(&sIntAddr, (EUI64_s *) psMacAddr);		/* 建立接口ID 					*/
	i6LP_CreateLinkLocalAddress (&s6LP_SockAddr.sin6_addr, &sIntAddr);	/* 建立连接本地地址 				*/
	/*
	 * Complete full socket address
	 */
	s6LP_SockAddr.sin6_family = E_6LP_PF_INET6;							/* 经常设置为E_6LP_PF_INET6		*/
	s6LP_SockAddr.sin6_flowinfo =0;										/* 包含flow信息 					*/
	s6LP_SockAddr.sin6_port = PROTOCOL_PORT;							/* IPV6端口号 					*/
	s6LP_SockAddr.sin6_scope_id =0;										/* 包含scope信息					*/

	val = sendData;
	Protocol_vSendTo(&s6LP_SockAddr, sendData, 20);

}
/**********************************************************************************************************
** Function name:     vJIP_Remote_DataSent
** Descriptions:      设置远程数据请求
** Input parameters:  ts6LP_SockAddr： 地址参数
**             		  teJIP_Status:  请求状态，E_JIP_OK请求成功，否则失败
** Output parameters: none
** Returned value:          none
**********************************************************************************************************/
PUBLIC void vJIP_Remote_DataSent(ts6LP_SockAddr *psAddr,
									   teJIP_Status eStatus)
{
	if (eStatus == E_JIP_OK) {
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\nMib Request Send Success");

	} else {
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\nMib Request Send Failed");
	}
}
/**********************************************************************************************************
** Function name:    vJIP_Remote_SetResponse
** Descriptions:     设置远程数据响应
** Input parameters: *psAddr: 地址参数
**			   		 u8Handle: 句柄
**			   		 u8MibIndex:MIB索引
**			   		 u8VarIndex:MIB变量索引
**             		 eStatus:  响应状态，E_JIP_OK请求成功，否则失败
** Output parameters: none
** Returned value:          none
**********************************************************************************************************/
PUBLIC WEAK void vJIP_Remote_SetResponse(ts6LP_SockAddr *psAddr,
        uint8 u8Handle,
        uint8 u8MibIndex,
        uint8 u8VarIndex,
        teJIP_Status eStatus)
{
	if (eStatus == E_JIP_OK) {
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\nMib Request Process Success");
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\nu8MibIndex:%d",u8MibIndex);
	} else {
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\nMib Request Process Failed");
	}
}


/**********************************************************************************************************
** Function name:     AppColdStart
** Descriptions:      程序入口，冷启动，上电重启时候入口
** Input parameters:  none
** Output parameters: none
** Returned value:          none
**********************************************************************************************************/
PUBLIC void AppColdStart(void)
{
	/*
	 * Initialise device
	 */
	Device_vInit(FALSE);
}

/**********************************************************************************************************
** Function name:     AppWarmStart
** Descriptions:      memory hold睡眠， 唤醒入口
** Input parameters:  none
** Output parameters: none
** Returned value:          none
**********************************************************************************************************/
PUBLIC void AppWarmStart(void)
{
	/*
	 * Initialise device
	 */
	Device_vInit(TRUE);
}

/**********************************************************************************************************
** Function name:     Device_vInit
** Descriptions:      设备初始化函数
** Input parameters:  bWarmStart：是否冷启动，TRUE热启动，FALSE冷启动
** Output parameters: none
** Returned value:          none
**********************************************************************************************************/
PUBLIC void Device_vInit(bool_t bWarmStart)
{
	v6LP_InitHardware();												/* 初始化硬件控制器		 		*/
	while(bAHI_Clock32MHzStable() == FALSE);							/* 等待时钟稳定 					*/
	if (FALSE == bWarmStart)											/* 判断是否冷启动 				*/
	{
		Exception_vInit();												/* Initialise exception handler */
		/*
		 * Initialise all DIO as outputs and drive low
		 */
		vAHI_DioSetDirection(0, 0xFFFFFFFE);
		vAHI_DioSetOutput(0xFFFFFFFE, 0);
	}

	#ifdef DBG_ENABLE													/* Debug ? 						*/
	{
		DBG_vUartInit(DEBUG_UART, DEBUG_BAUD_RATE);						/* Initialise debugging 		*/
		/*
		 * Disable the debug port flow control lines to turn off LED2
		 */
		vAHI_UartSetRTSCTS(DEBUG_UART, FALSE);
	}
	#endif

	DBG_vPrintf(TRUE, "       ");
	DBG_vPrintf(TRUE, "\n\nTest Pro");
	if (bWarmStart) {
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\nbWarmStart");
	} else {
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\r\nbColdStart");
	}

	if (FALSE == bWarmStart)											/* Cold start ? 				*/
	{
		u8TickQueue = 0;												/* Reset the tick queue 		*/
		/*
		 * Initialise PDM
		 */
		PDM_vInit(0, 63, 64, (OS_thMutex)1, /* Mutex */
					NULL, NULL, NULL);
		/*
		 * 初始化MIB
		 */
		MibDioControl_vInit(hMibDioControl, &sMibDioControl);
		/*
		 * 注册MIB
		 */
		MibDioControl_vRegister();
		Node_eJipInit();												/* 初始化协议栈 					*/

		/****************dong***********************/
		vAHI_DioSetOutput(0 , light_bit);
		u32AHI_Init();
		vAHI_SiMasterConfigure(TRUE,FALSE,15);

		vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
		    					    E_AHI_AP_INT_DISABLE,
		    					    E_AHI_AP_SAMPLE_2,
		    					    E_AHI_AP_CLOCKDIV_500KHZ,
		    					    E_AHI_AP_INTREF);
	    while (!bAHI_APRegulatorEnabled());

		//vSpiInit();

		//DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nHMC5883_Init()");
		//HMC5883_Init();
		bmp085Calibration();

	  /****************dong***********************/

	}
	else                                                              /* WarmStart */
	{
		if(i6LP_ResumeStack()!=0)										/* Resume 6LoWPAN 				*/
		{
			DBG_vPrintf(DEBUG_DEVICE_FUNC, "\ni6LP_ResumeStack() fail");
			while(1);
		}
		else
		{
			DBG_vPrintf(DEBUG_DEVICE_FUNC, "\ni6LP_ResumeStack() success");
		}
	}



		bInitialised = TRUE;											/* Now initialised 				*/

		bSleep = FALSE;
		while(FALSE == bSleep)											/* Main loop 					*/
		{
			vAHI_WatchdogRestart();										/* Restart watchdog 			*/
			Device_vTick();												/* Deal with device tick 		*/
																		/* timer events ? 				*/
			vAHI_CpuDoze();												/* Doze 	暂停CPU时钟， 节省电能， 有中断CPU 时钟继续					*/
		}
		Device_vSleep();												/* Go to sleep if we exit main 	*/

}

/**********************************************************************************************************
** Function name:     v6LP_ConfigureNetwork
** Descriptions:      设置其他网络参数
** Input parameters:  none
** Output parameters: *psNetworkConfigData：Pointer to structure containing values to be set
** Returned value:    none
**********************************************************************************************************/
PUBLIC void v6LP_ConfigureNetwork(tsNetworkConfigData *psNetworkConfigData)
{
	tsSecurityKey netSecurityKey;
	uint16 mode;
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nv6LP_ConfigureNetwork()");

	v6LP_EnableSecurity();												/* 使能安全密码 					*/

#define STACK_MODE_STANDALONE  0x0001
#define STACK_MODE_COMMISSION  0x0002

	vApi_SetStackMode(STACK_MODE_COMMISSION);							/* 设置为COMMISSION模式			*/
	mode = u16Api_GetStackMode();										/* 获取协议栈模式 				*/
	DBG_vPrintf(DEBUG_NODE_VARS, "\nStackMode is 0x%04x", mode);




	netSecurityKey.u32KeyVal_1 = 0x10101010;
	netSecurityKey.u32KeyVal_2 = 0x10101010;
	netSecurityKey.u32KeyVal_3 = 0x10101010;
	netSecurityKey.u32KeyVal_4 = 0x10101010;
	vApi_SetNwkKey(0, &netSecurityKey);									/* 设置为网络密码 				*/
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nEndDevice NetWork key Load");

}

/**********************************************************************************************************
** Function name:     v6LP_DataEvent
** Descriptions:      数据事件
** Input parameters:  iSocket：   Socket on which packet received
**             		  eEvent：   Data event
**             		  *psAddr：   Source address (for RX) or destination (for TX)
**             		  u8AddrLen：   Length of address
** Output parameters: none
** Returned value:          none
**********************************************************************************************************/
PUBLIC void v6LP_DataEvent(int iSocket, te6LP_DataEvent eEvent,
                           ts6LP_SockAddr *psAddr, uint8 u8AddrLen)
{
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nv6LP_DataEvent(%d)", eEvent);		/* Debug 						*/
	switch (eEvent)														/* Which event ? 				*/
	{
	case E_DATA_RECEIVED:											/* Data received ? 				*/
	{
		int					iRecvFromResult;
		ts6LP_SockAddr 	    s6lpSockAddr;
		uint8		       u8SockAddrLen;
		uint8    		  au8Data[256];
		void              *pvData = (void *) au8Data;



		/* Read 6LP packet */
		iRecvFromResult = i6LP_RecvFrom(iSocket, au8Data, 256, 0, &s6lpSockAddr, &u8SockAddrLen);
		/* Debug */
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\ni6LP_RecvFrom(0x%x, %c) = %d", s6lpSockAddr.sin6_addr.s6_addr32[3], au8Data[0], iRecvFromResult);

		/* Error ? */
		if (iRecvFromResult == -1)
		{

		}
		/* Success ? */
		else
		{

		}
	}
		case E_IP_DATA_RECEIVED:										/* IP data received ? 			*/
		case E_6LP_ICMP_MESSAGE:										/* 6LP ICMP message ? 			*/
		{
			/*
			 * Discard 6LP packets as only interested in JIP communication
			 */
			i6LP_RecvFrom(iSocket, NULL, 0, 0, NULL, NULL);
		}
		break;
		default:
		{
			;															/* Do nothing 					*/
		}
		break;
	}
}

/**********************************************************************************************************
** Function name:     vJIP_StackEvent
** Descriptions:      协议栈事件
** Input parameters:  eEvent：   Stack event
**             		  *pu8Data：   Additional information associated with event
**             		  u8AddrLen：   Length of additional information
** Output parameters: none
** Returned value:    none
**********************************************************************************************************/
PUBLIC void vJIP_StackEvent(te6LP_StackEvent eEvent, uint8 *pu8Data, uint8 u8DataLen)
{
	bool_t bPollNoData;
	tsSecurityKey netSecurityKey;

	switch (eEvent) {
	case E_STACK_STARTED:												/* 协议栈启动，协调器建立网络 		*/
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_STARTED", eEvent);
		break;
	case E_STACK_JOINED:												/* Network is now up 			*/
		/*
		 * 将父节点的mac地址保存到sCommNodeAddr，并打印。
		 */
		memcpy((uint8 *)&sCommNodeAddr, (uint8 *)&(((tsNwkInfo *)pu8Data)->sParentAddr), 8);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_JOINED", eEvent);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nmy parent's mac:%08x", ((tsNwkInfo *)pu8Data)->sParentAddr.u32H);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "%08x", ((tsNwkInfo *)pu8Data)->sParentAddr.u32L);
		break;
	case E_STACK_NODE_JOINED:
		/*
		 * 将子节点的mac地址保存到sCommNodeAddr，并打印。
		 */
		memcpy((uint8 *)&sCommNodeAddr, (uint8 *)&(((tsAssocNodeInfo *)pu8Data)->sMacAddr), 8);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_NODE_JOINED", eEvent);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\njoin node mac:%08x", ((tsAssocNodeInfo *)pu8Data)->sMacAddr.u32H);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "%08x", ((tsAssocNodeInfo *)pu8Data)->sMacAddr.u32L);
		break;
	case E_STACK_NODE_LEFT:
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_NODE_LEFT", eEvent);
		break;
	case E_STACK_TABLES_RESET:
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_TABLES_RESET", eEvent);
		break;
	case E_STACK_RESET:
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_RESET", eEvent);
		break;
	case E_STACK_POLL:
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_POLL", eEvent);
		te6LP_PollResponse ePollResponse;
		ePollResponse = *((te6LP_PollResponse *) pu8Data);				/* Cast response			 */
	//	DBG_vPrintf(DEBUG_DEVICE_FUNC, " POLL %d", ePollResponse);
		switch (ePollResponse)
		{
			case E_6LP_POLL_DATA_READY:									/* Got some data ? 			*/
			{
				bSleep = FALSE;
			//	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\n bSleep = FALSE");
			//	DBG_vPrintf(DEBUG_DEVICE_FUNC, " DATA_READY");
				/*
				 * Poll again in case there is more
				 */
				ePollResponse = e6LP_Poll();
			//	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\ne6LP_Poll() = %d", ePollResponse);
			}
			break;
			default:													/* Others ? 				*/
			{
				/*
				 * Network up - set flag to indicate polled but no data
				 */
				bSleep = TRUE;
				DBG_vPrintf(DEBUG_DEVICE_FUNC, "\n bSleep = TRUE");
			}
			break;
		}
		break;
	case E_STACK_NODE_JOINED_NWK:
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_NODE_JOINED_NWK", eEvent);
		break;
	case E_STACK_NODE_LEFT_NWK:
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_NODE_LEFT_NWK", eEvent);
		break;
	case E_STACK_NODE_AUTHORISE:
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_NODE_AUTHORISE", eEvent);
		break;
	case E_STACK_ROUTE_CHANGE:
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_ROUTE_CHANGE", eEvent);
		break;
	case E_STACK_GROUP_CHANGE:
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_GROUP_CHANGE", eEvent);
		break;
//	case E_STACK_GATEWAY_STARTED:
//		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_GATEWAY_STARTED", eEvent);
//		break;
	default:
		break;
	}
}

/**********************************************************************************************************
** Function name:     v6LP_PeripheralEvent
** Descriptions:     外设事件
** Input parameters:  u32Device：   Device that caused peripheral event
**             		  u32ItemBitmap：   Events within that peripheral
** Output parameters: none
** Returned value:    none
**********************************************************************************************************/
PUBLIC void v6LP_PeripheralEvent(uint32 u32Device, uint32 u32ItemBitmap)
{
	/*
	 * Tick Timer is run by stack at 10ms intervals
	 */
	if (u32Device == E_AHI_DEVICE_TICK_TIMER)
	{
		if (bInitialised == TRUE)										/* Initialised ? 				*/
		{
			if (u8TickQueue < 100) u8TickQueue++;						/* Increment pending ticks 		*/
		}
	}
}

/**********************************************************************************************************
** Function name:     Device_vTick
** Descriptions:
** Input parameters:  none
** Output parameters: none
** Returned value:    none
**********************************************************************************************************/
PUBLIC void Device_vTick(void)
{
	while (u8TickQueue > 0)												/* Tick timer fired ? 			*/
	{
		u8Tick++;														/* Increment tick counter 		*/

		if (u8Tick >= 100)												/* A second has passed ? 		*/
		{
			u8Tick = 0;													/* Zero tick counter 			*/

			if (u32Second < 0xFFFFFFFF) u32Second++;					/* Increment second counter 	*/

			#ifdef DBG_ENABLE
			{
				if (u32Second % 60 == 0)								/* A minute has passed ? 		*/
				{
					DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nDevice_vTick() u32Second = %d", u32Second);
				}
			}
			#endif
		}

		/*
		 * Pass seconds onto DIO mibs
		 */
		#if MK_BLD_MIB_DIO_CONTROL
//			if (u8Tick == 88) MibDioControl_vSecond();
		#endif

		/*
		 * Call 6LP tick for last tick in queue
		 */
		if (u8TickQueue == 1)
		{
			v6LP_Tick();												/* Call 6LP tick for last entry */
																	    /* in queue 					*/
		}
		if (u8TickQueue > 0) u8TickQueue--;								/* Decrement pending ticks 		*/
	}
}


/**********************************************************************************************************
** Function name:     Device_vSleep
** Descriptions:	  go to sleep
** Input parameters:  none
** Output parameters: none
** Returned value:    none
**********************************************************************************************************/
PUBLIC void Device_vSleep(void)
{
	//SleepNum++;
	//if(SleepNum == 120) ED_SLEEP = 5000*120 ;
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nDevice_vSleep()");
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nPDM_vSave()");

	/*
	 * Pass fake second calls into mibs (these save PDM data)
	 */
//	MibDioControl_vSecond();
	/****************dong*************************/
	u32AHI_Init();
	vAHI_SiMasterConfigure(TRUE,FALSE,15);

	vAHI_ApConfigure(E_AHI_AP_REGULATOR_ENABLE,
	    					    E_AHI_AP_INT_DISABLE,
	    					    E_AHI_AP_SAMPLE_2,
	    					    E_AHI_AP_CLOCKDIV_500KHZ,
	    					    E_AHI_AP_INTREF);
    while (!bAHI_APRegulatorEnabled());

    //SendDataToRemote(0x00033131);
    //I2C_WriteByte(0x1E,0X02,0x00);
	//vSpiInit();
	//vUartInit();


    	HMC5883_Init();
    	delay_nms(5);
    	buf[0] = (uint16)I2C_ReadByte(0x1E,0X03);
    	buf[1] = (uint16)I2C_ReadByte(0x1E,0X04);
    	x=(buf[0] << 8 | buf[1]); //Combine MSB and LSB of X Data output register
    	UserData[1] = x;
    	DBG_vPrintf(DEBUG_DEVICE_FUNC, "HMC5883_x: %d \n", x);

    	buf[4] = (uint16)I2C_ReadByte(0x1E,0X05);
    	buf[5] = (uint16)I2C_ReadByte(0x1E,0X06);
        buf[2] = (uint16)I2C_ReadByte(0x1E,0X07);
        //vPrintf("HMC5883 buf2:%x \n ",buf[2]);
    	buf[3] = (uint16)I2C_ReadByte(0x1E,0X08);
    	//vPrintf("HMC5883 buf3:%x \n ",buf[3]);
        y=(buf[2] << 8 | buf[3]); //Combine MSB and LSB of Y Data output register
        z=(buf[4] << 8 | buf[5]); //Combine MSB and LSB of Z Data output register
        UserData[2] = y;
        UserData[8] = z;
       // UserData[8] = z;
        DBG_vPrintf(DEBUG_DEVICE_FUNC, "HMC5883_y: %d \n", y);
        DBG_vPrintf(DEBUG_DEVICE_FUNC, "HMC5883_z: %d \n", z);

        DBG_vPrintf(DEBUG_DEVICE_FUNC, "HMC5883_x: %x \n", buf[0]);
        DBG_vPrintf(DEBUG_DEVICE_FUNC, "HMC5883_x: %x \n", buf[1]);
        DBG_vPrintf(DEBUG_DEVICE_FUNC, "HMC5883_x: %x \n", buf[2]);
        DBG_vPrintf(DEBUG_DEVICE_FUNC, "HMC5883_x: %x \n", buf[3]);
        DBG_vPrintf(DEBUG_DEVICE_FUNC, "HMC5883_x: %x \n", buf[4]);
        DBG_vPrintf(DEBUG_DEVICE_FUNC, "HMC5883_x: %x \n", buf[5]);

        //SendDataToRemote(0x00033131);
        I2C_WriteByte(0x1E,0X02,0x00);

    	vGetADXL335value1();

    	vGetADXL335value2();

    	vGetADXL335value3();

		I2C_WriteByte(adress,0xF4,0x2E);
		delay_nms(5);
		temperature = bmp085GetTemperature(bmp085ReadUT()); //MUST be called first bmp085ReadUT()
		T_trans = (uint16)temperature;
		UserData[6] = T_trans;

		I2C_WriteByte(adress,0xF4,0x34 + (OSS<<6));
		delay_nms(7);
		pressure = bmp085GetPressure(bmp085ReadUP());//0.59312/1.00804;bmp085ReadUP()
		P_trans = (uint16)(pressure>>1);
		UserData[7] = P_trans;


    UserData[0] = EndNum;            //第一位为起始位“S”，第二位为节点号
    UserData[9] = 0x4500;                              //最后为截止位“E”

    SendDataToRemote(&UserData[0]);     //发送数据

	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nv6LP_Sleep(TRUE, %d)        ", ED_SLEEP);
	/*
	 * Request stack to put us to sleep,  memory hold  sleep 1000ms
	 */
	v6LP_Sleep(TRUE, ED_SLEEP);

	while(1)															/* Sleep loop 					*/
	{
	   	/*
	   	 * Deal with device tick timer events ?
	   	 */
	   	Device_vTick();
		vAHI_CpuDoze();													/* Doze 						*/
	}
}

/**********************************************************************************************************
** Function name:     Node_eJipInit
** Descriptions:      初始化协议栈
** Input parameters:  none
** Output parameters: none
** Returned value:    none
**********************************************************************************************************/
PUBLIC void Node_eJipInit(void)
{
	teJIP_Status     eStatus;
	DBG_vPrintf(DEBUG_NODE_FUNC, "\nNode_eJipInit()");

	PUBLIC	tsJIP_InitData sJipInitData;								/* Jip initialisation structure */

	/*
	 *  配置JIP
	 */
	sJipInitData.u64AddressPrefix       = CONFIG_ADDRESS_PREFIX; 		/* 配置WPAN内IPv6地址的前缀，		*/
																		/* 只有协调器才需要配置			*/
	sJipInitData.u32Channel				= CONFIG_SCAN_CHANNELS;     	/* 指定所用的信道号或配置所 		*/
																		/* 要扫描的信道号		  		*/
	sJipInitData.u16PanId				= CONFIG_PAN_ID;
	/*
	 *  配置最大的IP包大小：最大数据量+40，大小在0或256~1280，若为0则是设置为1280
	 */
	sJipInitData.u16MaxIpPacketSize		= 0;
	sJipInitData.u16NumPacketBuffers	= 2;    						/* Number of IP packet buffers 	*/
	sJipInitData.u8UdpSockets			= 2;           					/* Number of UDP sockets 		*/
																		/* supported 					*/
	sJipInitData.eDeviceType			= E_JIP_DEVICE_END_DEVICE;             	/* Device type (C, R, or ED) E_JIP_DEVICE_ROUTER  ,	E_JIP_DEVICE_END_DEVICE*/

	sJipInitData.u32RoutingTableEntries = 0;


	/*
	 *  每个节点都有一个Device ID用于识别设备的类型，属于同一设备类型的设备具有相同的MIBs和相应的MIB变量
	 *  Device ID由两部分构成：31~16为厂商ID，15~0为产品ID
	 *  厂商ID的最高位用于识别剩余的15位是否是有效ID，若最高位为0则为无效ID，在开发阶段建议最高位设置为0
	 */
	sJipInitData.u32DeviceId			= MK_JIP_DEVICE_ID;
	sJipInitData.u8UniqueWatchers		= CONFIG_UNIQUE_WATCHERS;
	sJipInitData.u8MaxTraps				= CONFIG_MAX_TRAPS;
	sJipInitData.u8QueueLength 			= CONFIG_QUEUE_LENGTH;
	sJipInitData.u8MaxNameLength		= CONFIG_MAX_NAME_LEN;			/* 配置节点MIB的描述名变量的大小	*/
	sJipInitData.u16Port				= JIP_DEFAULT_PORT;				/* 配置接收JIP数据包的UDP端口号	*/
	sJipInitData.pcVersion 				= MK_VERSION;

	DBG_vPrintf(DEBUG_NODE_VARS, "\n\tsJipInitData.u32Channel             = 0x%08x", sJipInitData.u32Channel);
	DBG_vPrintf(DEBUG_NODE_VARS, "\n\tsJipInitData.u16PanId               = 0x%04x", sJipInitData.u16PanId);
	DBG_vPrintf(DEBUG_NODE_VARS, "\n\tsJipInitData.eDeviceType            = %d",     sJipInitData.eDeviceType);
	DBG_vPrintf(DEBUG_NODE_VARS, "\n\tsJipInitData.u32RoutingTableEntries = %d",     sJipInitData.u32RoutingTableEntries);
	DBG_vPrintf(DEBUG_NODE_VARS, "\n\tsJipInitData.u32DeviceId            = 0x%08x", sJipInitData.u32DeviceId);
	DBG_vPrintf(DEBUG_NODE_VARS, "\n\tsJipInitData.pcVersion              = '%s'",   sJipInitData.pcVersion);

	DBG_vPrintf(DEBUG_NODE_FUNC, "\neJIP_Init()");

	eStatus = eJIP_Init(&sJipInitData);									/* Initialise JIP 				*/

	if (eStatus != E_JIP_OK) {
		DBG_vPrintf(DEBUG_NODE_FUNC, "\nFailed to initialise JenNet-IP stack");
		while(1);
	}
	v6LP_SetPacketDefragTimeout(1);										/* Set 1 second defrag timeout 	*/
}

/**********************************************dong code************************************/
PUBLIC uint8 I2C_ReadByte(uint8 addr,uint8 reg_addr)
{
	uint8 temp = 0 ;
	if(FALSE == bAHI_SiMasterPollBusy())
	{
	    vAHI_SiMasterWriteSlaveAddr(addr,FALSE);                             /*write a 7-bit slave address      */
	    bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,E_AHI_SI_NO_STOP_BIT,E_AHI_SI_NO_SLAVE_READ,E_AHI_SI_SLAVE_WRITE,E_AHI_SI_SEND_ACK,E_AHI_SI_NO_IRQ_ACK);
	  																					/*issue start and wrote commands   */
		while(TRUE == bAHI_SiMasterPollTransferInProgress()) ;            /*wait for an indication of success*/
        bAHI_SiMasterCheckRxNack();

		//if(TRUE == bAHI_SiMasterPollArbitrationLost())   DBG_vPrintf(DEBUG_DEVICE_FUNC, "\n TRUE");
		//else DBG_vPrintf(DEBUG_DEVICE_FUNC, "\n FALSE");

		vAHI_SiMasterWriteData8(reg_addr); 	                                /*write a 8-bit data               */

		/*issue a wrote command            */
		bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,E_AHI_SI_NO_STOP_BIT,E_AHI_SI_NO_SLAVE_READ,E_AHI_SI_SLAVE_WRITE,E_AHI_SI_SEND_ACK,E_AHI_SI_NO_IRQ_ACK);

		while(TRUE == bAHI_SiMasterPollTransferInProgress()) ;            /*wait for an indication of success*/
		bAHI_SiMasterCheckRxNack();

		vAHI_SiMasterWriteSlaveAddr(addr,TRUE);                             /*write a 7-bit slave address      */
	    bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,E_AHI_SI_NO_STOP_BIT,E_AHI_SI_NO_SLAVE_READ,E_AHI_SI_SLAVE_WRITE,E_AHI_SI_SEND_ACK,E_AHI_SI_NO_IRQ_ACK);	  																					/*issue start and wrote commands   */
		while(TRUE == bAHI_SiMasterPollTransferInProgress()) ;            /*wait for an indication of success*/
		bAHI_SiMasterCheckRxNack();

	    bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,E_AHI_SI_STOP_BIT,E_AHI_SI_SLAVE_READ,E_AHI_SI_NO_SLAVE_WRITE,E_AHI_SI_SEND_NACK,E_AHI_SI_NO_IRQ_ACK);
	    while(TRUE == bAHI_SiMasterPollTransferInProgress()) ;
	    temp = u8AHI_SiMasterReadData8();

		//DBG_vPrintf(DEBUG_DEVICE_FUNC, "\n End of Read \n");
	}
	return temp ;
}

/**********************************************************************************************************
** Function name:     I2C_WriteByte
** Descriptions:      向I2C slave 写入一个8位的数据
** Input parameters:  slave地址       addr ,寄存器地址         reg_addr ,  数据  data
** Output parameters: none
** Returned value:    none
**********************************************************************************************************/

PUBLIC void I2C_WriteByte(uint8 addr,uint8 reg_addr,uint8 data)
{
	if(FALSE == bAHI_SiMasterPollBusy())
	{
		vAHI_SiMasterWriteSlaveAddr(addr,FALSE);                             /*write a 7-bit slave address      */
		bAHI_SiMasterSetCmdReg(E_AHI_SI_START_BIT,E_AHI_SI_NO_STOP_BIT,E_AHI_SI_NO_SLAVE_READ,E_AHI_SI_SLAVE_WRITE,E_AHI_SI_SEND_ACK,E_AHI_SI_NO_IRQ_ACK);
		while(TRUE == bAHI_SiMasterPollTransferInProgress()) ;            /*wait for an indication of success*/
		bAHI_SiMasterCheckRxNack();

		//if(TRUE == bAHI_SiMasterPollArbitrationLost())   DBG_vPrintf(DEBUG_DEVICE_FUNC, "\n TRUE");
		//else DBG_vPrintf(DEBUG_DEVICE_FUNC, "\n FALSE");

		vAHI_SiMasterWriteData8(reg_addr);                                 /*write a 8-bit data               */
		bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,E_AHI_SI_NO_STOP_BIT,E_AHI_SI_NO_SLAVE_READ,E_AHI_SI_SLAVE_WRITE,E_AHI_SI_SEND_ACK,E_AHI_SI_NO_IRQ_ACK);
		while(TRUE == bAHI_SiMasterPollTransferInProgress()) ;            /*wait for an indication of success*/
		bAHI_SiMasterCheckRxNack();

		vAHI_SiMasterWriteData8(data);                                 /*write a 8-bit data               */
		bAHI_SiMasterSetCmdReg(E_AHI_SI_NO_START_BIT,E_AHI_SI_STOP_BIT,E_AHI_SI_NO_SLAVE_READ,E_AHI_SI_SLAVE_WRITE,E_AHI_SI_SEND_ACK,E_AHI_SI_NO_IRQ_ACK);
						                                                                /*issue a wrote command            */

		//DBG_vPrintf(DEBUG_DEVICE_FUNC, "\n End of Write");
	}
}

PRIVATE void vGetADXL335value1(void)
{
    vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
    			    E_AHI_AP_INPUT_RANGE_2,
    			    E_AHI_ADC_SRC_ADC_1);
    vAHI_AdcStartSample();                                       /*初始化 ADC  */

    while (bAHI_AdcPoll());
	u16AdcReading = u16AHI_AdcRead();
	/* Input range is 0 to 2.4V. ADC has full scale range of 10 bits.
	Therefore a 1 bit change represents a voltage of approx 2344uV */
	u16BattLevelmV = ((uint32)(u16AdcReading * 2344)) / 1000;

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "u16AdcReading: %d \n", u16AdcReading);
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "X Voltage (mV): %d \n", u16BattLevelmV);
	//vPrintf("X Voltage (mV): %d \n",u16BattLevelmV);

	UserData[3] = u16BattLevelmV;
	//SendDataToRemote(0x00053131);

	vAHI_AdcStartSample();
}
PRIVATE void vGetADXL335value2(void)
{

    vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
    			    E_AHI_AP_INPUT_RANGE_2,
    			    E_AHI_ADC_SRC_ADC_2);
    vAHI_AdcStartSample();

    while (bAHI_AdcPoll());
	u16AdcReading = u16AHI_AdcRead();
	/* Input range is 0 to 2.4V. ADC has full scale range of 10 bits.
	Therefore a 1 bit change represents a voltage of approx 2344uV */
	u16BattLevelmV = ((uint32)(u16AdcReading * 2344)) / 1000;

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "u16AdcReading: %d \n", u16AdcReading);
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "Y Voltage (mV): %d \n", u16BattLevelmV);
	//vPrintf("Y Voltage (mV): %d \n",u16BattLevelmV);
	UserData[4] = u16BattLevelmV;
	//SendDataToRemote(0x00073131);
	vAHI_AdcStartSample();
}
PRIVATE void vGetADXL335value3(void)
{
    vAHI_AdcEnable(E_AHI_ADC_SINGLE_SHOT,
    			    E_AHI_AP_INPUT_RANGE_2,
    			    E_AHI_ADC_SRC_ADC_3);
    vAHI_AdcStartSample();

    while (bAHI_AdcPoll());
	u16AdcReading = u16AHI_AdcRead();
	/* Input range is 0 to 2.4V. ADC has full scale range of 10 bits.
	Therefore a 1 bit change represents a voltage of approx 2344uV */
	u16BattLevelmV = ((uint32)(u16AdcReading * 2344)) / 1000;

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "u16AdcReading: %d \n", u16AdcReading);
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "Z Voltage (mV): %d \n", u16BattLevelmV);
	//vPrintf("Z Voltage (mV): %d \n",u16BattLevelmV);
	UserData[5] = u16BattLevelmV;
	//SendDataToRemote(0x00093131);
	vAHI_AdcStartSample();
}

PRIVATE void bmp085Calibration(void)
{
	delay_nms(5);

	ac1 = (uint16)I2C_ReadByte(adress,0xAA);
	ac1 <<= 8;
	ac1 += (uint16)I2C_ReadByte(adress,0xAB);
	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "ac1: %d \n", ac1);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "ac1: %04x \n", ac1);

	ac2 = (uint16)I2C_ReadByte(adress,0xAC);
	ac2 <<= 8;
	ac2 += (uint16)I2C_ReadByte(adress,0xAD);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "ac2: %d \n", ac2);

	ac3 = (uint16)I2C_ReadByte(adress,0xAE);
	ac3 <<= 8;
	ac3 += (uint16)I2C_ReadByte(adress,0xAF);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "ac3: %d \n", ac3);

	ac4 = (uint16)I2C_ReadByte(adress,0xB0);
	ac4 <<= 8;
	ac4 += (uint16)I2C_ReadByte(adress,0xB1);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "ac4: %d \n", ac4);

	ac5 = (uint16)I2C_ReadByte(adress,0xB2);
	ac5 <<= 8;
	ac5 += (uint16)I2C_ReadByte(adress,0xB3);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "ac5: %d \n", ac5);

	ac6 = (uint16)I2C_ReadByte(adress,0xB4);
	ac6 <<= 8;
	ac6 += (uint16)I2C_ReadByte(adress,0xB5);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "ac6: %d \n", ac6);

	b1 = (uint16)I2C_ReadByte(adress,0xB6);
	b1 <<= 8;
	b1 += (uint16)I2C_ReadByte(adress,0xB7);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "b1: %d \n", b1);

	b2 = (uint16)I2C_ReadByte(adress,0xB8);
	b2 <<= 8;
	b2 += (uint16)I2C_ReadByte(adress,0xB9);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "b2: %d \n", b2);

	mb = (uint16)I2C_ReadByte(adress,0xBA);
	mb <<= 8;
	mb += (uint16)I2C_ReadByte(adress,0xBB);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "mb: %d \n", mb);

	mc = (uint16)I2C_ReadByte(adress,0xBC);
	mc <<= 8;
	mc += (uint16)I2C_ReadByte(adress,0xBD);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "mc: %d \n", mc);

	md = (uint16)I2C_ReadByte(adress,0xBE);
	md <<= 8;
	md += (uint16)I2C_ReadByte(adress,0xBF);

	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "md: %d \n", md);



	chip_id = (uint16)I2C_ReadByte(adress,0xD0);

}

PRIVATE float bmp085GetTemperature(uint16 ut)
{
	uint16 temper = 0;
	int32 x1, x2;
	float temp;

  x1 = (((int32)ut - (int32)ac6)*(int32)ac5) >> 15;
  x2 = ((int32)mc << 11)/(x1 + md);
  b5 = x1 + x2;
  //DBG_vPrintf(DEBUG_DEVICE_FUNC, "b5: %d \n", b5);

  temp = ((b5 + 8)>>4);
  //temp = temp /10;
  temper = (uint16)temp ;

  //DBG_vPrintf(DEBUG_DEVICE_FUNC, "T_temper: %d \n", temper);
  return temp;
}

PRIVATE int32 bmp085GetPressure(uint32 up){
  int32 x1, x2, x3, b3, b6, p;
  uint32 b4, b7;
  int32 temp;

  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((int32)ac1)*4 + x3)<<OSS) + 2)>>2;
  //DBG_vPrintf(DEBUG_DEVICE_FUNC, "b3: %d \n", b3);

  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (uint32)(x3 + 32768))>>15;
  //DBG_vPrintf(DEBUG_DEVICE_FUNC, "b4: %d \n", b4);

  b7 = ((uint32)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;
  //DBG_vPrintf(DEBUG_DEVICE_FUNC, "P: %d \n", p);


  x1 = (p>>8) * (p>>8);
  //DBG_vPrintf(DEBUG_DEVICE_FUNC, "x1: %d \n", x1);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  //DBG_vPrintf(DEBUG_DEVICE_FUNC, "x2: %d \n", x2);
  p += (x1 + x2 + 3791)>>4;


  temp = p;
  return temp;
}

PRIVATE uint16 bmp085ReadUT()
{

  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
	//I2C_WriteByte(adress,0xF4,0x2E);

   // delay(); //延时5 ms
	//delay_nms(5);
  // Read two bytes from registers 0xF6 and 0xF7
  ut = (uint16)I2C_ReadByte(adress,0xF6);
	ut <<= 8;
	ut += (uint16)I2C_ReadByte(adress,0xF7);
	//delay();
	//delay_nms(5);
	//DBG_vPrintf(DEBUG_DEVICE_FUNC, "temperature: %d \n", ut);
  return ut;
}

// Read the uncompensated pressure value
PRIVATE uint32 bmp085ReadUP(){

  uint16 msb, lsb, xlsb;

  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
//	I2C_WriteByte(adress,0xF4,0x34 + (OSS<<6));

	 // Wait for conversion, delay time dependent on OSS
	//delay(); //延时5 ms
	//delay_nms(5);
  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  msb = (uint16)I2C_ReadByte(adress,0xF6);
  lsb = (uint16)I2C_ReadByte(adress,0xF7);
  xlsb = (uint16)I2C_ReadByte(adress,0xF8);

  up = (((uint32) msb << 16) | ((uint32) lsb << 8) | (uint32) xlsb) >> (8-OSS);
  //DBG_vPrintf(DEBUG_DEVICE_FUNC, "pressure: %d \n", up);

  return up;
}

PRIVATE void delay_nms(uint16 t)
{
    for(i=0;i<100;i++)
    {
    	for(j=0;j<(10*t);j++)
    	{
    		//_nop_();
    		if(delay_num == 1) delay_num = 0;
    		else delay_num = 1 ;
    	}
    }
}



PRIVATE void HMC5883_Init(void)
{
	I2C_WriteByte(0x1E,0X00,0x70);
	I2C_WriteByte(0x1E,0X02,0x00);

}

/***********************************************************dong code************************************************/

/********************************************************************************************************
	END OF FILE
********************************************************************************************************/
