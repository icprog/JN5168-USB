

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
PUBLIC  uint32  u32Second;  												/* Second counter 				*/
PRIVATE uint8   bTog=0;
PRIVATE uint8   turn_flag = 1;
PRIVATE uint8   i2c_counter = 0;
PRIVATE bool_t   bInitialised;
PRIVATE bool_t   bSleep;
/**********************************************************************************************************
    Other data
**********************************************************************************************************/
PRIVATE uint8   *pTxBuf ;
PRIVATE uint8   *pRxBuf ;

PRIVATE uint8  rxdata[1000]={0};
PRIVATE	uint16 rxdata_count = 0 ;
PRIVATE uint8 received_data_uart1[40];
PRIVATE uint8 received_num_uart1 =1;
PRIVATE uint8    data_to_send[1000]={0};
PRIVATE uint8 start_juge1 =0;
PRIVATE uint64 u64childAddr;
PRIVATE uint8 child_leave[15];
/**********************************************************************************************************
    Exported Functions
**********************************************************************************************************/
PUBLIC void Device_vInit(bool_t bWarmStart);
PUBLIC void Node_eJipInit(void);
PRIVATE void delay_16ms(uint8 t);

#define UART E_AHI_UART_1
PRIVATE void vPutChar(uint8 c) ;
PRIVATE void vUartInit(void);

#define LED                             (1 << 17)

//���ݽ��նˣ� ������Ҫ����Ӧ�� PORT���а󶨣� Ȼ����v6LP_DataEvent�¼��н���E_DATA_RECEIVED��Ϣ��
#define PROTOCOL_PORT				 	 (1190)

PRIVATE bool_t          	  bProtocolSocket;
PRIVATE int					  iProtocolSocket;
PRIVATE ts6LP_SockAddr 		  s6lpSockAddrLocal;
PRIVATE void Protocol_vSendTo(ts6LP_SockAddr *ps6lpSockAddr, uint8 *pu8Data, uint16 u16DataLen);

/**********************************************************************************************************
	Զ�̽ڵ��ַ
**********************************************************************************************************/
MAC_ExtAddr_s sParentNodeAddr;
MAC_ExtAddr_s sLocalNodeAddr;
MAC_ExtAddr_s sChildNodeAddr;
MAC_ExtAddr_s sCommNodeAddr;

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

