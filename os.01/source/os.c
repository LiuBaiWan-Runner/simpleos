/**
 * 功能：32位代码，完成多任务的运行
 *
 */
#include "os.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// GDT表结构
struct
{uint16_t limitl, basel, basemid_attr, limith_baseh;	//	参考Inter开发手册第三卷3.4.5中Figure 3-8. Segment Descriptor，数据段的结构
} gdt_table[256] __attribute__((aligned(8))) = {	// 8字节对齐
	[KERNEL_CODE_SEG / 8] = {0xffff, 0x0000, 0x9a00, 0x00cf},
	[KERNEL_DATA_SEG / 8] = {0xffff, 0x0000, 0x9200, 0x00cf},
};