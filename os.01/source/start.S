/**
 * 功能：16位与32位的启动混合代码
 *
 */
	#include "os.h"

	// 声明本地以下符号是全局的，在其它源文件中可以访问
	.global _start

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
	mov $_start_32, %bx // 需要存放在内存中的起始地址
	mov $0x2, %cx	// 指定起始扇区号
	mov $0x240, %ax //AH为2指明读磁盘操作，AL指明读取的扇区数为64
	mov $0x80, %dx // 指明读取哪块磁盘，第一块磁盘
	int $0x13 // 软中断指令，通过中断向量表调用BIOS中的加载函数
	jc read_self_all // 条件跳转指令，如果读取失败会将EFLAGS的CF置一，循环进行读取，直至成功

	jmp .

	.org 0x1fe	//510
	.byte 0x55, 0xaa
	
_start_32:
	.fill 64*1024, 1, 0x35