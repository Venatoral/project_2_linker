#ifndef INC_ARM_LINKER_HPP
#define INC_ARM_LINKER_HPP
#include <elf.h>
#include <map>
#include <vector>
#include <string>
using namespace std;

// 重定位信息结构
struct RelItem {
public:
    // 重定位段名称
    string seg_name_;
    // 重定位符号名称
    string rel_name_;
    Elf64_Rel* rel_;
};


// Elf文件类
class ElfFile {
private:
    Elf64_Ehdr ehdr_;
    // 程序头
    vector<Elf64_Phdr*> phdr_tbl_;
    // section头
    map<string, Elf64_Shdr*> shdr_tbl_;
    // section名顺序
    vector<string> shdr_names_;
    // 符号表
    map<string, Elf64_Sym*> sym_tbl_;
    // 符号名称
    vector<string> sym_names_;
    // 重定位
    vector<RelItem*> rel_tbl_;
    string shstrtab_;
    string strtab_;

public:
    // 构造函数，从file_dir_读取文件，构造ElfFile对象
    ElfFile(const string file_dir_);
    // 输出ElfFile信息到指定文件
    void writeFile();
};


// 数据块结构
struct Block {
public:
    // 字节数据
    char* data_;
    // 偏移量
    uint offset_;
    // 块大小
    uint size_;
};


// 段列表结构
class SegList {
public:
    // 基址
    uint baseAddr_;
    // 对齐前偏移
    uint begin_;
    // 对齐后偏移量
    uint offset_;
    // 大小
    uint size_;
    // 所有者文件
    vector<ElfFile*> owner_list_;
    // 数据块
    vector<Block*> blocks;


    // 分配地址
    void allocAddr(string name, uint& base, uint& off);
    // 重定位
    void relocAddr(uint rel_addr, unsigned char type, uint sym_addr);
};


// 符号信息
struct SymLink {
    // 符号名称
    string sym_name_;
    // 定义符号文件
    ElfFile* prov_;
    // 引用符号文件
    ElfFile* recv_;
};


#endif