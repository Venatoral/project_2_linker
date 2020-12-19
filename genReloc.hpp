#ifndef GEN_RELOC_HPP
#define GEN_RELOC_HPP

#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <map>

#include "elfStruct.h"

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
    RelocatableFile(string file_name)
    {
        this->file_name_ = file_name;
        genRelocFile();
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

//lt call all generating relocateable file's function
void RelocatableFile::genRelocFile(){
    genSectionNote(); //生成一些无关紧要or内容固定的节区or第0号节区
    cout<<"note"<<endl;
    //tlx
    genSectionBss();    //生成特殊节区-未初始化数据（初始化为0 不占用文件空间）--给这个函数写了个示意可以参考看看
    genSectionData();   //生成特殊节区-已初始化数据
    genSectionRodata(); //生成特殊节区-只读数据（如printf函数里面的格式控制字符串）
    cout<<"data"<<endl;
    //yrc
    genSectionText(); //生成特殊节区-可执行代码
    cout<<"text"<<endl;
    //nzb
    genSectionReloc(); //生成特殊节区-重定位表
    cout<<"reloc"<<endl;
    //zyj
    genSectionSymtab(); //生成特殊节区-符号表
    genSectionStrtab(); //生成特殊节区-符号名字表
    cout<<"sym"<<endl;
    //lt
    genSectionShstrtab(); //生成特殊节区-节区名字表
    cout<<"1"<<endl;
    genShdrList();        //生成节区头部列表
    cout<<"2"<<endl;
    genElfHeader();       //生成elf文件头
    cout<<"sh"<<endl;
    //ml
    genFile();    //输出到文件
    cout<<"file"<<endl;
    //freeMalloc(); //释放掉malloc的内存（在生成阶段，节区内容是malloc申请内存的，所以传出char *指针）

}

void RelocatableFile::genSectionNote()
{
    //生成一些无关紧要or内容固定的节区
    //input: 参考gcc输出和文档要求
    //output: section_info_list.push_back(xxx);

    SectionInfo *empty_section = new SectionInfo();
    empty_section->no = cur_sec_no++;
    empty_section->name = "";
    empty_section->size = 0;
    empty_section->content = NULL;
    section_info_list.push_back(empty_section);
}

void RelocatableFile::genSectionBss()
{
    //生成特殊节区-未初始化数据（初始化为0 不占用文件空间）
    //input: bss_element_list
    //output: section_info_list.push_back(xxx);

    //TODO elfstruct.h line:51 根据bss节的文档 设计数据结构[X]把这个节区的内容组织起来

    SectionBss x;
    Elf32_Word sz = 0; //x的大小

    SectionInfo *bss = new SectionInfo();
    bss->no = cur_sec_no++;
    bss->name = ".bss";

    {
        //TODO 根据bss_element_list生成节区内容存到[X]里
        //TODO 存内容的时候维护sz
        bss->size = (Elf32_Word)sz;
        bss->content = (char *)malloc((size_t)bss->size);
        //TODO 把数据结构的内容写到连续内存空间里
    }
    section_info_list.push_back(bss);
}

void RelocatableFile::genSectionData()
{
    //生成特殊节区-已初始化数据
    //input: data_element_list
    //output: section_info_list.push_back(xxx);

    SectionInfo *data = new SectionInfo();
    data->no = cur_sec_no++;
    data->name = ".data";
    data->size = 0;
    data->content = NULL;
    section_info_list.push_back(data);

    for (int i = 0; i < ARM_analyze::data_element_list.size(); i++)
    {
        if (ARM_analyze::data_element_list[i]->op_name == "word")
            data->size += 4;
        else if (ARM_analyze::data_element_list[i]->op_name == "space")
            data->size += ARM_analyze::data_element_list[i]->value;
    }

    data->content = (char *)malloc(data->size);

    int cur = 0;
    for (int i = 0; i < ARM_analyze::data_element_list.size(); i++)
    {
        if (ARM_analyze::data_element_list[i]->op_name == "word")
        {
            *(data->content + cur) = ARM_analyze::data_element_list[i]->value;
            cur += 4;
        }
        else if (ARM_analyze::data_element_list[i]->op_name == "space")
        {
            memset(data->content + cur, 0,ARM_analyze::data_element_list[i]->value);
            cur += ARM_analyze::data_element_list[i]->value;
        }
    }
}

void RelocatableFile::genSectionRodata()
{
    //生成特殊节区-只读数据（如printf函数里面的格式控制字符串）
    //input: data_element_list
    //output: section_info_list.push_back(xxx);

    // 我们没有字符串，略
}

#define _POP_ 0
#define _PUSH_ 1
#define _SUB_ 0
#define _ADD_ 1
#define _MUL_ 2
#define _MOV_ 3
#define _CMP_ 4
#define _STR_ 5
#define _LDR_ 6
void RelocatableFile::genSectionText()
{
    //生成特殊节区-可执行代码
    //input: arm_assem_list
    //output: section_info_list.push_back(xxx);
    SectionInfo *text = new SectionInfo();
    text->name = ".text";
    text->no = cur_sec_no;
    cur_sec_no++;

    for (int i = 0; i < ARM_analyze::arm_assem_list.size(); i++)
    {
        if (ARM_analyze::arm_assem_list[i]->op_name.compare("push") == 0)
        {
            trans_push(ARM_analyze::arm_assem_list[i], _PUSH_);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("pop") == 0)
        {
            trans_push(ARM_analyze::arm_assem_list[i], _POP_);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("sub") == 0)
        {
            trans_sub(ARM_analyze::arm_assem_list[i], _SUB_);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("add") == 0)
        {
            trans_sub(ARM_analyze::arm_assem_list[i], _ADD_);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("mul") == 0)
        {
            trans_sub(ARM_analyze::arm_assem_list[i], _MUL_);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("mov") == 0)
        {
            trans_sub(ARM_analyze::arm_assem_list[i], _MOV_);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("str") == 0)
        {
            trans_sub(ARM_analyze::arm_assem_list[i], _STR_);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("ldr") == 0)
        {
            trans_sub(ARM_analyze::arm_assem_list[i], _LDR_);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("cmp") == 0)
        {
            trans_sub(ARM_analyze::arm_assem_list[i], _CMP_);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("b") == 0)
        {
            trans_jmp(ARM_analyze::arm_assem_list[i], 0);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("bl") == 0)
        {
            trans_jmp(ARM_analyze::arm_assem_list[i], 1);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("beq") == 0)
        {
            trans_jmp(ARM_analyze::arm_assem_list[i], 2);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("bne") == 0)
        {
            trans_jmp(ARM_analyze::arm_assem_list[i], 3);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("ble") == 0)
        {
            trans_jmp(ARM_analyze::arm_assem_list[i], 4);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("bge") == 0)
        {
            trans_jmp(ARM_analyze::arm_assem_list[i], 5);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("bgt") == 0)
        {
            trans_jmp(ARM_analyze::arm_assem_list[i], 6);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("blt") == 0)
        {
            trans_jmp(ARM_analyze::arm_assem_list[i], 7);
        }
        else if (ARM_analyze::arm_assem_list[i]->op_name.compare("nop") == 0)
        {
            trans_nop(ARM_analyze::arm_assem_list[i]);
        }
    }

    char byte;
    char bytes[4] = {'\0'};
    text->content = (char *)malloc(arm_machine.size());
    int offset = 0;
    int i = 0;
    for (auto content : arm_machine)
    {
        // printf("%hhx", content);
        byte = content;
        bytes[3 - i] = byte;
        i++;
        if (i == 4)
        {
            //printf("\n");
            i = 0;
            memcpy(text->content + offset, bytes, sizeof(bytes));
            offset += sizeof(bytes);
        }
    }
    text->size=arm_machine.size();
    section_info_list.push_back(text);
}

//tool functions(yrc)
/**
 * r0~r7返回对应数值
 * sp: 0x0d
 * fp: 0x0b
**/
char RelocatableFile::get_reg(string s)
{
    if (s.find("r7") != s.npos)
        return 0x07;
    if (s.find("r6") != s.npos)
        return 0x06;
    if (s.find("r5") != s.npos)
        return 0x05;
    if (s.find("r4") != s.npos)
        return 0x04;
    if (s.find("r3") != s.npos)
        return 0x03;
    if (s.find("r2") != s.npos)
        return 0x02;
    if (s.find("r1") != s.npos)
        return 0x01;
    if (s.find("r0") != s.npos)
        return 0x00;
    if (s.find("sp") != s.npos)
        return 0x0d;
    if (s.find("fp") != s.npos)
        return 0x0b;
    if (s.find("pc") != s.npos)
        return 0x0f;

    if (s.find("#") != s.npos)
    {
        int index = s.find("#");
        string num = s.substr(index + 1);
        int intm = atoi(num.c_str());
        char byte = intm;
        return byte;
    }

    return 0xFF;
}

//push and pop
void RelocatableFile::trans_push(arm_assem *_arm_assem, int type)
{
    char high = 0x00, low = 0x00;
    switch (type)
    {
    case _PUSH_:
        arm_machine.push_back(0xe9);
        arm_machine.push_back(0x2d);
        break;

    case _POP_:
        arm_machine.push_back(0xe8);
        arm_machine.push_back(0xbd);
        break;

    default:
        break;
    }

    string reg_list;
    for (int i = 0; i < _arm_assem->reglist.size(); i++)
    {
        reg_list += _arm_assem->reglist[i];
    }

    if (reg_list.find("lr") != reg_list.npos)
        high = high | 0b01000000;

    if (reg_list.find("fp") != reg_list.npos)
        high = high | 0b00001000;

    if (reg_list.find("pc") != reg_list.npos)
        high = high | 0b10000000;

    if (reg_list.find("r0") != reg_list.npos)
        low = low | 0b00000001;
    if (reg_list.find("r1") != reg_list.npos)
        low = low | 0b00000010;
    if (reg_list.find("r2") != reg_list.npos)
        low = low | 0b00000100;
    if (reg_list.find("r3") != reg_list.npos)
        low = low | 0b00001000;
    if (reg_list.find("r4") != reg_list.npos)
        low = low | 0b00010000;
    if (reg_list.find("r5") != reg_list.npos)
        low = low | 0b00100000;
    if (reg_list.find("r6") != reg_list.npos)
        low = low | 0b01000000;
    if (reg_list.find("r7") != reg_list.npos)
        low = low | 0b10000000;

    arm_machine.push_back(high);
    arm_machine.push_back(low);
}

void RelocatableFile::trans_nop(arm_assem *_arm_assem)
{
    arm_machine.push_back(0x00);
    arm_machine.push_back(0x00);
    arm_machine.push_back(0x00);
    arm_machine.push_back(0x00);
}

//sub add mul mov cmp str ldr
void RelocatableFile::trans_sub(arm_assem *_arm_assem, int type)
{
    char fisrt_byte = 0x00;
    char second_byte = 0x00;
    char third_byte = 0x00;
    char forth_byte = 0x00;
    char R_second = 0x00;
    char R_first = 0x00;
    char has_Immediate = 0x00;

    string F_R = _arm_assem->Operands1;
    string S_R = _arm_assem->Operands2;
    string last = _arm_assem->Operands3;

    R_first = get_reg(F_R);
    R_second = get_reg(S_R);

    if ((last.find("#") != last.npos) || (S_R.find("#") != S_R.npos))
        has_Immediate = 0x02;

    switch (type)
    {
    case _SUB_:
        fisrt_byte = 0xe0;
        second_byte = 0x40;

        fisrt_byte += has_Immediate;
        second_byte += R_second;
        third_byte = R_first * 16;
        forth_byte = get_reg(last);
        break;

    case _ADD_:
        fisrt_byte = 0xe0;
        second_byte = 0x80;

        fisrt_byte += has_Immediate;
        second_byte += R_second;
        third_byte = R_first * 16;
        forth_byte = get_reg(last);
        break;

    case _MUL_:
        fisrt_byte = 0xe0;
        forth_byte = 0x90;

        second_byte += R_first;
        third_byte = get_reg(last);
        forth_byte += R_second;
        break;

    case _MOV_:
        fisrt_byte = 0xe1;
        second_byte = 0xa0;

        fisrt_byte += has_Immediate;
        third_byte = R_first * 16;
        forth_byte = get_reg(S_R);
        break;

    case _CMP_:
        fisrt_byte = 0xe1;
        second_byte = 0x50;

        R_first = get_reg(F_R);
        second_byte += R_first;
        forth_byte = get_reg(S_R);
        break;

    case _STR_:
        fisrt_byte = 0xe7;
        second_byte = 0x80;

        fisrt_byte -= has_Immediate;

        if (last.find("#-") != last.npos)
        { //str和ldr的后12位为无符号偏移量
            last = '#' + last.substr(2);
            second_byte -= 0x80;
        }

        second_byte += R_second;
        third_byte += R_first * 16;
        forth_byte = get_reg(last);
        break;

    case _LDR_:
        fisrt_byte = 0xe7;
        second_byte = 0x90;

        fisrt_byte -= has_Immediate;

        if (last.find("#-") != last.npos)
        { //str和ldr的后12位为无符号偏移量
            last = '#' + last.substr(2);
            second_byte -= 0x80;
        }

        second_byte += R_second;
        third_byte += R_first * 16;

        forth_byte = get_reg(last);
        break;

    default:
        break;
    }

    arm_machine.push_back(fisrt_byte);
    arm_machine.push_back(second_byte);
    arm_machine.push_back(third_byte);
    arm_machine.push_back(forth_byte);
}

//jmp指令后24位有符号数计算方法，偏移量=signd_immdeiate<<2 +8
//type标识跳转的指令0：B   1：BL   2:BEQ   3:BNE   4:BLE   5:BGE   6:BGT   7:BLT
void RelocatableFile::trans_jmp(arm_assem *_arm_assem, int type)
{
    string label_name;
    signed int signd_immediate;
    int offset;
    char fisrt_byte = 0x00;
    char second_byte = 0x00;
    char third_byte = 0x00;
    char fourth_byte = 0x00;
    switch (type)
    {
    case 0:
        fisrt_byte = 0xea;
        break;
    case 1:
        fisrt_byte = 0xeb;
        break;
    case 2:
        fisrt_byte = 0x0a;
        break;
    case 3:
        fisrt_byte = 0x1a;
        break;
    case 4:
        fisrt_byte = 0xda;
        break;
    case 5:
        fisrt_byte = 0xaa;
        break;
    case 6:
        fisrt_byte = 0xca;
        break;
    case 7:
        fisrt_byte = 0xba;
        break;
    default:
        break;
    }
    string operands = _arm_assem->Operands1;

    offset = atoi(operands.substr(1).c_str());
    signd_immediate = (offset - 8) >> 2;
    fourth_byte = signd_immediate % (16 * 16 * 16 * 16);
    signd_immediate >>= 8;
    third_byte = signd_immediate % (16 * 16);
    signd_immediate >>= 8;
    second_byte = signd_immediate;

    arm_machine.push_back(fisrt_byte);
    arm_machine.push_back(second_byte);
    arm_machine.push_back(third_byte);
    arm_machine.push_back(fourth_byte);
}

#undef _POP_
#undef _PUSH_
#undef _SUB_
#undef _ADD_
#undef _MUL_
#undef _MOV_
#undef _CMP_
#undef _STR_
#undef _LDR_

void RelocatableFile::genSectionReloc()
{
    //生成特殊节区-重定位表
    //input: reloc_symbol_list
    //output: section_info_list.push_back(xxx);
    SectionInfo *rel = new SectionInfo();
    rel->no = cur_sec_no;
    cur_sec_no++;
    rel->name = ".rel";

    //计算节区大小
    rel->size = ARM_analyze::reloc_symbol_list.size() * sizeof(Elf32_Rel);
    rel->content = (char *)malloc(rel->size);
    //填入节区内容
    for (int i = 0, off = 0; i < ARM_analyze::reloc_symbol_list.size(); i++)
    {

        Elf32_Rel *r = new Elf32_Rel();
        int j = 0;
        for (; j < ARM_analyze::symbol_list.size(); j++)
        { //找到符号表索引，为确定r_info作准备。
            if (ARM_analyze::reloc_symbol_list[i]->name == ARM_analyze::symbol_list[j]->name)
                break;
        }
        r->r_info = j << 8; //ELF32_R_SYM(i)的逆过程
        r->r_offset = ARM_analyze::reloc_symbol_list[i]->value;

        if (ARM_analyze::reloc_symbol_list[i]->type == 0)
        {                              //0是函数，类型应该是R_386_JMP_SLOT    7
            r->r_info = r->r_info + 7; //ELF32_R_TYPE(i)的逆过程
        };
        if (ARM_analyze::reloc_symbol_list[i]->type == 1)
        {                              //1是全局变量，类型应该是R_386_GLOB_DAT    6
            r->r_info = r->r_info + 6; //ELF32_R_TYPE(i)的逆过程
        };

        memcpy(rel->content + off, r, sizeof(Elf32_Rel));
        off += sizeof(Elf32_Rel);
    }
    //存到列表里
    section_info_list.push_back(rel);
}

#define NOTYPE -1
#define FUNCTION 0
#define GLOBAL_VAR 1
#define LABEL 2
#define LOCAL 1
#define GLOBAL 0
void RelocatableFile::genSectionSymtab()
{
    //生成特殊节区-符号表
    //input: symbol_list
    //output: section_info_list.push_back(xxx);
    //另外把这个值填一下：symtab_local_last_idx=符号表中最后一个bind==local的符号的序号（从0开始编号）
    SectionInfo *symtab = new SectionInfo();
    symtab->no = cur_sec_no++;
    symtab->name = ".symtab";
    vector<Elf32_Sym> *content = new vector<Elf32_Sym>();
    Elf32_Sym temp;
    int offset = 0;
    int curIndex = -1;
    for (auto symbol : ARM_analyze::symbol_list)
    {
        ++curIndex;
        temp.st_info = symbol->type;
        if (symbol->name.length() != 0)
        {
            temp.st_name = offset;
            for (auto ch : symbol->name)
                strtabContent[offset++] = ch;
            strtabContent[offset++] = '\0';
        }
        temp.st_value = symbol->value;
        temp.st_size = symbol->name.length();
        temp.st_other = 0;
        //Question: st_shndx如何从symbol_list获取？
        content->emplace_back(temp); //push_back是使用值传递，不需要重新声明temp
        if (symbol->bind == LOCAL)
            symtab_local_last_idx = curIndex;
    }
    strtabContentSize = offset;
    symtab->size = (Elf32_Word)(content->size() * sizeof(Elf32_Word));
    symtab->content = (char *)content;
    section_info_list.emplace_back(symtab);
}
#define LOCAL 1
#define GLOBAL 0
#undef NOTYPE
#undef FUNCTION
#undef GLOBAL_VAR
#undef LABEL

void RelocatableFile::genSectionStrtab()
{
    //生成特殊节区-符号名字表
    //input: section_info_list[x].name==".symtab"
    //output: section_info_list.push_back(xxx);
    SectionInfo *strtab = new SectionInfo();
    strtab->no = cur_sec_no++;
    strtab->name = ".strtab";
    strtab->content = strtabContent;
    strtab->size = strtabContentSize;
    section_info_list.push_back(strtab);

}

//lt
void RelocatableFile::genSectionShstrtab()
{
    //生成特殊节区-节区名字表
    //input: section_info_list,this
    //output: section_info_list.push_back(xxx);

    SectionInfo *shstrtab = new SectionInfo();
    shstrtab->no = cur_sec_no++;
    shstrtab->name = ".shstrtab";
    shstrtab->size = 0;
    section_info_list.push_back(shstrtab);
    //计算节区大小
    for (int i = 0; i < section_info_list.size(); i++)
    {
        shstrtab->size += section_info_list[i]->name.length() + 1;
       // cout<<section_info_list[i]->name<<endl;
        //cout<<section_info_list[i]->name.length()<<endl;
        //cout<<shstrtab->size<<endl;
        //+1为了留出'\0'的位置
    }
    shstrtab->content = (char *)malloc(shstrtab->size);
    //cout<<"malloc"<<endl;
    //填入节区内容
    memset(shstrtab->content, 0, (shstrtab->size) * sizeof(char));
    //cout<<"memset"<<endl;
    for (int i = 0, off = 0; i < section_info_list.size(); i++)
    {
        strncpy(shstrtab->content + off, section_info_list[i]->name.c_str(), section_info_list[i]->name.length());
        off += section_info_list[i]->name.length() + 1;
        //+1可以让那个位置保留初始的'\0'（前面memset了）
    }
    //之前已经push_back了
}

//lt
void RelocatableFile::genShdrList()
{
    //生成节区头部列表
    //input: section_info_list
    //output: shdr_list

    Elf32_Off cur_sec_off = sizeof(Elf32_Shdr) * (cur_sec_no - 1) + sizeof(Elf32_Ehdr);

    for (int i = 0; i < cur_sec_no; i++)
    {
        Elf32_Shdr *newShdr = new Elf32_Shdr();
        newShdr->sh_name = section_info_list[i]->no;
        newShdr->sh_type = getShType(section_info_list[i]->name);
        newShdr->sh_flags = getShFlags(section_info_list[i]->name);
        newShdr->sh_addr = 0; //现在还没有

        newShdr->sh_offset = cur_sec_off;
        cur_sec_off += section_info_list[i]->size;

        newShdr->sh_size = section_info_list[i]->size;
        newShdr->sh_link = getShLink(section_info_list[i]->name);
        newShdr->sh_info = getShInfo(section_info_list[i]->name);
        newShdr->sh_addralign = getShAddralign(section_info_list[i]->name);
        newShdr->sh_entsize = getShEntsize(section_info_list[i]->name);
        shdr_list.push_back(newShdr);
    }
}

//lt
void RelocatableFile::genElfHeader()
{
    //生成elf文件头
    //input: section_info_list,shdr_list
    //output: elf_header

    //e_ident
    memset(elf_header.e_ident, 0, sizeof(elf_header.e_ident));
    elf_header.e_ident[EI_MAG0] = ELFMAG0;
    elf_header.e_ident[EI_MAG1] = ELFMAG1;
    elf_header.e_ident[EI_MAG2] = ELFMAG2;
    elf_header.e_ident[EI_MAG3] = ELFMAG3;
    elf_header.e_ident[EI_CLASS] = ELFCLASS32;
    elf_header.e_ident[EI_DATA] = ELFDATA2LSB;
    elf_header.e_ident[EI_VERSION] = EI_VERSION;

    elf_header.e_type = ET_REL;
    elf_header.e_machine = 40; //ARM
    elf_header.e_version = EV_CURRENT;
    elf_header.e_entry = 0;                  //现在没有程序入口
    elf_header.e_phoff = 0;                  //现在没有程序头部表格
    elf_header.e_shoff = sizeof(Elf32_Ehdr); //紧接着elf头
    elf_header.e_flags = 0x5000000;          //Version5 EABI
    elf_header.e_ehsize = sizeof(Elf32_Ehdr);

    elf_header.e_phentsize=0; //现在没有程序头部表格
    elf_header.e_phnum=0;     //现在没有程序头部表格

    elf_header.e_shentsize = sizeof(Elf32_Shdr);
    elf_header.e_shnum = cur_sec_no - 1;
    elf_header.e_shstrndx = cur_sec_no - 1; //由于最后压入节区名字表，故等于cur_sec_no-1
}

void RelocatableFile::genFile()
{
    //输出到文件
    //input: section_info_list,shdr_list,elf_header
    //output: FILE*
    //TODO
    //elf文件按这个顺序组织：elf头部-节区头部表格-节区
    //节区头部列表和节区列表：按照节区序号顺序组织
    //有的节区需要注意一下4byte对齐 具体可以参考getShAddralign的对齐值

    // 打开文件
    FILE *fp = fopen(this->file_name_.c_str(), "wb");
    if (!fp)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    // 输出文件头
    fwrite(&this->elf_header, sizeof(Elf32_Ehdr), 1, fp);
    // 输出节区头
    for (int i = 0; i < this->shdr_list.size(); i++)
    {
        fwrite(this->shdr_list[i], sizeof(Elf32_Shdr), 1, fp);
    }
    // 输出节区
    for (int i = 0; i < this->section_info_list.size(); i++)
    {
        int size = this->section_info_list[i]->size;
        // 是否已经 align过 ?
        /**
         * Todo 如果没有align,则加上
         *  size = round(size, BASE);
         * 其中一般Section中BASE = 4, 而.text段为16
         * */
        fwrite(this->section_info_list[i]->content, size, 1, fp);
    }
    fclose(fp);
}

Elf32_Word RelocatableFile::getShType(string name)
{
    if (name == ".text" || name == ".data" || name == ".rodata")
        return SHT_PROGBITS;
    if (name == ".strtab" || name == ".shstrtab")
        return SHT_STRTAB;
    if (name == ".symtab")
        return SHT_SYMTAB;
    if (name == ".rel")
        return SHT_REL;
    if (name == ".bss")
        return SHT_NOBITS;
    return SHT_NULL;
}

Elf32_Word RelocatableFile::getShFlags(string name)
{
    if (name == ".data" || name == ".bss")
        return SHF_ALLOC | SHF_WRITE;
    if (name == ".text")
        return SHF_ALLOC | SHF_EXECINSTR;
    if (name == ".rel")
        return SHF_INFO_LINK;
    if (name == ".rodata")
        return SHF_ALLOC;
    return 0;
}

Elf32_Word RelocatableFile::getShLink(string name)
{
    if (name == ".symtab")
    { //找它的相关名字表序号 现在只有唯一的名字表
        for (int i = 0; i < section_info_list.size(); i++)
        {
            if (section_info_list[i]->name == ".strtab")
                return i;
        }
    }
    if (name == ".rel")
    { //找它的相关符号表序号 现在只有唯一的符号表
        for (int i = 0; i < section_info_list.size(); i++)
        {
            if (section_info_list[i]->name == ".symtab")
                return i;
        }
    }
    return 0;
}

Elf32_Word RelocatableFile::getShInfo(string name)
{
    if (name == ".symtab")
    { //找它的最后一个局部符号（bind=STB_LOCAL）的索引 再+1
        return symtab_local_last_idx + 1;
    }
    if (name == ".rel")
    { //找它适用的节区的序号 现在只有唯一的.text
        for (int i = 0; i < section_info_list.size(); i++)
        {
            if (section_info_list[i]->name == ".text")
                return i;
        }
    }
    return 0;
}
Elf32_Word RelocatableFile::getShAddralign(string name)
{
    if (name == ".data" || name == ".bss" || name == ".strtab" || name == ".shstrtab")
        return 1;
    if (name == ".text" || name == ".rel" || name == ".rodata" || name == ".symtab")
        return 4;
    return 0;
}
Elf32_Word RelocatableFile::getShEntsize(string name)
{
    if (name == ".rel")
        return sizeof(Elf32_Rel);
    if (name == ".symtab")
        return sizeof(Elf32_Sym);
    return 0;
}

#endif
