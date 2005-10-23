#include <kernel/kernel.h>
#include <kernel/console.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/mm/mm.h>
#include <kernel/mm/paging.h>
#include <kernel/debug.h>
#include <kernel/tasking/scheduler.h>

int kernel_lockCount = 0;

BYTE inportb( WORD port )
{
    BYTE rv;
    ASM( "inb %1, %0" : "=a" (rv) : "dN" (port) );
    return rv;
}

void outportb( WORD port, BYTE data )
{
    ASM( "outb %1, %0" : : "dN" (port), "a" (data) );
}

inline void kernel_lock()
{
	cli();
	kernel_lockCount++;
}

inline void kernel_unlock()
{
	if( --kernel_lockCount == 0 )
		sti();
}

void kernel_init( struct MULTIBOOT_INFO * m )
{
	//char * p, * q;

	{
		kernel_lock();
		
		console_init();
		
		gdt_init();
		
		idt_init();
	
		irq_init();
		
		mm_init( m->mem_upper );
		
		scheduler_init();
		
		kernel_unlock();
	}
/*	
	kprintf( "going to malloc some memory\n" );
	p = mm_malloc( 32 );
	kprintf( "p = %x\n", p );
	//mm_free( p );
	q = mm_malloc( 32 );
	kprintf( "q = %x\n", q );
*/
    while(TRUE);
}
