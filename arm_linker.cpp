#include "./inc/arm_linker.hpp"
/**
 * 1.ARM Linux 64位起始虚拟地址：0xFFFFFE0000000000  
 * 2.（内存）页对齐：4096字节
 * 3. .text代码段对齐：16字节，其他4字节
 * */

#define BASE_ADDR 0xFFFFFE0000000000
#define TEXT_ALIGN 16
#define DESC_ALIGN 4
#define MEM_ALIGN 4096

// 合并段列表
map<string, SegList *> seg_list;
// 符号引用信息
vector<SymLink *> sym_ref;
// 符号定义信息
vector<SymLink *> sym_def;


/*
	name: 段名
	off: 文件偏移地址
	base: 加载基址，修改后提供给其他段
*/
void SegList::allocAddr(string name, unsigned int &base, unsigned int &off) {

}

/*
	rel_addr:重定位虚拟地址
	type:重定位类型
	sym_addr:重定位符号的虚拟地址
    Todo: 根据rel_addr寻找对应block的‘data’中的位置，并根据方式type（绝对，将对）和符号地址sym_addr来修改地址。
*/
void SegList::relocAddr(unsigned int rel_addr, unsigned char type, unsigned int sym_addr) {

}


/**
 * 添加一个目标文件
 * Todo: 构造ElfFile对象，添加到list中
 * */
void Linker::addElf(const char *dir) {
    ElfFile* elf = new ElfFile(dir);
    this->elfs.push_back(elf);
}


// 搜集段信息和符号关联信息
void Linker::collectInfo() {

}


// 分配地址空间，重新计算虚拟地址空间，磁盘空间连续分布不重新计算，其他的段全部删除
void Linker::allocAddr() {

}


// 符号解析，原地计算定义和未定义的符号虚拟地址
void Linker::parseSym() {

}


// 符号重定位，根据所有目标文件的重定位项修正符号地址
void Linker::relocate() {

}


// 组装可执行文件
void Linker::makeExec() {

}


// 输出elf
void Linker::writeExecFile(const char *dir) {

}


// 链接（其实就是顺序调用了上述过程)
// dir : 输出可执行文件的地址
bool Linker::link(const char *dir) {
    // 收集信息
    collectInfo();
    // 分配地址空间
    allocAddr();
    // 解析符号
    parseSym();
    // 重定位
    relocate();
    // 生成可执行文件
    makeExec();
    // 输出可执行文件到dir
    writeExecFile(dir);   
}