/**********************************************************************************************************
** Function name:     SendDataToRemote
** Descriptions:      �������ݷ�������

**********************************************************************************************************/
PRIVATE int SendDataToRemote(MAC_ExtAddr_s *psMacAddr, uint8 *pu8Data, uint16 u16DataLen)
{
    uint8 i = 0;
    ts6LP_SockAddr s6LP_SockAddr;                                       /* ����ts6LP_SockAddr             */
    EUI64_s  sIntAddr;
    int iGetDataBufferResult;
    int iSendToResult;
    uint8 *pu8Buffer;
                                                                        /* vJIP_StackEvent��ֵ                */
    memset(&s6LP_SockAddr, 0, sizeof(ts6LP_SockAddr));                  /* ���s6LP_SockAddr�ṹ���ڴ�ֵ    */
    i6LP_CreateInterfaceIdFrom64(&sIntAddr, (EUI64_s *) psMacAddr);     /* �����ӿ�ID                   */
    i6LP_CreateLinkLocalAddress (&s6LP_SockAddr.sin6_addr, &sIntAddr);  /* �������ӱ��ص�ַ                 */
    /*
     * Complete full socket address
     */
    s6LP_SockAddr.sin6_family = E_6LP_PF_INET6;                         /* ��������ΪE_6LP_PF_INET6      */
    s6LP_SockAddr.sin6_flowinfo =0;                                     /* ����flow��Ϣ                     */
    s6LP_SockAddr.sin6_port = PROTOCOL_PORT;                            /* IPV6�˿ں�                  */
    s6LP_SockAddr.sin6_scope_id =0;                                     /* ����scope��Ϣ*/

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
** Descriptions:      ������
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
** Descriptions:      �������� memory hold ���߻��ѵ�ʱ�����
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
** Input parameters:  bWarmStart��TRUE��������FALSE������
** Output parameters: none
** Returned value:          none
**********************************************************************************************************/
PUBLIC void Device_vInit(bool_t bWarmStart)
{
	v6LP_InitHardware();
	vAHI_HighPowerModuleEnable(TRUE,TRUE);

	/* ��ʼ��Ӳ��		 		*/
	while(bAHI_Clock32MHzStable() == FALSE);  //set clock 32MHz ?? D							/* �ȴ�ʱ���ȶ� 					*/
	if (FALSE == bWarmStart)											/* �ж��Ƿ������� 				*/
	{
		Exception_vInit();												/* Initialise exception handler */
		/*
		 * Initialise all DIO as outputs and drive low
		 */
		vAHI_DioSetDirection(0, 0xFFFFFFFE);
		vAHI_DioSetOutput(0xFFFFFFFE, 0);

	}

	#ifdef DBG_ENABLE													/* �Ƿ���debug 						*/
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

		MibDioControl_vInit(hMibDioControl, &sMibDioControl);          /* ��ʼ��MIB */

		MibDioControl_vRegister();										/* ע��MIB */
		Node_eJipInit();												/* ��ʼ��Э��ջ 					*/

		u32AHI_Init();

		vUartInit();
	}



	while (1) {                                                           /*��ѭ��*/
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

												/* ������Э���� 					*/
	netSecurityKey.u32KeyVal_1 = 0x10101010;
	netSecurityKey.u32KeyVal_2 = 0x10101010;
	netSecurityKey.u32KeyVal_3 = 0x10101010;
	netSecurityKey.u32KeyVal_4 = 0x10101010;

	vApi_SetNwkKey(0, &netSecurityKey);									/* ����Ϊ�������� 				*/
	DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nCoordinator network key Load");

	if (bJnc_SetJoinProfile(3, NULL)) {									/* ���ü������ò��� 				*/
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nJoinProfile Successfully");
	} else {
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nJoinProfile Fail");
	}

	if (bJnc_SetRunProfile(3, NULL)) {									/* �����������ò��� 				*/
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nRunProfile Successfully");
	} else {
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nRunProfile Fail");
	}



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
		case E_DATA_RECEIVED:											/* Data received ? 				*/
		{
			DBG_vPrintf(DEBUG_DEVICE_FUNC, "E_DATA_RECEIVED");
			int					iRecvFromResult;
			ts6LP_SockAddr 	    s6lpSockAddr;
			uint8		       u8SockAddrLen;
			uint8    		  au8Data[256];
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
                for(k = 0; k < au8DataLen; k++)
                {
                    vPutChar(au8Data[k]);
                }

				if(turn_flag == 0)
				{
				    vAHI_DioSetOutput(0, LED);
					turn_flag = 1;
				}
				else
				{
					vAHI_DioSetOutput(LED, 0);
					turn_flag = 0;
				}
			}
		}
		break;
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
	bool_t bPollNoData;
	tsSecurityKey netSecurityKey;

	switch (eEvent) {
	case E_STACK_STARTED:												/* Э��ջ������Э������������ 		*/
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_STARTED", eEvent);
		memcpy((uint8 *)&sLocalNodeAddr, (uint8 *)&(((tsNwkInfo *)pu8Data)->sLocalAddr), 8);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nmy mac:%08x", ((tsNwkInfo *)pu8Data)->sLocalAddr.u32H);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "%08x", ((tsNwkInfo *)pu8Data)->sLocalAddr.u32L);
		Protocol_vOpenSocket();    //���ͷ�����Ҫ�� Socket.  ���շ���Ҫ��socket
		vAHI_DioSetOutput(0, LED);
		break;
	case E_STACK_JOINED:												/* Network is now up 			*/
		/*
		 * �����ڵ��mac��ַ���浽sCommNodeAddr������ӡ��
		 */
		memcpy((uint8 *)&sParentNodeAddr, (uint8 *)&(((tsNwkInfo *)pu8Data)->sParentAddr), 8);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_JOINED", eEvent);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nmy parent's mac:%08x", ((tsNwkInfo *)pu8Data)->sParentAddr.u32H);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "%08x", ((tsNwkInfo *)pu8Data)->sParentAddr.u32L);

		break;
	case E_STACK_NODE_JOINED:
		/*
		 * ���ӽڵ��mac��ַ���浽sCommNodeAddr������ӡ��
		 */
		memcpy((uint8 *)&sChildNodeAddr, (uint8 *)&(((tsAssocNodeInfo *)pu8Data)->sMacAddr), 8);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_NODE_JOINED", eEvent);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\njoin node mac:%08x", ((tsAssocNodeInfo *)pu8Data)->sMacAddr.u32H);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "%08x", ((tsAssocNodeInfo *)pu8Data)->sMacAddr.u32L);
		DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nProtocol_vOpenSocket()", eEvent);

		break;
	case E_STACK_NODE_LEFT:

	    memcpy((uint8 *)&sChildNodeAddr, (uint8 *)&(((tsAssocNodeInfo *)pu8Data)->sMacAddr), 8);
	    u64childAddr = (((uint64)(sChildNodeAddr.u32H)) << 32) + sChildNodeAddr.u32L;

	    DBG_vPrintf(DEBUG_DEVICE_FUNC, "\nE_STACK_NODE_LEFT", eEvent);

        child_leave[0]='C';
        child_leave[1] = (uint8)((u64childAddr >> 56) );
        child_leave[2] = (uint8)((u64childAddr >> 48) );
        child_leave[3] = (uint8)((u64childAddr >> 40) );
        child_leave[4] = (uint8)((u64childAddr >> 32) );
        child_leave[5] = (uint8)((u64childAddr >> 24) );
        child_leave[6] = (uint8)((u64childAddr >> 16) );
        child_leave[7] = (uint8)((u64childAddr >> 8)  );
        child_leave[8] = (uint8)(u64childAddr);
        child_leave[9]='L';

        uint8 n = 5, num;
        while(n>0)
        {
          n=n-1;
          for(num=0;num<10;num++)
          {
              vPutChar(child_leave[num]);
          }
        }

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
    uint32  AddrH, AddrL;
	/*
	 * Tick Timer is run by stack at 10ms intervals    Э��ջ���Զ�����һ��10ms�����tick timer�жϡ�  ��������Լ��趨tick timer����Ч��
	 */
	if (u32Device == E_AHI_DEVICE_TICK_TIMER)
	{
		if (bInitialised == TRUE)										/* Initialised ? 				*/
		{
			if (u8TickQueue < 100) u8TickQueue++;						/* Increment pending ticks 		*/
		}

		/**********************************************************dong test ********************/
		i2c_counter++;

   }
	else if (u32Device == E_AHI_DEVICE_UART1)
	{
	    uint8  received_char_uart1;
	    received_char_uart1 = u8AHI_UartReadData(UART);
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

                 if(received_num_uart1 == 35)
                 {
                     start_juge1 = 0;
                     received_num_uart1 = 1;
                     if((received_data_uart1[0] == 'S') && (received_data_uart1[34] == 'E'))
                     {
                        AddrH = ((uint32)(received_data_uart1[3])) << 24;
                        AddrH = AddrH | (((uint32)(received_data_uart1[4])) << 16);
                        AddrH = AddrH | (((uint32)(received_data_uart1[5])) << 8 );
                        AddrH = AddrH |  ((uint32)(received_data_uart1[6]));

                        AddrL = ((uint32)(received_data_uart1[7])) << 24;
                        AddrL = AddrL | (((uint32)(received_data_uart1[8])) << 16);
                        AddrL = AddrL | (((uint32)(received_data_uart1[9])) << 8 );
                        AddrL = AddrL |  ((uint32)(received_data_uart1[10]));

                        sCommNodeAddr.u32H = AddrH;
                        sCommNodeAddr.u32L = AddrL;

                        SendDataToRemote(&sCommNodeAddr, received_data_uart1, 35);
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
	sJipInitData.eDeviceType			= E_JIP_DEVICE_COORDINATOR;             	/* Device type (C, R, or ED) 	*/

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

PRIVATE void delay_16ms(uint8 t)
{
	uint16 i = 0;
	uint16 j = 0;
	uint8 delay_num = 0;

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



/********************************************************************************************************
	END OF FILE
********************************************************************************************************/
