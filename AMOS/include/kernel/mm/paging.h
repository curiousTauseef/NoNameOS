#ifndef _KERNEL_MM_PAGING_H_
#define _KERNEL_MM_PAGING_H_

#include <sys/types.h>
#include <kernel/tasking/task.h>

#define PAGE_ENTRYS		1024

#define PAGE_SIZE		SIZE_4KB

#define PAGE_ALIGN( address )	(void *)( ( ((DWORD)address) + PAGE_SIZE - 1 ) & ~( PAGE_SIZE - 1 ) )

#define V2P( address ) (void *)( (DWORD)address - 0xC0001000 + 0x00101000 )

// see page 3-26
#define SUPERVISOR	0x00
#define USER		0x01

#define READONLY	0x00
#define READWRITE	0x01

// see page 3-21
#define OFFSET_MASK	0x03FF

#define DIRECTORY_SHIFT_R( address )		( (DWORD)address >> 22 )
#define TABLE_SHIFT_R( address )			( (DWORD)address >> 12 )
#define TABLE_SHIFT_L( address )			( (DWORD)address << 12 )

#define GET_DIRECTORY_INDEX( linearAddress )( ( DIRECTORY_SHIFT_R(linearAddress) ) & OFFSET_MASK )

#define GET_TABLE_INDEX( linearAddress )( ( TABLE_SHIFT_R( linearAddress ) ) & OFFSET_MASK )

// From Intel IA32 Architecture Software Developers Manual Vol. 3 (3-24)
struct PAGE_DIRECTORY_ENTRY
{
    unsigned int present:1;
    unsigned int readwrite:1;
    unsigned int user:1;
    unsigned int writethrough:1;
    unsigned int cachedisabled:1;
    unsigned int accessed:1;
    unsigned int reserved:1;
    unsigned int pagesize:1;
    unsigned int globalpage:1;
    unsigned int available:3;
    unsigned int address:20;
} PACKED;

struct PAGE_TABLE_ENTRY
{
    unsigned int present:1;
    unsigned int readwrite:1;
    unsigned int user:1;
    unsigned int writethrough:1;
    unsigned int cachedisabled:1;
    unsigned int accessed:1;
    unsigned int dirty:1;
    unsigned int attributeindex:1;
    unsigned int globalpage:1;
    unsigned int available:3;
    unsigned int address:20;
} PACKED;

struct PAGE_DIRECTORY
{
    struct PAGE_DIRECTORY_ENTRY entry[PAGE_ENTRYS];
};

struct PAGE_TABLE
{
    struct PAGE_TABLE_ENTRY entry[PAGE_ENTRYS];
};

struct PAGE_DIRECTORY * paging_getCurrentPageDir();

void paging_setCurrentPageDir( struct PAGE_DIRECTORY * );

struct PAGE_DIRECTORY_ENTRY * paging_getPageDirectoryEntry( struct PAGE_DIRECTORY *, void * );

void paging_clearDirectory( struct PAGE_DIRECTORY * );

struct PAGE_TABLE_ENTRY * paging_getPageTableEntry( struct PAGE_DIRECTORY *, void * );

void paging_setPageTableEntry( struct PAGE_DIRECTORY *, void *, void *, BOOL );

//void paging_setDirectoryTableEntry( struct PAGE_DIRECTORY *, void *, void * );
void paging_setPageDirectoryEntry( struct PAGE_DIRECTORY *, void *, void *, BOOL );

struct PAGE_DIRECTORY * paging_createDirectory();

void paging_destroyDirectory( struct PAGE_DIRECTORY * );

void paging_mapKernel( struct PAGE_DIRECTORY * );

void paging_mapKernelHeap( struct PAGE_DIRECTORY * );

void paging_init();

DWORD paging_pageFaultHandler( struct TASK_STACK * );

#endif
