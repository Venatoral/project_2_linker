#ifndef GEN_RELOC_HPP
#define GEN_RELOC_HPP

#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <map>
#include "./elf_struct.h"
#include "arm_analyze.hpp"

using namespace std;

class RelocatableFile
{
private:
    string file_name_;

public:
/*
    //读入
    vector<symbols *> symbol_list;            //函数和全局变量的符号 函数的偏移
    vector<reloc_symbol *> reloc_symbol_list; //重定位符号和重定位位置...
    vector<arm_assem *> arm_assem_list;       //储存分析好的汇编代码...一一对应...
    vector<data_element *> data_element_list; //data段变量名称和值
    vector<bss_element *> bss_element_list;   //bss段变量名称和值
*/
    //生成的结构
    vector<SectionInfo *> section_info_list; //节区列表
    vector<Elf32_Shdr *> shdr_list;          //节区头部列表
    Elf32_Ehdr elf_header;                   //elf文件头

    //生成的过程中需要用到的变量
    Elf32_Half cur_sec_no = 0; //记录现在是第几个节区 0号节区有留空规则 每生成一个节区记得自增
    Elf32_Word symtab_local_last_idx = 0;

    //生成.o文件使用的函数列表
public:
    // ml: 可以把 freeMalloc() 的工作放到析构函数？
    ~RelocatableFile()
    {
#define FREE_LIST(x)                   \
    for (int i = 0; i < x.size(); i++) \
    {                                  \
        free(x[i]);                    \
    }
        FREE_LIST(ARM_analyze::symbol_list)
        FREE_LIST(ARM_analyze::reloc_symbol_list)
        FREE_LIST(ARM_analyze::arm_assem_list)
        FREE_LIST(ARM_analyze::data_element_list)
        FREE_LIST(ARM_analyze::bss_element_list)
        FREE_LIST(section_info_list)
        FREE_LIST(shdr_list)
#undef FREE_LIST
    }
    // ml: 构造函数指定文件名称
    RelocatableFile(string file_name) {
        this->file_name_ = file_name;
        // genRelocFile();
    }

    void genSectionNote(); //生成一些无关紧要or内容固定的节区or第0号节区

    //tlx
    void genSectionBss();    //生成特殊节区-未初始化数据（初始化为0 不占用文件空间）--给这个函数写了个示意可以参考看看
    void genSectionData();   //生成特殊节区-已初始化数据
    void genSectionRodata(); //生成特殊节区-只读数据（如printf函数里面的格式控制字符串）

    //yrc
    void genSectionText(); //生成特殊节区-可执行代码

    //nzb
    void genSectionReloc(); //生成特殊节区-重定位表

    //zyj
    void genSectionSymtab(); //生成特殊节区-符号表
    void genSectionStrtab(); //生成特殊节区-符号名字表

    //lt
    void genSectionShstrtab(); //生成特殊节区-节区名字表
    void genShdrList();        //生成节区头部列表
    void genElfHeader();       //生成elf文件头

    //ml
    void genFile();    //输出到文件
   //freeMalloc(); //释放掉malloc的内存（在生成阶段，节区内容是malloc申请内存的，所以传出char *指针）

    //lt
    void genRelocFile();

private:
    //tool lt
    Elf32_Word getShType(string name);
    Elf32_Word getShFlags(string name);
    Elf32_Word getShLink(string name);
    Elf32_Word getShInfo(string name);
    Elf32_Word getShAddralign(string name);
    Elf32_Word getShEntsize(string name);

private:
    char *strtabContent = (char *)malloc(sizeof(char) * 65536);
    int strtabContentSize = 0;

    //tool yrc
    vector<char> arm_machine; //先把二进制码放在列表里，然后放入char *content;

    void trans_push(arm_assem *arm, int type);
    void trans_sub(arm_assem *arm, int pop);
    char get_reg(string s);
    void trans_jmp(arm_assem *arm, int type);
    void trans_nop(arm_assem *arm);
};
#endif
