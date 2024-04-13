/**
 * 功能：32位代码，完成多任务的运行
 *
 */
#include "os.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define MAP_ADDR	0x80000000
#define true		1
#define false		0

void task_0(){
	uint8_t color = 0;

	while (true)
	{
		color ++;
	}
	
}

void task_1(){
	uint8_t color = 0xff;

	while (true)
	{
		color --;
	}
	
}

#define PDE_P		(1 << 0)
#define PDE_W		(1 << 1)
#define PDE_U		(1 << 2)
#define PDE_PS		(1 << 7)

uint8_t map_phy_buffer[4096] __attribute__((aligned(4096))) = {0x36}; //定义4K实际物理内存空间

static uint32_t page_table[1024] __attribute__((aligned(4096))) = {PDE_U};  //定义一个4K页表

// 32位页目录表结构第一项，4K对齐
// 参考Inter开发手册第三卷4.3中的Figure 4-4. Formats of CR3 and Paging-Structure Entries with 32-Bit Paging
uint32_t pg_dir[1024] __attribute__((aligned(4096))) = {
	[0] = (0) | PDE_P | PDE_W | PDE_U | PDE_PS,
};


uint32_t task0_dpl0_stack[1024], task0_dpl3_stack[1024];
uint32_t task1_dpl0_stack[1024], task1_dpl3_stack[1024];


uint32_t task0_tss[] = {
	0, (uint32_t)task0_dpl0_stack + 4 * 1024, KERNEL_DATA_SEG, 0x0, 0x0, 0x0, 0x0,

	(uint32_t)pg_dir, (uint32_t)task_0, 0x202, 0xa, 0xc, 0xd, 0xb, (uint32_t)task0_dpl3_stack + 4 * 1024, 0x1, 0x2, 0x3,

	APP_DATA_SEG, APP_CODE_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, 0x0, 0x0,

};

uint32_t task1_tss[] = {
	0, (uint32_t)task1_dpl0_stack + 4 * 1024, KERNEL_DATA_SEG, 0x0, 0x0, 0x0, 0x0,

	(uint32_t)pg_dir, (uint32_t)task_1, 0x202, 0xa, 0xc, 0xd, 0xb, (uint32_t)task1_dpl3_stack + 4 * 1024, 0x1, 0x2, 0x3,

	APP_DATA_SEG, APP_CODE_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, 0x0, 0x0,
};


//IDT表结构		参考Inter开发手册第三卷6.11中Figure 6-2. IDT Gate Descriptors（中断门）
struct {uint16_t offset_l, selector, attr, offset_h;} idt_table[256] __attribute__((aligned(8))); 		//无初始值

// GDT表结构	参考Inter开发手册第三卷3.4.5中Figure 3-8. Segment Descriptor，数据段的结构
struct{uint16_t limitl, basel, basemid_attr, limith_baseh;} gdt_table[256] __attribute__((aligned(8))) = {	// 8字节对齐
	[KERNEL_CODE_SEG / 8] = {0xffff, 0x0000, 0x9a00, 0x00cf},
	[KERNEL_DATA_SEG / 8] = {0xffff, 0x0000, 0x9200, 0x00cf},

	[APP_CODE_SEG / 8] = {0xffff, 0x0000, 0xfa00, 0x00cf},
	[APP_DATA_SEG / 8] = {0xffff, 0x0000, 0xf300, 0x00cf},

	[TASK0_TSS_SEL / 8] = {0x68, 0, 0xe900, 0}, 
	[TASK1_TSS_SEL / 8] = {0x68, 0, 0xe900, 0}, 

};

// 封装汇编指令，将一字节数据发送到指定端口
void outb (uint8_t data, uint16_t port){
	__asm__ __volatile__("outb %[v], %[p]" :: [v]"a"(data), [p]"d"(port));
}

void task_sched(){
	static int task_tss = TASK0_TSS_SEL;

	task_tss = (task_tss == TASK0_TSS_SEL) ? TASK1_TSS_SEL : TASK0_TSS_SEL;
	uint32_t addr[] = {0, task_tss};
	__asm__ __volatile__("ljmpl *(%[a])" :: [a]"r"(addr));
}

void timer_int ();
void os_init (void){
	//配置两片8259
	outb(0x11, 0x20);		//对8259主片初始化
	outb(0x11, 0xA0);		//对8259从片初始化
	outb(0x20, 0x21);		//指定主片从IDT中的0x20开始，这里的0x21指定是对主片操作
	outb(0x28, 0xA1);		//指定从片从IDT中的0x28开始，这里的0xA1指定是对从片操作
	outb(1 << 2, 0x21);		//告诉主片他的IRQ2上连接了一个从片
	outb(2, 0xA1);			//告诉从片他的IRQ2连接到了主片
	//设置8259的工作模式
	outb(0x1, 0x21);
	outb(0x1, 0xA1);
	//配置打开IQR0，即8253引脚的终端，屏蔽其他引脚的中断
	outb(0xfe, 0x21);
	outb(0xff, 0xA1);

	//配置8253定时器 实现每隔100ms产生一个中断
	//外部时钟源频率1193180Hz
	int tmo = 1193180 / 100;	//设置初始值
	outb(0x36, 0x43);			//配置使用8253定时器中第0个定时器，工作在模式3，周期性的加载初始值产生中断
	//写入初始值，高八位第八位分开写入
	outb((uint8_t)tmo, 0x40);	//低八位
	outb(tmo >> 8, 0x40);		//高八位


	//配置时钟中断处理函数偏移量
	idt_table[0x20].offset_l = (uint32_t)timer_int & 0xFFFF;		//低16位
	idt_table[0x20].offset_h = (uint32_t)timer_int >> 16;			//高16位
	idt_table[0x20].selector = KERNEL_CODE_SEG;						//选择子指向GDT表的代码段
	idt_table[0x20].attr = 0x8E00;									//根据用户手册配置属性，指定表项为Interrupt Gate
	

	gdt_table[TASK0_TSS_SEL / 8].basel = (uint16_t)(uint32_t)task0_tss;
	gdt_table[TASK1_TSS_SEL / 8].basel = (uint16_t)(uint32_t)task1_tss;


	pg_dir[MAP_ADDR >> 22] = (uint32_t)page_table | PDE_P | PDE_W | PDE_U;
	page_table[(MAP_ADDR >> 12) & 0x3FF] = (uint32_t)map_phy_buffer | PDE_P | PDE_W | PDE_U;
}