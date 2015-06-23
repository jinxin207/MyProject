#include "TEA.H"
#include "string.h"

//TEA��Կ
uint8 TEA_key[16]={0xf3, 0x67, 0x10, 0x32, 0xda, 0x8c, 0x50, 0x4b,
				 0xe6, 0x9a, 0x69, 0x27, 0xac, 0x4f, 0x92, 0x7d};

/********************************************************************* 
*                           ���� 
**********************************************************************/  
  /********************************************************************* 
*                           tea���� 
*����:v:Ҫ���ܵ�����,����Ϊ8�ֽ� 
*     k:�����õ�key,����Ϊ16�ֽ� 
**********************************************************************/  
/********************************************************************* 
*                           �����㷨 
*����:src:Դ����,��ռ�ռ����Ϊ8�ֽڵı���.������ɺ�����Ҳ������� 
*     size_src:Դ���ݴ�С,��λ�ֽ� 
*     key:��Կ,16�ֽ� 
*����:���ĵ��ֽ��� 
**********************************************************************/  
  
extern void encrypt(uint8 *src,uint16 size_src,uint8 *key)  
{  
	uint8 mem[16];
	uint8 i = 0;
	memcpy(mem, src, 16);

	for(i=0; i<16; i++)
	{
		src[i] = mem[i] ^ key[i];
	}
}  
  
/********************************************************************* 
*                           �����㷨 
*����:src:Դ����,��ռ�ռ����Ϊ8�ֽڵı���.������ɺ�����Ҳ������� 
*     size_src:Դ���ݴ�С,��λ�ֽ� 
*     key:��Կ,16�ֽ� 
*����:���ĵ��ֽ���,���ʧ��,����0 
**********************************************************************/  
  
extern void decrypt(uint8 *src,uint16 size_src,uint8 *key)  
{  
	uint8 mem[16];
	uint8 i = 0;
	memcpy(mem, src, 16);

	for(i=0; i<16; i++)
	{
		src[i] = mem[i] ^ key[i];
	}
}  

#define WORD_MSB(_val)              ( ((_val) & 0xff00) >> 8 )
/*! \brief Extract the LSB of a 16-bit integer. */
#define WORD_LSB(_val)              ( ((_val) & 0x00ff) )

extern void KeyConvert(uint16 k, uint8* key)
{
	uint8 i =0;
	uint8 x = (uint8)(k>>8 & 0x00ff);
	x += (uint8)(k&0x00ff);
	for(i=0; i<16; i++)
	{
		key[i] = TEA_key[i] ^ x;
	}
}


extern uint32 WORD16_TO_WORD32(uint16 x, uint16 y)
{
	uint32 r = 0;
	r = SWAP_WORD16(x);
	r <<= 16;
	r += SWAP_WORD16(y);
	return r;
}

extern uint16 BYTE8_TO_WORD16(uint8 x, uint8 y)
{
	uint16 r = 0;
	r = x;
	r <<= 8;
	r += y;
	return r;
}

