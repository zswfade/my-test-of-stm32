#include "flash.h"
#include "object.h"
#include <string.h>

FLASH_Status FlashStatus;
u16 flash_read_buff[STM_SECTOR_SIZE / 2]; // 读取扇区数据的缓冲区

void InitFlashObject(PFlashObject flash_object, void* object)
{
	u16 temp;
	TRunObject* this_object = (TRunObject*)object;
	//======================================object============================================//
	this_object->write_flash_addr_no = 1; // 默认在STM32_FLASH_BASE1扇区写入object数据
	// flash_object->FlashWriteObject = FLASH_Write_All; // 设置object保存方法
    flash_object->object_size = &this_object->addr_1 - &this_object->addr_0; // 获取需写入的长度
	flash_object->object_size_u16_num = flash_object->object_size / 2;
	temp = STMFLASH_ReadHalfWord(STM32_FLASH_LEAD_OBJECT_SECTOR);
	// 判断该区间是否被初始化
	if (temp != 0xABCD)
	{
		flash_object->object_addr_index = 2;
		// 初始化object在flash中的值
        STMFLASH_Write(STM32_FLASH_BASE1, (u16*)object, flash_object->object_size_u16_num);
        this_object->write_flash_addr_no = 2; // 默认在STM32_FLASH_BASE1扇区写入object数据

		FLASH_Unlock();
        // 初始化lead_object扇区
        FLASH_ErasePage(STM32_FLASH_LEAD_OBJECT_SECTOR); // 擦除这个扇区
		FLASH_ProgramHalfWord(STM32_FLASH_LEAD_OBJECT_SECTOR + flash_object->object_addr_index, this_object->write_flash_addr_no); // 写入write_flash_addr_no
		FLASH_ProgramHalfWord(STM32_FLASH_LEAD_OBJECT_SECTOR, 0xABCD); // 写入标志位
		FLASH_Lock();
	}
#if STM_SECTOR_SIZE != 2048
#error STM_SECTOR_SIZE is changed !
#endif
	// 读取标志位并且计算出object_addr_index
	for (flash_object->object_addr_index = 2; flash_object->object_addr_index <= 2000; flash_object->object_addr_index += 2)
	{
		temp = STMFLASH_ReadHalfWord(STM32_FLASH_LEAD_OBJECT_SECTOR + flash_object->object_addr_index);
		if (temp == 0xFFFF)
		    break;
		else
		    this_object->write_flash_addr_no = temp;
	}
	// 读取出所有数据
	FLASH_ObjectRead(flash_object, object);
}


// 写入object引导数据(控制object写入哪个扇区)
static void FLASH_WriteObjectLeadData(PFlashObject flash_object, void* object)
{
	TRunObject* this_object = (TRunObject*)object;
#if STM_SECTOR_SIZE != 2048
#error STM_SECTOR_SIZE is changed !
#endif
    if (flash_object->object_addr_index >= 2000) // 达到设定上限，擦除扇区重新写入 2000 == STM_SECTOR_SIZE - 24(随意)
	{
		flash_object->object_addr_index = 2;
        FLASH_Unlock();
        FLASH_ErasePage(STM32_FLASH_LEAD_OBJECT_SECTOR); // 擦除这个扇区
		FLASH_ProgramHalfWord(STM32_FLASH_LEAD_OBJECT_SECTOR, 0xABCD); // 写入标志位
		FLASH_ProgramHalfWord(STM32_FLASH_LEAD_OBJECT_SECTOR + flash_object->object_addr_index, this_object->write_flash_addr_no); // 写入write_flash_addr_no
		FLASH_Lock();
		flash_object->object_addr_index += 2;
	}
	else
	{
        FLASH_Unlock();
		FLASH_ProgramHalfWord(STM32_FLASH_LEAD_OBJECT_SECTOR + flash_object->object_addr_index, this_object->write_flash_addr_no); // 写入write_flash_addr_no
        FLASH_Lock();
		flash_object->object_addr_index += 2;
	}
}

// 对象数据写入Flash
void FLASH_ObjectWrite(PFlashObject flash_object, void* object)
{
	u16* buff = (u16*)object;
	TRunObject* this_object = (TRunObject*)object;
    // 关闭spi 以免影响到时序
    // this_object->abs_encoder_object.SpiSwitch(0,this_object);

    if (this_object->write_flash_addr_no == 1)
	{
        STMFLASH_Write(STM32_FLASH_BASE1, buff, flash_object->object_size_u16_num);
		this_object->write_flash_addr_no = 2;
	}

    else if (this_object->write_flash_addr_no == 2)
	{
        STMFLASH_Write(STM32_FLASH_BASE2, buff, flash_object->object_size_u16_num);
		this_object->write_flash_addr_no = 1;
	}
	else
	{
		STMFLASH_Write(STM32_FLASH_BASE1, buff, flash_object->object_size_u16_num);
		this_object->write_flash_addr_no = 1;
	}
	FLASH_WriteObjectLeadData(flash_object, object); // 写入write_flash_addr_no使开机时能够知道读取哪一块扇区
    // 打开spi
    // this_object->abs_encoder_object.SpiSwitch(1,this_object);
}

