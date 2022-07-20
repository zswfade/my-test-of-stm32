/*
* 文件名：Flash.h
* 作者：lsw
* 时间：2021-5-10
* 内容：flash数据存储模块
*/
#ifndef _FLASH_H_
#define _FLASH_H_

#include "stm32f10x.h"
#include "stm32f10x_flash.h"

#define STM32_FLASH_BASE (0x8000000 + BOOTLOADER_SIZE)   // STM32 Bootloader FLASH的起始地址
#define STM32_FLASH_SIZE 128 	      // STM32f103CBT6的FLASH容量大小(单位为2KB)
#define STM_SECTOR_SIZE  2048         // STM32f103一个扇区的大小(单位为byte)
#define STM32_DATA_SIZE  5

#define STM32_FLASH_FACTORY_DATA 0x0801D800  // (STM32_FLASH_SIZE/2 - STM32_DATA_SIZE)*2048出厂数据(wifi名字密码ip端口...)
#define STM32_FLASH_LEAD_OBJECT_SECTOR (STM32_FLASH_FACTORY_DATA + STM_SECTOR_SIZE)  // STM32 FLASH中object存储的引导信息

// 两个扇区分担擦写次数
#define STM32_FLASH_BASE1   (STM32_FLASH_LEAD_OBJECT_SECTOR + STM_SECTOR_SIZE)  // STM32 FLASH中object存储的第一地址
#define STM32_FLASH_BASE2   (STM32_FLASH_BASE1 + STM_SECTOR_SIZE)  // STM32 FLASH中object存储的第二地址
#define STM32_FLASH_END     (STM32_FLASH_BASE2 + STM_SECTOR_SIZE)  // 设置最后一个扇区为上限

typedef struct STFlashObject TFlashObject;
typedef TFlashObject* PFlashObject;

// 写入flash的方法
typedef u16 (*FFlashWriteObject)(PFlashObject flash_object, void* object);

// flash对象
struct STFlashObject
{
    u16 object_size;               // object对象的大小
    u16 object_size_u16_num;       // object对象的大小需要的半字数
    u16 object_addr_index;
    u16 object_addr_i;
    u16 is_erase_sector;           // 是否擦除扇区
    u32 object_write_addr;         // object对象的数据的写入地址
    FFlashWriteObject FlashWriteObject; // 写入flash的方法
};

void InitFlashObject(PFlashObject flash_object, void* object);

// 对象数据写入Flash
void FLASH_ObjectWrite(PFlashObject flash_object, void* object);
// 从Flash读取对象数据
void FLASH_ObjectRead(PFlashObject flash_object, void* object);
// 写入系统所有需要掉电保存的数据
u16 FLASH_Write_All(PFlashObject flash_object, void* object);

// 读取指定地址的半字(16位数据)
// faddr:读地址(此地址必须为偶数!!)
// 返回值:对应数据
u16 STMFLASH_ReadHalfWord(u32 flash_addr);

// 不检查传入参数的正确性，提高效率，需要调用时保证参数正确性
// WriteAddr:写入位置起始地址
// Buffer:写入缓冲区
// NumToWrite:半字(16位)数   
void STMFLASH_WriteNoCheck(u32 WriteAddr, u16 *Buffer, u16 NumToWrite);

// 从指定地址开始读出指定长度的数据
// ReadAddr:起始地址
// pBuffer:数据指针
// NumToWrite:半字(16位)数
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);

// 向flash写入数据
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);
// 擦除Flash
char FlashErasePage(uint32_t EraseAddress,u16 PageCnt);
// 指定地址写入一个16位数据
char FlashWriteHalfWord(uint32_t WriteAddress,u16 data);
extern TFlashObject flash;
#endif
