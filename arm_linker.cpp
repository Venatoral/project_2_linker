#include "./inc/arm_linker.hpp"
/**
 * 1. 起始地址 0x08040000  
 * 2.（内存）页对齐：4096字节
 * 3. .text代码段对齐：16字节，其他4字节
 * */

#define BASE_ADDR 0x08040000
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
    int off = rel_addr - baseAddr_;
    int block_num = blocks.size();
    int block_idx = 0;
    for(; block_idx < block_num; block_idx++) {
        if(off >= blocks[block_idx]->offset_ 
            && off < blocks[block_idx]->offset_ + blocks[block_idx]->size_)
            break;
    }

    unsigned char *pdata = (unsigned char *) ( blocks[block_idx]->data_ + (off - blocks[block_idx]->offset_) );
    if(type = 7) {  //R_386_JMP_SLOT
        

    }
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


// 符号解析，原地计算定义和未定义的符号虚拟地址 (解析先解析已定义符号，然后未定义符号)
void Linker::parseSym() {
    // 解析以定义符号
    printf("解析定义符号...\n");
    for(SymLink* s: sym_def) {
        Elf32_Sym* sym = s->prov_->sym_tbl_[s->sym_name_];
        string seg_name = s->prov_->shdr_names_[sym->st_shndx];
        sym->st_value += s->prov_->shdr_tbl_[seg_name]->sh_offset;
        printf("name: %s, addr: %8x\n", s->sym_name_.c_str(), sym->st_value);
    }
    printf("解析未定义UND符号...\n");
    for(SymLink* s: sym_ref) {
        Elf32_Sym* def_sym = s->prov_->sym_tbl_[s->sym_name_];
        Elf32_Sym* ref_sym = s->recv_->sym_tbl_[s->sym_name_];
        ref_sym->st_value = def_sym->st_value;
        printf("UND name: %s, addr: %8x\n", s->sym_name_, ref_sym->st_value);
    }
}


// 符号重定位，根据所有目标文件的重定位项修正符号地址
void Linker::relocate() {
    int elf_num = elfs.size();
    for(int i = 0; i < elf_num; i++) {
        ElfFile *elf = elfs[i];
        
        int rel_num = elf->rel_tbl_.size();
        for(int j = 0; j < rel_num; j++) {
            RelItem *rel = elf->rel_tbl_[j];

            Elf32_Shdr *seg = elf->shdr_tbl_[rel->seg_name_];
            unsigned int rel_addr = seg->sh_addr + rel->rel_->r_offset;

            Elf32_Sym *sym = elf->sym_tbl_[rel->rel_name_];
            unsigned int sym_addr = sym->st_value;

            unsigned char type = (unsigned char) rel->rel_->r_info;

            seg_lists_[rel->seg_name_]->relocAddr(rel_addr, type, sym_addr);
        }
    }
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