// 从Flash读取对象数据
void FLASH_ObjectRead(PFlashObject flash_object, void* object)
{
	u16* buff = (u16*)object;
	TRunObject* this_object = (TRunObject*)object;
    if (this_object->write_flash_addr_no == 1)
    {
		STMFLASH_Read(STM32_FLASH_BASE2, buff, flash_object->object_size_u16_num);
	}
    else if (this_object->write_flash_addr_no == 2)
	{
		STMFLASH_Read(STM32_FLASH_BASE1, buff, flash_object->object_size_u16_num);
	}
	else
	{
		STMFLASH_Read(STM32_FLASH_BASE1, buff, flash_object->object_size_u16_num);
	}
}


// 读取指定地址的半字(16位数据)
// faddr:读地址(此地址必须为偶数!!)
// 返回值:对应数据
u16 STMFLASH_ReadHalfWord(u32 flash_addr)
{
	return *(vu16*)flash_addr; 
}

//从指定地址开始读出指定长度的数据
//ReadAddr:起始地址
//pBuffer:数据指针
//NumToWrite:半字(16位)数
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead)   	
{
	u16 i;
	for(i = 0; i < NumToRead; i++)
	{
		pBuffer[i] = STMFLASH_ReadHalfWord(ReadAddr);//读取2个字节.
		ReadAddr += 2;//偏移2个字节.	
	}
}
// 不检查传入参数的正确性，提高效率，需要调用时保证参数正确性
// WriteAddr:写入位置起始地址
// Buffer:写入缓冲区
// NumToWrite:半字(16位)数
void STMFLASH_WriteNoCheck(u32 WriteAddr, u16 *Buffer, u16 NumToWrite)   
{ 			 		 
	u16 i;
	for(i = 0; i < NumToWrite; i++)
	{
		FLASH_ProgramHalfWord(WriteAddr, Buffer[i]); // 写入十六位数据
	    WriteAddr += 2; // 地址增加2.
	}  
}
// 注意：WriteAddr只能是1024的倍数
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)
{
	FLASH_Unlock();
	FLASH_ErasePage(WriteAddr);
    STMFLASH_WriteNoCheck(WriteAddr, pBuffer, NumToWrite);
	FLASH_Lock();
}

/**
* @brief  擦除Flash
* @waing  
* @param  EraseAddress  擦除的地址
* @param  PageCnt       擦除连续的几页
* @param  None
* @retval 4:成功
* @example 
**/
char FlashErasePage(uint32_t EraseAddress,u16 PageCnt)
{
	u32 i=0;
	u32 secpos;	   //扇区地址
	if(EraseAddress<STM32_FLASH_BASE||(EraseAddress>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE)))return 0;//非法地址
	secpos = EraseAddress-STM32_FLASH_BASE;//实际地址
	secpos = secpos/STM_SECTOR_SIZE;//扇区地址 
	if(STM_SECTOR_SIZE==2048){PageCnt=PageCnt/2;}
	if(FLASH_GetStatus() == FLASH_COMPLETE)//可以操作Flash
	{
		FLASH_Unlock();
		for(i=0;i<PageCnt;i++)
		{
			FlashStatus = FLASH_ErasePage(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE);//擦除这个扇区
			if(FlashStatus != FLASH_COMPLETE) break;
			secpos++;
		}
		FLASH_Lock();//上锁
	}	
	return FlashStatus;
}

/**
* @brief  指定地址写入一个16位数据
* @waing  写入的地址请先擦除
* @param  WriteAddress  写入的地址
* @param  data          写入的数据
* @param  None
* @retval 0:成功
* @example 
**/
char FlashWriteHalfWord(uint32_t WriteAddress,u16 data)
{
	FlashStatus = FLASH_BUSY;//设置为忙
	if(FLASH_GetStatus() == FLASH_COMPLETE)//可以操作Flash
	{
		FLASH_Unlock();		
		FlashStatus = FLASH_ProgramHalfWord(WriteAddress,data);//写入数据
		
		if(FlashStatus == FLASH_COMPLETE)//操作完成
		{
			if(STMFLASH_ReadHalfWord(WriteAddress) == data)//读出的和写入的一致
			{
				FLASH_Lock();//上锁
				return 0;
			}
			else
			{
				FLASH_Lock();//上锁
				return 5;
			}
		}
		FLASH_Lock();//上锁
	}
	return FlashStatus;
} 
