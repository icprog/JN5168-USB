

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
PRIVATE  uint8 board_num[2];

PRIVATE  uint32 start_counter=0;
PRIVATE	 uint32 tx_error_num1=0;
PRIVATE	 uint32 tx_num1=0;
PRIVATE  uint8 received_data_uart1[40];
PRIVATE  uint8 received_num_uart1 =1;
PRIVATE  uint8 data_to_send[1000]={0};
PRIVATE  uint8 start_juge1 =0;
/**********************************************************************************************************
    Other data
**********************************************************************************************************/
PRIVATE uint8   u8TickQueue;											/* Tick timer queue 			*/
PRIVATE uint8   u8Tick;	   												/* Tick counter 				*/
PUBLIC  uint32  u32Second;  												/* Second counter*/
PRIVATE bool_t   bInitialised;
PRIVATE bool_t   bSleep;
PRIVATE uint8   *pTxBuf ;
PRIVATE uint8   *pRxBuf ;
PRIVATE bool_t   bLight=TRUE;
PRIVATE bool_t   bConnected=FALSE;
/**********************************************************************************************************
    Exported Functions
**********************************************************************************************************/
PUBLIC void Device_vInit(bool_t bWarmStart);
PUBLIC void Node_eJipInit(void);
PRIVATE void delay_nms(uint16 time);


#define UART E_AHI_UART_1
PRIVATE void vPutChar(uint8 c) ;
PRIVATE void vUartInit(void);

PRIVATE uint8   bTog=0;
PRIVATE uint8  turn_flag = 0;

#define LED_TOP (1 << 9)
#define LED_BOTTOM (1 << 10)

/**********************************************************************************************************
	Զ���豸��ַ
**********************************************************************************************************/

#define PROTOCOL_PORT				 	 (1190)
PRIVATE ts6LP_SockAddr 		  s6lpSockAddrLocal;
PRIVATE bool_t          	  bProtocolSocket;
PRIVATE int					  iProtocolSocket;

MAC_ExtAddr_s sCommNodeAddr;
MAC_ExtAddr_s sChildNodeAddr;
MAC_ExtAddr_s sParentNodeAddr;
MAC_ExtAddr_s sLocalNodeAddr;

PRIVATE uint64 u64LocalAddr;

//PRIVATE void Protocol_vSendTo(ts6LP_SockAddr *ps6lpSockAddr, uint8 *pu8Data, uint16 u16DataLen);
/**********************************************************************************************************
** Function name:     SendDataToRemote
** Descriptions:      �������ݷ�������
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
		}
}

PRIVATE int SendDataToRemote(MAC_ExtAddr_s *psMacAddr, uint8 *pu8Data, uint16 u16DataLen)
{
	uint8 i = 0;
	ts6LP_SockAddr s6LP_SockAddr;										/* ����ts6LP_SockAddr 		    */
	EUI64_s  sIntAddr;
    int iGetDataBufferResult;
    int iSendToResult;
    uint8 *pu8Buffer;
																		/* vJIP_StackEvent��ֵ    			*/
	memset(&s6LP_SockAddr, 0, sizeof(ts6LP_SockAddr));					/* ���s6LP_SockAddr�ṹ���ڴ�ֵ 	*/
	i6LP_CreateInterfaceIdFrom64(&sIntAddr, (EUI64_s *) psMacAddr);		/* �����ӿ�ID 					*/
	i6LP_CreateLinkLocalAddress (&s6LP_SockAddr.sin6_addr, &sIntAddr);	/* �������ӱ��ص�ַ 				*/
	/*
	 * Complete full socket address
	 */
	s6LP_SockAddr.sin6_family = E_6LP_PF_INET6;							/* ��������ΪE_6LP_PF_INET6		*/
	s6LP_SockAddr.sin6_flowinfo =0;										/* ����flow��Ϣ 					*/
	s6LP_SockAddr.sin6_port = PROTOCOL_PORT;							/* IPV6�˿ں� 					*/
	s6LP_SockAddr.sin6_scope_id =0;										/* ����scope��Ϣ*/

	/* Try to get a data buffer */
	iGetDataBufferResult = i6LP_GetDataBuffer(&pu8Buffer);

	if (iGetDataBufferResult == -1)
	{
	    DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nFAIL1", 0);
	    return -2;
	}
	else
	{
	    memcpy(pu8Buffer, pu8Data, u16DataLen);
	    /* Transmit packet */
	    iSendToResult = i6LP_SendTo(iProtocolSocket, pu8Buffer, u16DataLen, 0, &s6LP_SockAddr, sizeof(ts6LP_SockAddr));

	    if (iSendToResult == -1)
	    {
	        DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nFAIL2", 0);
	        return -1;
	    }
	    else
	    {
	        DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nSUCCESS", 0);
	        return 0;
	    }
	}
}

