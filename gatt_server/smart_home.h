#include <types.h>

typedef struct
{
	uint16 Random;
	uint8   Key[16];
	uint32 SmartUUID;
	uint16 LocalADDR;
	uint16 SmartGRUOP;
	uint8   SmartDATA[6];
}Smart_Data_Struct;


extern void SmartBuildData(void);
extern void SmartReadData(uint8* dat);
extern void SmartParserFrame(uint8* dat);
extern void SmartSendData(uint8* dat);

extern void SmartStartScan(bool sc);