/**
 * 功能：16位与32位的启动混合代码
 *
 */
	#include "os.h"

	// 声明本地以下符号是全局的，在其它源文件中可以访问
	.global _start, timer_int

	// 指定以下的代码生成16位的机器指令，这样才能在启动时的实模式下运行
  	.code16

	// 以下是代码区
 	.text
_start: 
	// 根据16位实模式平坦模型初始化段寄存器都为0
	mov $0, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %gs
	mov %ax, %fs

	// 将栈空间指针移动到栈底，栈从高位向低位存储，高位为栈底，低位为栈顶
	mov $_start, %esp

	// 利用BIOS加载剩余代码到内存大小为32KB，64个扇区，通过寄存器传递参数
read_self_all:
	mov $_start_32, %bx			// 需要存放在内存中的起始地址
	mov $0x2, %cx				// 指定起始扇区号
	mov $0x240, %ax 			// AH为2指明读磁盘操作，AL指明读取的扇区数为64
	mov $0x80, %dx 				// 指明读取哪块磁盘，第一块磁盘
	int $0x13 					// 软中断指令，通过中断向量表调用BIOS中的加载函数
	jc read_self_all 			// 条件跳转指令，如果读取失败会将EFLAGS的CF置一，循环进行读取，直至成功


	cli							// 关中断
	lgdt gdt_desc				// 加载GDT表
	lidt idt_desc				// 加载IDT表
	mov $1, %eax				// CR0寄存器PE位置一进入保护模式
	lmsw %ax
	jmp $KERNEL_CODE_SEG, $_start_32

	.org 0x1fe	//510
	.byte 0x55, 0xaa


	// 指定以下的代码生成32位的机器指令，这样才能在启动时的实模式下运行
  	.code32

	// 以下是代码区
 	.text
	
_start_32:
	mov $KERNEL_DATA_SEG, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %gs
	mov %ax, %fs

	// 将栈空间指针移动到栈底，栈从高位向低位存储，高位为栈底，低位为栈顶
	mov $_start, %esp

	call os_init

	// 配置CR3寄存器，使其指向页目录表
	mov $pg_dir, %eax
	mov %eax, %cr3

	// 配置CR4寄存器，第4位置一，打开4M映射
	mov %cr4, %eax
	orl $(1 << 4), %eax
	mov %eax, %cr4

	// 配置CR0寄存器，最高位置一，打开分页机制
	mov %cr0, %eax
	orl $(1 << 31), %eax
	mov %eax, %cr0

	sti
	jmp .

timer_int:

	push %ds		//保护现场，将DS段寄存器压栈
	pusha			//保护现场，将部分通用寄存器的值压栈	Inter开发手册第二卷PUSHA/PUSHAD—Push All General-Purpose Registers

	//中断返回前向8259发送0x20
	mov $0x20, %al
	outb %al, $0x20

	popa			//恢复现场
	pop %ds

	iret			//中断返回指令

gdt_desc:
	.word(256*8) - 1
	.long gdt_table

idt_desc:
	.word(256*8) - 1
	.long idt_table