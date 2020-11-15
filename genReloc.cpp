#include <cstdio>
#include <vector>
#include <string>
#include <cstdlib>
#include "elfstruct.h"
using namespace std;

class RelocatableFile{
    //读入
    vector<symbols*> symbol_list;//函数和全局变量的符号 函数的偏移
    vector<reloc_symbol*> reloc_symbol_list;//重定位符号和重定位位置...
    vector<arm_assem*> arm_assem_list;//储存分析好的汇编代码...一一对应...
    vector<data_element*> data_element_list;//data段变量名称和值
    vector<bss_element*> bss_element_list;//bss段变量名称和值

    //生成的结构
    vector<SectionInfo> section_info_list;//节区列表
    vector<Elf32_Shdr> shdr_list;//节区头部列表
    Elf32_Ehdr elf_header;//elf文件头

    //生成的过程中需要用到的变量
    Elf32_Half cur_sec_no=0;//记录现在是第几个节区 0号节区有留空规则 每生成一个节区记得自增

    //生成.o文件使用的函数列表
    void genSectionNote();//生成一些无关紧要or内容固定的节区or第0号节区

    void genSectionBss();//生成特殊节区-未初始化数据（初始化为0 不占用文件空间）--给这个函数写了个示意可以参考看看
    void genSectionData();//生成特殊节区-已初始化数据
    void genSectionRodata();//生成特殊节区-只读数据（如printf函数里面的格式控制字符串）

    void genSectionText();//生成特殊节区-可执行代码
    void genSectionReloc();//生成特殊节区-重定位表

    void genSectionSymtab();//生成特殊节区-符号表
    void genSectionStrtab();//生成特殊节区-符号名字表

    void genSectionShstrtab();//生成特殊节区-节区名字表

    void genShdrList();//生成节区头部列表
    void genElfHeader();//生成elf文件头

    void genFile();//输出到文件

    void freeMalloc();//释放掉malloc的内存（在生成阶段，节区内容是malloc申请内存的，所以传出char *指针）
};

void RelocatableFile::genSectionNote(){
    //生成一些无关紧要or内容固定的节区
    //input: 参考gcc输出和文档要求
    //output: section_info_list.push_back(xxx);
}

void RelocatableFile::genSectionBss(){
    //生成特殊节区-未初始化数据（初始化为0 不占用文件空间）
    //input: bss_element_list
    //output: section_info_list.push_back(xxx);

    //TODO elfstruct.h line:51 根据bss节的文档 设计数据结构[X]把这个节区的内容组织起来
    
    SectionBss x;
    Elf32_Word sz=0;//x的大小

    SectionInfo bss;
    bss.no=cur_sec_no++;
    bss.name=".bss";

    {
        //TODO 根据bss_element_list生成节区内容存到[X]里
        //TODO 存内容的时候维护sz
        bss.size=(Elf32_Word)sz;
        bss.content=(char *)malloc((size_t)bss.size);
        //TODO 把数据结构的内容写到连续内存空间里
    }
}

void RelocatableFile::genSectionData(){
    //生成特殊节区-已初始化数据
    //input: data_element_list
    //output: section_info_list.push_back(xxx);
}

void RelocatableFile::genSectionRodata(){
    //生成特殊节区-只读数据（如printf函数里面的格式控制字符串）
    //input: data_element_list
    //output: section_info_list.push_back(xxx);
}


void RelocatableFile::genSectionText(){
    //生成特殊节区-可执行代码
    //input: arm_assem_list
    //output: section_info_list.push_back(xxx);
}

void RelocatableFile::genSectionReloc(){
    //生成特殊节区-重定位表
    //input: reloc_symbol_list
    //output: section_info_list.push_back(xxx);
}


void RelocatableFile::genSectionSymtab(){
    //生成特殊节区-符号表
    //input: symbol_list
    //output: section_info_list.push_back(xxx);
}

void RelocatableFile::genSectionStrtab(){
    //生成特殊节区-符号名字表
    //input: section_info_list[x].name==".symtab"
    //output: section_info_list.push_back(xxx);
}


void RelocatableFile::genSectionShstrtab(){
    //生成特殊节区-节区名字表
    //input: section_info_list,this
    //output: section_info_list.push_back(xxx);
}


void RelocatableFile::genShdrList(){
    //生成节区头部列表
    //input: section_info_list
    //output: shdr_list
}

void RelocatableFile::genElfHeader(){
    //生成elf文件头
    //input: section_info_list,shdr_list
    //output: elf_header
}


void RelocatableFile::genFile(){
    //输出到文件
    //input: section_info_list,shdr_list,elf_header
    //output: FILE*
}
