/**
 * 功能：公共头文件
 *
 */
#ifndef OS_H
#define OS_H

#define KERNEL_CODE_SEG     8
#define KERNEL_DATA_SEG     16

#define APP_CODE_SEG        ((3 * 8) | 3)       //或3，配置特权级3
#define APP_DATA_SEG        ((4 * 8) | 3)

#define TASK0_TSS_SEL       (5 * 8)
#define TASK1_TSS_SEL       (6 * 8)



#endif // OS_H
