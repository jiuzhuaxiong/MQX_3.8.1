void uart_isr();
extern uchar read_buffer[7];  // ����������ʱ��������
extern void LCDTouchSel(void); // �����������жϳ���

extern unsigned int  DataSize; // DMA �� major count �Ĵ������൱��һ��DMA����������ݵĸ���

extern uchar BufRxchar[6175]; // DMA�������ݵĻ�����
extern void DMA_RecData_OK(); //DMA ���ݽ�����ɺ�Ĵ�����
extern uchar SPI_Send; // WK @130406 --> SPI�Ƿ����·������ݵı�־
extern uchar SysDataSend[56]; // K60д��DSP������

extern void DisTimeFlg(); //wk @130510 --> KRTC.C
//#define _UART_DBUG_  // wk --> �����ã��Ƿ���Ҫ������Ϣ���
//#define _SPIDMA_DBUG_

