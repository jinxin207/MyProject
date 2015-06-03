#include <types.h>

typedef struct
{
	
	uint32 SmartUUID;	////device uuid
	uint16 SmartADDR;		///local id, des id
	uint16 SmartGRUOP;	///group id
	uint16 SmartDataType;	///data type
	uint8   SmartDATA[6];	///len + 5* bytes data
	uint16 Random;
	uint8   Key[16];
}Smart_Data_Struct;


extern void SmartBuildData(void);
extern void SmartReadData(uint8* dat);
extern void SmartParserFrame(uint8* dat);
extern void SmartSendData(uint8* dat);

extern void SmartStartScan(bool sc);