PRIVATE void Protocol_vSendTo(ts6LP_SockAddr *ps6lpSockAddr, uint8 *pu8Data, uint16 u16DataLen)
{

}
/**********************************************************************************************************
** Function name:     vJIP_Remote_DataSent
** Descriptions:      ����Զ����������
** Input parameters:  ts6LP_SockAddr�� ��ַ����
**             		  teJIP_Status:  ����״̬��E_JIP_OK����ɹ�������ʧ��
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
** Descriptions:     ����Զ��������Ӧ
** Input parameters: *psAddr: ��ַ����
**			   		 u8Handle: ���
**			   		 u8MibIndex:MIB����
**			   		 u8VarIndex:MIB��������
**             		 eStatus:  ��Ӧ״̬��E_JIP_OK����ɹ�������ʧ��
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
** Descriptions:      ������ڣ����������ϵ�����ʱ�����
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
** Descriptions:      ��������memory hold˯�ߣ��������
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
** Descriptions:      �豸��ʼ������
** Input parameters:  bWarmStart���Ƿ���������TRUE��������FALSE������
** Output parameters: none
** Returned value:          none
**********************************************************************************************************/
PUBLIC void Device_vInit(bool_t bWarmStart)
{
	sCommNodeAddr.u32H = COORDINAROR_MAC_H;  //coordinator's mac
	sCommNodeAddr.u32L = COORDINAROR_MAC_L;
	bConnected = FALSE;

	v6LP_InitHardware();
	vAHI_HighPowerModuleEnable(TRUE,TRUE);

	/* ��ʼ��Ӳ��������		 		*/
	while(bAHI_Clock32MHzStable() == FALSE);							/* �ȴ�ʱ���ȶ� 					*/
	if (FALSE == bWarmStart)											/* �ж��Ƿ������� 				*/
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
		 * ��ʼ��MIB
		 */
		MibDioControl_vInit(hMibDioControl, &sMibDioControl);
		/*
		 * ע��MIB
		 */
		MibDioControl_vRegister();
		Node_eJipInit();												/* ��ʼ��Э��ջ 					*/
		vUartInit();
		vAHI_DioSetOutput(LED_BOTTOM, 0);
	}

	while (1)
	    {
				v6LP_Tick();
				vAHI_WatchdogRestart();
		}

}

/**********************************************************************************************************
** Function name:     v6LP_ConfigureNetwork
** Descriptions:      ���������������
** Input parameters:  none
** Output parameters: *psNetworkConfigData��Pointer to structure containing values to be set
** Returned value:    none
**********************************************************************************************************/
PUBLIC void v6LP_ConfigureNetwork(tsNetworkConfigData *psNetworkConfigData)
{
	tsSecurityKey netSecurityKey;
	uint16 mode;
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nv6LP_ConfigureNetwork()");

	v6LP_EnableSecurity();												/* ʹ�ܰ�ȫ���� 					*/

#define STACK_MODE_STANDALONE  0x0001
#define STACK_MODE_COMMISSION  0x0002

	vApi_SetStackMode(STACK_MODE_COMMISSION);							/* ����ΪCOMMISSIONģʽ			*/
	mode = u16Api_GetStackMode();										/* ��ȡЭ��ջģʽ 				*/
	DBG_vPrintf(DEBUG_NODE_VARS, "\nStackMode is 0x%04x", mode);



														/* ������·��					*/
	netSecurityKey.u32KeyVal_1 = 0x10101010;
	netSecurityKey.u32KeyVal_2 = 0x10101010;
	netSecurityKey.u32KeyVal_3 = 0x10101010;
	netSecurityKey.u32KeyVal_4 = 0x10101010;
	vApi_SetNwkKey(0, &netSecurityKey);									/* ����Ϊ�������� 				*/
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nRouter NetWork key Load");



}

