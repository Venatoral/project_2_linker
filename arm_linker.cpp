#include "./inc/arm_linker.hpp"
// 合并段列表
map<string, SegList*> seg_lists;
// 符号引用信息
vector<SymLink*> sym_ref;
// 符号定义信息
vector<SymLink*> sym_def;

/**
 * 1.ARM Linux 64位起始虚拟地址：0xFFFFFE0000000000  
 * 2.（内存）页对齐：4096字节
 * 3. .text代码段对齐：16字节，其他4字节
 * */

#define BASE_ADDR   0xFFFFFE0000000000
#define TEXT_ALIGN  16
#define DESC_ALIGN  4
#define MEM_ALIGN   4096