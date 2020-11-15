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
    void readElf(const char *dir);                              //读入elf
    void getData(char *buf, Elf32_Off offset, Elf32_Word size); //读取数据
    int getSegIndex(string segName);                            //获取指定段名在段表下标
    int getSymIndex(string symName);                            //获取指定符号名在符号表下标
    void addPhdr(Elf32_Word type, Elf32_Off off, Elf32_Addr vaddr, Elf32_Word filesz,
                 Elf32_Word memsz, Elf32_Word flags, Elf32_Word align); //添加程序头表项
    void addShdr(string sh_name, Elf32_Word sh_type, Elf32_Word sh_flags, Elf32_Addr sh_addr, Elf32_Off sh_offset,
                 Elf32_Word sh_size, Elf32_Word sh_link, Elf32_Word sh_info, Elf32_Word sh_addralign,
                 Elf32_Word sh_entsize);      //添加一个段表项
    void addSym(string st_name, Elf32_Sym *); //添加一个符号表项
    void writeElf(const char *dir, int flag); //输出Elf文件
    ~ElfFile();
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

class Linker
{
    vector<string> segNames; //链接关心的段
    ElfFile exe;            //链接后的输出文件
    ElfFile *startOwner;    //拥有全局符号START/_start的文件
public:
    vector<ElfFile *> elfs;                           //所有目标文件对象
    map<string, SegList*> segLists; //所有合并段表序列
    vector<SymLink *> symLinks;                        //所有符号引用信息，符号解析前存储未定义的符号prov字段为NULL
    vector<SymLink *> symDef;                          //所有符号定义信息recv字段NULL时标示该符号没有被任何文件引用，否则指向本身（同prov）
public:
    Linker();
    void addElf(const char *dir);    //添加一个目标文件
    void collectInfo();              //搜集段信息和符号关联信息
    bool symValid();                 //符号关联验证，分析符号间的关联（定义和引用），全局符号位置，出现非法符号逻辑返回false
    void allocAddr();                //分配地址空间[重新计算虚拟地址空间，磁盘空间连续分布不重新计算]，其他的段全部删除
    void symParser();                //符号解析，原地计算定义和未定义的符号虚拟地址
    void relocate();                 //符号重定位，根据所有目标文件的重定位项修正符号地址
    void assemExe();                 //组装可执行文件
    void exportElf(const char *dir); //输出elf
    bool link(const char *dir);      //链接
    ~Linker();
};
#endif