
#include "main.h"

#define UARTx                   USART1
#define GPIOx                   GPIOA
#define TX_PIN					GPIO_Pin_9
#define RX_PIN					GPIO_Pin_10
#define DMAx_Stream_Rx          DMA2_Stream2
#define DMA_Channel_Rx          DMA_Channel_4  
#define BUF_RX_SIZE             512*100

enum xioCodes {
	XIO_OK = 0,				// OK - ALWAYS ZERO
	XIO_ERR,				// generic error return (errors start here)
	XIO_EAGAIN,				// function would block here (must be called again)
	XIO_NOOP,				// function had no-operation
	XIO_COMPLETE,			// operation complete
	XIO_TERMINATE,			// operation terminated (gracefully)
	XIO_RESET,				// operation reset (ungraceful)
	XIO_EOL,				// function returned end-of-line
	XIO_EOF,				// function returned end-of-file
	XIO_FILE_NOT_OPEN,		// file is not open
	XIO_FILE_SIZE_EXCEEDED, // maximum file size exceeded
	XIO_NO_SUCH_DEVICE,		// illegal or unavailable device
	XIO_BUFFER_EMPTY,		// more of a statement of fact than an error code
	XIO_BUFFER_FULL,
	XIO_BUFFER_FULL_FATAL,
	XIO_INITIALIZING,		// system initializing, not ready for use
	XIO_ERROR_16,			// reserved
	XIO_ERROR_17,			// reserved
	XIO_ERROR_18,			// reserved
	XIO_ERROR_19			// NOTE: XIO codes align to here
};

#define LF	0x0A		// ^j - line feed
#define CR	0x0D		// ^m - carriage return
	
static u8 bufRx[BUF_RX_SIZE];
static u16 qiShiPos,dmaDuPos;

static void DMA_Config(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
	
	
	DMA_InitStructure.DMA_Channel = DMA_Channel_Rx;  
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UARTx->DR;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)bufRx;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = BUF_RX_SIZE;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMAx_Stream_Rx, &DMA_InitStructure);
	
}

static void USART_Config(void)
{
  USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
	
	
  /* Connect PXx to USARTx_Tx*/
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
  /* Connect PXx to USARTx_Rx*/
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
	
	
  /* Configure USART Tx as alternate function  */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
  GPIO_InitStructure.GPIO_Pin = TX_PIN;
  GPIO_Init(GPIOx, &GPIO_InitStructure);

	
  /* Configure USART Rx as alternate function  */
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = RX_PIN;
  GPIO_Init(GPIOx, &GPIO_InitStructure);
  
  
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(UARTx , &USART_InitStructure);
  
  USART_DMACmd(UARTx,USART_DMAReq_Rx,ENABLE);
	
  
	
}

/*****************  发送一个字符 **********************/
void xuart_SendByte( uint8_t ch)
{
	/* 发送一个字节数据到USART */
	USART_SendData(UARTx,ch);
		
	/* 等待发送数据寄存器为空 */
	while (USART_GetFlagStatus(UARTx, USART_FLAG_TXE) == RESET);	
}

///重定向c库函数printf到串口，重定向后可使用printf函数
int fputc(int ch, FILE *f)
{
		/* 发送一个字节数据到串口 */
		xuart_SendByte( (uint8_t) ch);
		return (ch);
}

int xio_usart_gets(char *buf, const int size)
{
	u16 pos=BUF_RX_SIZE-DMAx_Stream_Rx->NDTR;
	u8 v;
	if(pos == dmaDuPos)
		return (XIO_EAGAIN);
	while(dmaDuPos!=pos)
	{
		v=bufRx[dmaDuPos];
		if(++dmaDuPos == BUF_RX_SIZE)
			dmaDuPos=0;
		if(v==CR || v==LF){
			int s=0;
			while(qiShiPos != dmaDuPos){
				if(s<size-1){
					*buf=bufRx[qiShiPos];
					buf++;
				}
				s++;
				if(++qiShiPos == BUF_RX_SIZE)
					qiShiPos=0;
			}
			if(s){
				buf--;
				*buf=0;
			}
			if(s>1){
				return (XIO_OK);
			}else{
				return (XIO_EAGAIN);
			}
			
		}
	}
	return (XIO_EAGAIN);
}

//初始化
void xio_usart_Init(void)
{
	
	qiShiPos=0;
	dmaDuPos=0;
	
	USART_Config();
	DMA_Config();
	
	
	USART_Cmd(UARTx , ENABLE);
	DMA_Cmd(DMAx_Stream_Rx,ENABLE);
	//printf("这是一个串口中断接收回显实验\n");
}


