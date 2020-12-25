#ifndef INC_ARM_LINKER_HPP
#define INC_ARM_LINKER_HPP
#include "elf_struct.h"
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
    Elf32_Rel *rel_;
public:
    RelItem(){};
    RelItem(string seg_name, string rel_name, Elf32_Rel* rel) {
        this->seg_name_ = seg_name;
        this->rel_name_ = rel_name;
        this->rel_ = rel;
    }
};

// 数据块结构
struct Block {
public:
    // 字节数据
    char *data_;
    // 偏移量
    unsigned int offset_;
    // 块大小
    unsigned int size_;
};
/**Tasks：
* 1. SegList::allocAddr + Linker::allocAddr    分配地址 yrc
* 2. SegList::relocAddr + Linker::relocate     重定位 tlx
* 3. Linker::collectInfo                       收集所有ElfFile类的信息 lt
* 4. Linker::parseSym                          符号解析（修改所有符号地址） male
* 5. Linker::makeExec                          组装可执行文件EilfFile对象 zyj
* 6. Linker::writeExec + ElfFile               构造函数ElfFile读取Elf文件 + 写可执行文件 nzb
**/

// Elf文件类
class ElfFile {
public:
    // 文件名称
    string elf_dir_;
    Elf32_Ehdr ehdr_;
    // 程序头
    vector<Elf32_Phdr *> phdr_tbl_;
    // section头
    map<string, Elf32_Shdr *> shdr_tbl_;
    // section名顺序
    vector<string> shdr_names_;
    // 符号表
    map<string, Elf32_Sym *> sym_tbl_;
    // 符号名称
    vector<string> sym_names_;
    // 重定位
    vector<RelItem *> rel_tbl_;
    char* shstrtab_;
    char* strtab_;
public:
    ElfFile(){};
    ElfFile(const char *file_dir_);
    void getData(char *buf, Elf32_Off offset, Elf32_Word size); //读取数据
    int getSegIndex(string seg_name);                           //获取指定段名在段表下标
    int getSymIndex(string sym_name);                           //获取指定符号名在符号表下标
    void addPhdr(Elf32_Phdr *new_phdr);                         //添加程序头表项
    void addShdr(string shdr_name, Elf32_Shdr *new_shdr);       //添加一个段表项
    void addSym(string st_name, Elf32_Sym *);                   //添加一个符号表项
    ~ElfFile();
};


// 段列表结构
class SegList {
public:
    // 基址
    unsigned int baseAddr_;
    // 对齐前偏移
    unsigned int begin_;
    // 对齐后偏移量
    unsigned int offset_;
    // 大小
    unsigned int size_;
    // 拥有该段的文件序列
    vector<ElfFile *> owner_list_;
    // 数据块
    vector<Block *> blocks;

    // 分配地址
    void allocAddr(string name, unsigned int &base, unsigned int &off);
    // 重定位
    void relocAddr(unsigned int rel_addr, unsigned char type, unsigned int sym_addr);
};

// 符号信息
struct SymLink {
    // 符号名称
    string sym_name_;
    // 定义符号文件
    ElfFile *prov_;
    // 引用符号文件
    ElfFile *recv_;
};

class Linker {
    vector<string> seg_names_; //链接关心的段(.text, ,data, .bss)
    ElfFile elf_exe_;          //链接后的输出文件
    ElfFile *start_owner_;     //拥有全局符号START/_start的文件

    // 符号引用信息
    vector<SymLink *> sym_ref;
    // 符号定义信息
    vector<SymLink *> sym_def;

public:
    vector<ElfFile *> elfs;            //所有目标文件对象
    map<string, SegList *> seg_lists_; //所有合并段表序列
    vector<SymLink *> sym_links_;      //所有符号引用信息，符号解析前存储未定义的符号prov字段为NULL
    vector<SymLink *> sym_def_;        //所有符号定义信息recv字段NULL时标示该符号没有被任何文件引用，否则指向本身（同prov）
public:
    Linker();
    void addElf(const char *dir);        //添加一个目标文件
    void collectInfo();                  //搜集段信息和符号关联信息
    void allocAddr();                    //分配地址空间[重新计算虚拟地址空间，磁盘空间连续分布不重新计算]，其他的段全部删除
    void parseSym();                     //符号解析，原地计算定义和未定义的符号虚拟地址
    void relocate();                     //符号重定位，根据所有目标文件的重定位项修正符号地址
    void makeExec();                     //组装可执行文件
    void writeExecFile(const char *dir); //输出elf
    void link(const char *dir);          //链接
    ~Linker();
};
#endif
