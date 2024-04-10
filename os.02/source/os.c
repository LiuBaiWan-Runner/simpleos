/**
 * 功能：32位代码，完成多任务的运行
 *
 */
#include "os.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define PDE_P		(1 << 0)
#define PDE_W		(1 << 1)
#define PDE_U		(1 << 2)
#define PDE_PS		(1 << 7)

#define MAP_ADDR	0x80000000

uint8_t map_phy_buffer[4096] __attribute__((aligned(4096))) = {0x36}; //定义4K实际物理内存空间

static uint32_t page_table[1024] __attribute__((aligned(4096))) = {PDE_U};  //定义一个4K页表

// 32位页目录表结构第一项，4K对齐
// 参考Inter开发手册第三卷4.3中的Figure 4-4. Formats of CR3 and Paging-Structure Entries with 32-Bit Paging
uint32_t pg_dir[1024] __attribute__((aligned(4096))) = {
	[0] = (0) | PDE_P | PDE_W | PDE_U | PDE_PS,
};

// GDT表结构
struct{					//	参考Inter开发手册第三卷3.4.5中Figure 3-8. Segment Descriptor，数据段的结构
uint16_t limitl, basel, basemid_attr, limith_baseh;	} gdt_table[256] __attribute__((aligned(8))) = {	// 8字节对齐
	[KERNEL_CODE_SEG / 8] = {0xffff, 0x0000, 0x9a00, 0x00cf},
	[KERNEL_DATA_SEG / 8] = {0xffff, 0x0000, 0x9200, 0x00cf},
};

void os_init (void){
	pg_dir[MAP_ADDR >> 22] = (uint32_t)page_table | PDE_P | PDE_W | PDE_U;
	page_table[(MAP_ADDR >> 12) & 0x3FF] = (uint32_t)map_phy_buffer | PDE_P | PDE_W | PDE_U;
}