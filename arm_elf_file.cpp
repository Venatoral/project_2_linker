#include "inc/arm_linker.hpp"
// 输出ElfFile信息到指定文件

// 构造函数，从file_dir_读取文件，构造ElfFile对象
ElfFile::ElfFile(const char* file_dir_) {
    this->elf_dir_ = string(file_dir_);
}

// 析构函数，释放内存空间
ElfFile::~ElfFile() {

}

// 从对应文件 offset位置开始，读取size字节到buf中
void ElfFile::getData(char *buf, Elf32_Off offset, Elf32_Word size) {
    FILE* fp = fopen(this->elf_dir_.c_str(), "rb");
    if(!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fseek(fp, offset, SEEK_SET);
    fread(buf, size, 1, fp);
    fclose(fp);
}

// 获取指定段名在段表下标
int ElfFile::getSegIndex(string seg_name) {
    int i = 0;
    while(i < shdr_names_.size()) {
        if(shdr_names_[i] == seg_name) {
            return i;
        }
        i ++;
    }
    // 没找到对应段
    return -1;
}

// 获取指定符号名在符号表下标
int ElfFile::getSymIndex(string sym_name) {
    int i = 0;
    while(i < sym_names_.size()) {
        if(sym_names_[i] == sym_name) {
            return i;
        }
        i ++;
    }
    // 没找到符号
    return -1;
}

// 添加程序头表项
void ElfFile::addPhdr(Elf32_Phdr *new_phdr) {
    this->phdr_tbl_.push_back(new_phdr);
}

// 添加一个段表项
void ElfFile::addShdr(string shdr_name, Elf32_Shdr *new_shdr) {
    this->shdr_names_.push_back(shdr_name);
    this->shdr_tbl_[shdr_name] = new_shdr;
}

// 添加一个符号表项
void ElfFile::addSym(string st_name, Elf32_Sym *new_sym) {
    this->sym_names_.push_back(st_name);
    this->sym_tbl_[st_name] = new_sym;
}