/**********************************************************************************************************
** Function name:     v6LP_DataEvent
** Descriptions:      �����¼�
** Input parameters:  iSocket��   Socket on which packet received
**             		  eEvent��   Data event
**             		  *psAddr��   Source address (for RX) or destination (for TX)
**             		  u8AddrLen��   Length of address
** Output parameters: none
** Returned value:          none
**********************************************************************************************************/
PUBLIC void v6LP_DataEvent(int iSocket, te6LP_DataEvent eEvent,
                           ts6LP_SockAddr *psAddr, uint8 u8AddrLen)
{
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nv6LP_DataEvent(%d)", eEvent);		/* Debug 						*/
	switch (eEvent)														/* Which event ? 				*/
	{
	    case E_DATA_SENT:
	    {
	    	tx_error_num1 = 0;
            if(bConnected == FALSE)
            {
            	bConnected = TRUE;
            	DBG_vPrintf(DEBUG_DEVICE_FUNC, "start\n", 0);
            	vAHI_DioSetOutput(0, LED_BOTTOM);
            }
	    	break;
	    }
	    case E_DATA_SEND_FAILED:
	    {
     	    tx_error_num1 = tx_error_num1 + 1;
     	    if(tx_error_num1 >= 4)
     	    {
     	       DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nRESET", 0);
     	       vAHI_DioSetOutput(LED_BOTTOM, 0);
     	       vPrintf("XTQD");
     	    }
	    	break;
	    }
		case E_DATA_RECEIVED:											/* Data received ? 				*/
		{
	          DBG_vPrintf(DEBUG_DEVICE_FUNC, "E_DATA_RECEIVED");
	            int                 iRecvFromResult;
	            ts6LP_SockAddr      s6lpSockAddr;
	            uint8              u8SockAddrLen;
	            uint8             au8Data[256];
	            uint16            au8DataLen;
	            uint16            *pau8DataLen;
	            void              *pvData = (void *) au8Data;

	            pau8DataLen = &au8DataLen;
	            i6LP_GetNextPacketSize(iSocket, pau8DataLen);
	            /* Read 6LP packet */
	            iRecvFromResult = i6LP_RecvFrom(iSocket, au8Data, au8DataLen, 0, &s6lpSockAddr, &u8SockAddrLen);
	            /* Error ? */
	            if (iRecvFromResult == -1)
	            {

	            }
	            /* Success ? */
	            else
	            {
	                int k;
	    			uint8	mcu_cmd[15];
	                if(au8DataLen == 35)
	    	        {
	    			    if(au8Data[0] == 'S')
	    			    {
	    			    	mcu_cmd[0] = 'S';    //��ʼ�����
	    			    	mcu_cmd[1] = 'R';     //ʹ��zigbeeģ��UART1����MCU��UART1������,R uart������������

	    			    	mcu_cmd[2] = au8Data[16];    //MCU���ݲɼ�ģʽ������or����
	    			    	mcu_cmd[3] = au8Data[17];    //���ݲɼ���� 2 bytes
	    			    	mcu_cmd[4] = au8Data[18];
	    			    	mcu_cmd[5] = au8Data[19];  //��Flash�������������ȫ�����͵�Coord
	    			    	mcu_cmd[6] = au8Data[20];  //ʱ����Ϣ����----�� 6 byets
	    			    	mcu_cmd[7] = au8Data[21];
							mcu_cmd[8] = au8Data[22];
							mcu_cmd[9] = au8Data[23];
							mcu_cmd[10] = au8Data[24];
							mcu_cmd[11] = au8Data[25];
							mcu_cmd[12] = au8Data[26];   //���߿���λ FF �����ߣ�00����

							mcu_cmd[13] = 0xFF;          //Ԥ��
							mcu_cmd[14] = 'E';          //������ֹ��
	    			    }

	    			    for(k = 0;k < 15;k ++)
	    				{
	    			    	vPutChar(mcu_cmd[k]);
	    				}
	    	        }

	                /*if(turn_flag == 0)
	                {
	                    vAHI_DioSetOutput(0, LED_BOTTOM);
	                    turn_flag = 1;
	                }
	                else
	                {
	                    vAHI_DioSetOutput(LED_BOTTOM, 0);
	                    turn_flag = 0;
	                }*/
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
** Descriptions:      Э��ջ�¼�
** Input parameters:  eEvent��   Stack event
**             		  *pu8Data��   Additional information associated with event
**             		  u8AddrLen��   Length of additional information
** Output parameters: none
** Returned value:    none
**********************************************************************************************************/
PUBLIC void vJIP_StackEvent(te6LP_StackEvent eEvent, uint8 *pu8Data, uint8 u8DataLen)
{
	//bool_t bPollNoData;
	//tsSecurityKey netSecurityKey;

	switch (eEvent) {
	case E_STACK_STARTED:												/* Э��ջ������Э������������ 		*/
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_STARTED", eEvent);
		break;
	case E_STACK_JOINED:												/* Network is now up 			*/
		/*
		 * �����ڵ��mac��ַ���浽sCommNodeAddr������ӡ��
		 */
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_JOINED", eEvent);

		memcpy((uint8 *)&sCommNodeAddr, (uint8 *)&(((tsNwkInfo *)pu8Data)->sParentAddr), 8);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nmy parent's mac:%08x", ((tsNwkInfo *)pu8Data)->sParentAddr.u32H);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "%08x", ((tsNwkInfo *)pu8Data)->sParentAddr.u32L);

		memcpy((uint8 *)&sLocalNodeAddr, (uint8 *)&(((tsNwkInfo *)pu8Data)->sLocalAddr), 8);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nmy mac:%08x", ((tsNwkInfo *)pu8Data)->sLocalAddr.u32H);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "%08x", ((tsNwkInfo *)pu8Data)->sLocalAddr.u32L);

		u64LocalAddr = (((uint64)(sLocalNodeAddr.u32H)) << 32) + sLocalNodeAddr.u32L;

		Protocol_vOpenSocket();    //���ͷ�����Ҫ�� Socket.  ���շ���Ҫ��socket
		vPrintf("K");
            /*data_to_send[0] = 'S';
            data_to_send[1] = 0x19;
            data_to_send[2] = 0x43;
            data_to_send[3] = (uint8)((u64LocalAddr >> 56) );
            data_to_send[4] = (uint8)((u64LocalAddr >> 48) );
            data_to_send[5] = (uint8)((u64LocalAddr >> 40) );
            data_to_send[6] = (uint8)((u64LocalAddr >> 32) );
            data_to_send[7] = (uint8)((u64LocalAddr >> 24) );
            data_to_send[8] = (uint8)((u64LocalAddr >> 16) );
            data_to_send[9] = (uint8)((u64LocalAddr >> 8)  );
            data_to_send[10] = (uint8)(u64LocalAddr);
            data_to_send[11] = 0xFF;
            data_to_send[12] = (board_num >>8)& 0x00FF;
            data_to_send[13] = board_num & 0x00FF;
            data_to_send[14] = 0xFF;
            data_to_send[15] = 0xFF;
            data_to_send[16] = 0xFF;
            data_to_send[17] = 0xFF;
            data_to_send[18] = 0xFF;
            data_to_send[19] = 0xFF;
            data_to_send[20] = 0xFF;
            data_to_send[21] = 0xFF;
            data_to_send[22] = 0xFF;
            data_to_send[23] = 0xFF;
            data_to_send[24] = 0xFF;
            data_to_send[25] = 0xFF;
            data_to_send[26] = 0xFF;
            data_to_send[27] = 0xFF;
            data_to_send[28] = 0xFF;
            data_to_send[29] = 0xFF;
            data_to_send[30] = 0xFF;
            data_to_send[31] = 0xFF;
            data_to_send[32] = 0xFF;
            data_to_send[33] = 0xFF;
            data_to_send[34] = 'D';

            SendDataToRemote(&sCommNodeAddr, data_to_send, 35);*/

		break;
	case E_STACK_NODE_JOINED:
		/*
		 * ���ӽڵ��mac��ַ���浽sCommNodeAddr������ӡ��
		 */
		memcpy((uint8 *)&sChildNodeAddr, (uint8 *)&(((tsAssocNodeInfo *)pu8Data)->sMacAddr), 8);
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
		DBG_vPrintf(DEBUG_DEVICE_FUNC, " POLL %d", ePollResponse);
		switch (ePollResponse)
		{
			case E_6LP_POLL_DATA_READY:									/* Got some data ? 			*/
			{
				bSleep = FALSE;
				DBG_vPrintf(DEBUG_DEVICE_FUNC, " DATA_READY");
				/*
				 * Poll again in case there is more
				 */
				ePollResponse = e6LP_Poll();
				DBG_vPrintf(DEBUG_DEVICE_FUNC, "\ne6LP_Poll() = %d", ePollResponse);
			}
			break;
			default:													/* Others ? 				*/
			{
				/*
				 * Network up - set flag to indicate polled but no data
				 */
				bSleep = TRUE;
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
** Descriptions:     �����¼�
** Input parameters:  u32Device��   Device that caused peripheral event
**             		  u32ItemBitmap��   Events within that peripheral
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
			if (u8TickQueue < 100) u8TickQueue++ ;						/* Increment pending ticks 		*/
		}
		if(bConnected == FALSE) start_counter++;
		if(start_counter >= 6000)
		{
			start_counter = 0;
			vAHI_DioSetOutput(LED_BOTTOM, 0);
			DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nRESET", 0);
            vPrintf("XTQD");
		}

	}
	else if (u32Device == E_AHI_DEVICE_UART1)
	{
	    uint8  received_char_uart1;
	    received_char_uart1 = u8AHI_UartReadData(E_AHI_UART_1);

	    if((start_juge1 == 0) && (received_char_uart1 == 'S'))
	    {
			start_juge1 = 1;
			received_data_uart1[0] = received_char_uart1;
	    }
	    else
	    {
	    	if(start_juge1 == 1)
	    	{
	    		received_data_uart1[received_num_uart1] = received_char_uart1;
	    	    received_num_uart1 = received_num_uart1 + 1;
	    	    if(received_num_uart1 == 36)
	    	    {
	    	    	received_num_uart1 = 1;
	    	        start_juge1 = 0;

	    		    if(received_data_uart1[35] == 'A' && received_data_uart1[31] == 'A') // ����ַ��Ϣ
	    			{
		    	         board_num[1] = received_data_uart1[1];
		    	         board_num[2] = received_data_uart1[2];

	    			     data_to_send[0] = 'S';
	    			     data_to_send[1] = 0x19;
	    			     data_to_send[2] = 0x43;
	    			     data_to_send[3] = (uint8)((u64LocalAddr >> 56) );
	    			     data_to_send[4] = (uint8)((u64LocalAddr >> 48) );
	    			     data_to_send[5] = (uint8)((u64LocalAddr >> 40) );
	    			     data_to_send[6] = (uint8)((u64LocalAddr >> 32) );
	    			     data_to_send[7] = (uint8)((u64LocalAddr >> 24) );
	    			     data_to_send[8] = (uint8)((u64LocalAddr >> 16) );
	    			     data_to_send[9] = (uint8)((u64LocalAddr >> 8)  );
	    			     data_to_send[10] = (uint8)(u64LocalAddr);

	    			     data_to_send[11] = 0xFF;

	    			     data_to_send[12]=board_num[1];
	    	 	     	 data_to_send[13]=board_num[2];

	    			     data_to_send[14] = 40;
	    			     data_to_send[15] = 0XFF;
	    			     data_to_send[16] = 0XFF;
	    			     data_to_send[17] = 0XFF;
	    			     data_to_send[18] = 0XFF;
	    			     data_to_send[19] = 0XFF;
	    			     data_to_send[20] = 0XFF;
	    			     data_to_send[21] = 0XFF;
	    			     data_to_send[22] = 0XFF;
	    			     data_to_send[23] = 0XFF;
	    			     data_to_send[24] = 0XFF;
	    			     data_to_send[25] = 0XFF;
	    			     data_to_send[26] = 0XFF;
	    			     data_to_send[27] = 0XFF;
	    			     data_to_send[28] = 0XFF;
	    			     data_to_send[29] = 0XFF;
	    			     data_to_send[30] = 0XFF;
	    			     data_to_send[31] = 0XFF;
	    			     data_to_send[32] = 0XFF;
	    			     data_to_send[33] = 0XFF;
	    			     data_to_send[34] = 'D';

	    			     if(SendDataToRemote(&sCommNodeAddr, data_to_send, 35) != 0)
						 {
							tx_error_num1 = tx_error_num1 + 1;
							if(tx_error_num1 >= 4)
							{
								vAHI_DioSetOutput(LED_BOTTOM, 0);
								DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nRESET", 0);
								vPrintf("XTQD");
							}
						 }
	    			}
	    		    else
	    		    {
	    		    	if(SendDataToRemote(&sCommNodeAddr, received_data_uart1, 36) != 0)
	    		    	{
	    		    		tx_error_num1 = tx_error_num1 + 1;
	    		    		if(tx_error_num1 >= 4)
	    		    		{
	    		    			vAHI_DioSetOutput(LED_BOTTOM, 0);
	    		    			DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nRESET", 0);
	    		    			vPrintf("XTQD");
	    		    		}
	    		    	}
	    		    }
	    	    }
	    	}
	    	 else
	         {
	    	     start_juge1 = 0;
	    	 }
	    }
	}
}


/**********************************************************************************************************
** Function name:     Node_eJipInit
** Descriptions:      ��ʼ��Э��ջ
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
	 *  ����JIP
	 */
	sJipInitData.u64AddressPrefix       = CONFIG_ADDRESS_PREFIX; 		/* ����WPAN��IPv6��ַ��ǰ׺��		*/
																		/* ֻ��Э��������Ҫ����			*/
	sJipInitData.u32Channel				= CONFIG_SCAN_CHANNELS;     	/* ָ�����õ��ŵ��Ż������� 		*/
																		/* Ҫɨ����ŵ���		  		*/

	sJipInitData.u16PanId				= CONFIG_PAN_ID;
	/*
	 *  ��������IP����С�����������+40����С��0��256~1280����Ϊ0��������Ϊ1280
	 */
	sJipInitData.u16MaxIpPacketSize		= 0;
	sJipInitData.u16NumPacketBuffers	= 2;    						/* Number of IP packet buffers 	*/
	sJipInitData.u8UdpSockets			= 2;           					/* Number of UDP sockets 		*/
																		/* supported 					*/
	sJipInitData.eDeviceType			= E_JIP_DEVICE_ROUTER;             	/* Device type (C, R, or ED) 	*/

	sJipInitData.u32RoutingTableEntries	= CONFIG_ROUTING_TABLE_ENTRIES; /* Routing table size (not ED) */

	/*
	 *  ÿ���ڵ㶼��һ��Device ID����ʶ���豸�����ͣ�����ͬһ�豸���͵��豸������ͬ��MIBs����Ӧ��MIB����
	 *  Device ID�������ֹ��ɣ�31~16Ϊ����ID��15~0Ϊ��ƷID
	 *  ����ID�����λ����ʶ��ʣ���15λ�Ƿ�����ЧID�������λΪ0��Ϊ��ЧID���ڿ����׶ν������λ����Ϊ0
	 */
	sJipInitData.u32DeviceId			= MK_JIP_DEVICE_ID;
	sJipInitData.u8UniqueWatchers		= CONFIG_UNIQUE_WATCHERS;
	sJipInitData.u8MaxTraps				= CONFIG_MAX_TRAPS;
	sJipInitData.u8QueueLength 			= CONFIG_QUEUE_LENGTH;
	sJipInitData.u8MaxNameLength		= CONFIG_MAX_NAME_LEN;			/* ���ýڵ�MIB�������������Ĵ�С	*/
	sJipInitData.u16Port				= JIP_DEFAULT_PORT;				/* ���ý���JIP���ݰ���UDP�˿ں�	*/
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



PRIVATE void vPutChar (uint8 c)
{
	while (!(u8AHI_UartReadLineStatus(UART) & E_AHI_UART_LS_THRE));
		vAHI_UartWriteData(UART, c);
	while ((u8AHI_UartReadLineStatus(UART) & (E_AHI_UART_LS_THRE | E_AHI_UART_LS_TEMT))
			!= (E_AHI_UART_LS_THRE | E_AHI_UART_LS_TEMT));
}

PRIVATE void vUartInit (void)
{
	#if (UART == E_AHI_UART_0)
	vAHI_UartSetRTSCTS(UART, FALSE); /* Disable use of CTS/RTS */
	#endif
	bAHI_UartEnable(E_AHI_UART_1,*pTxBuf,1000,*pRxBuf,1000);
	vAHI_UartSetLocation(UART,TRUE);
	vAHI_UartEnable(UART);
	vAHI_UartSetControl(E_AHI_UART_1, E_AHI_UART_EVEN_PARITY, E_AHI_UART_PARITY_DISABLE, E_AHI_UART_WORD_LEN_8, E_AHI_UART_1_STOP_BIT, E_AHI_UART_RTS_HIGH);
	vAHI_UartReset(UART, /* ��λ�շ�FIFO */
			TRUE,
			TRUE);
	vAHI_UartSetBaudRate(UART, E_AHI_UART_RATE_9600); /* ���ò�����*/
	vAHI_UartSetInterrupt(UART, FALSE, FALSE, FALSE, TRUE, E_AHI_UART_FIFO_LEVEL_1); /* �ر��ж� */
	vInitPrintf((void *)vPutChar);
}
PRIVATE void delay_nms(uint16 time)
{
   uint16 k=0;
   while(time--)
   {
      k=12000;  //�Լ�����
      while(k--) ;
   }
}

/********************************************************************************************************
	END OF FILE
********************************************************************************************************/
