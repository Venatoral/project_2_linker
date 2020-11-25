#ifndef ARM_ANALYZE_HPP
#define ARM_ANALYZE_HPP

#include <cstdio>
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
#include <map>
#include "elfStruct.h"
using namespace std;

class ARM_analyze
{

public:
    static vector<symbols *> symbol_list;
    static vector<reloc_symbol *> reloc_symbol_list;
    static vector<arm_assem *> arm_assem_list;
    static vector<data_element *> data_element_list;
    static vector<bss_element *> bss_element_list;

private:
    int offset_text = 0; //用于记录当前指令或标号（或函数）在.text中的相对位置，每处理一个正常指令（非伪指令、标号、函数），offset_text+=4
    int offset_data = 0; //还设立一个全局变量offset_data记录全局变量在data段中的偏移,每处理一个.word或.sapce offset_data增加对应的数目

    bool is_in_text = false; //判断汇编代码是否进入.text区

public:
    // 构造函数
    ARM_analyze(string arm_assemble)
    {
        arm_analyze(arm_assemble);
    }

private:
    void arm_analyze(string arm_assemble); //逐行读取arm汇编代码arm_assemble将每一行汇编交给arm_handler向不同模块分发
    void arm_handler(string arm);          //分发arm代码
    void _globl_handler(string arm);       //处理  .global  语句
    void _type_handler(string arm);        //处理  .type   语句
    void _label_handler(string arm);       //处理  Lable   语句
    void _data_handler(string arm);        //处理  .space/.word   语句
    void _instruction_handler(string arm); //处理一般的指令
    void _lable_fixer();                   //在所有指令生成后回去处理指令中的跳转标号

public:
    ~ARM_analyze();
};

/**
 * Author: yrc
 * input:   字符串形式的完整的汇编代码
 * 逐行拆分arm代码
 * */
void ARM_analyze::arm_analyze(string arm_assemble)
{
    string arm_code = arm_assemble;
    int size = arm_code.length();
    int head = 0;
    int index = 0;
    while (head != size)
    {
        if (head == 0)
        {
            index = arm_code.find("\n");
        }
        else
        {
            index = arm_code.find("\n", head);
        }
        string str = arm_code.substr(head, index - head);
        index++;
        head = index;

        arm_handler(str);
    }
    _lable_fixer();
}

/**
 * Author: yrc
 * input: 单独一行的汇编代码
 * 根据不同指令特点将指令分发给不同模块处理
 * */
void ARM_analyze::arm_handler(string arm)
{
    if (arm.find(":") != arm.npos)
    {
        _label_handler(arm);

        return;
    }

    if (arm.find(".text") != arm.npos)
    {
        is_in_text = true;

        return;
    }

    if (arm.find(".globl") != arm.npos || arm.find(".global") != arm.npos)
    {
        _globl_handler(arm);

        return;
    }

    if (arm.find(".type") != arm.npos)
    {
        _type_handler(arm);

        return;
    }

    if (arm.find(".word") != arm.npos || arm.find(".space") != arm.npos)
    {
        _data_handler(arm);

        return;
    }

    if (arm.find("pop ") != arm.npos ||
        arm.find("push ") != arm.npos ||
        arm.find("pop") != arm.npos ||
        arm.find("mov ") != arm.npos ||
        arm.find("str ") != arm.npos ||
        arm.find("ldr ") != arm.npos ||
        arm.find("cmp ") != arm.npos ||
        arm.find("mul ") != arm.npos ||
        arm.find("add ") != arm.npos ||
        arm.find("sub ") != arm.npos ||
        arm.find("bl ") != arm.npos ||
        arm.find("b ") != arm.npos ||
        arm.find("beq ") != arm.npos ||
        arm.find("bne ") != arm.npos ||
        arm.find("ble ") != arm.npos ||
        arm.find("bge ") != arm.npos ||
        arm.find("blt ") != arm.npos ||
        arm.find("bgt ") != arm.npos)
    {
        _instruction_handler(arm);

        return;
    }

    return;
}

/**
 * Author: lt
 * input: 单独一行的汇编代码    (.globl n)
 * //TODO
 * 创建一个symbols并做相应初始化
 * 并加入symbol_list
 * */
void ARM_analyze::_globl_handler(string arm)
{
    symbols *a = new symbols();
    int front = 0, end = 0;
    front = arm.find(" ") + 1;
    end = arm.length();
    string n = arm.substr(front, end);

    if (is_in_text)
    { //函数
        a->type = 0;
        a->defined = false; //?
        a->name = n;
        a->value = offset_text;
        a->bind = 0;
    }
    else
    {
        a->type = 1;
        a->defined = false; //?
        a->name = n;
        a->value = offset_data;
        a->bind = 0;
    }

    symbol_list.push_back(a);
}

/**
 * Author: zyj
 * input: 单独一行的汇编代码    (.type main , %type: function)
 * //TODO
 * 根据name在symbol_list中查找对应符号（通常在链表的结尾，可以从后往前查找），将其中的type改为%type对应的值, 没找到的话创建一个Symbols并初始化，
 * */
#define NOTYPE -1
#define FUNCTION 0
#define GLOBAL_VAR 1
#define LABLE 2
#define PREFIX_LEN 6 
//此处PREFIX_LEN 是指.type'\t'总共加起来的语句前缀长度
void ARM_analyze::_type_handler(string arm)
{
    static std::map<string, int> typeHandleMap = {
        {"function", FUNCTION},
        {"object", GLOBAL_VAR},
    };
    string name = "";
    for (auto ch : arm.substr(PREFIX_LEN))
        if ((ch <= 'z' && ch >= 'a') || (ch <= 'Z' && ch >= 'A') || (ch <= '9' && ch >= '0') || ch == '_')
            name += ch;
        else
            break;
    string type = arm.substr(arm.find_first_of('%') + 1);
    bool isFind = false;
    for (auto symbol : ARM_analyze::symbol_list)
        if (symbol->name == name)
        {
            symbol->type = typeHandleMap[type];
            isFind = true;
            return;
        }
    if (!isFind)
    {
        symbols *newSym = new symbols();
        newSym->type = typeHandleMap[type];
        newSym->name = name;
        ARM_analyze::symbol_list.emplace_back(newSym);
    }
}
#undef FUNCTION
#undef GLOBAL_VAR
#undef NOTYPE
#undef LABLE
#undef PREFIX_LEN

/**
 * Author: nzb
 * input: 单独一行的汇编代码    (Merge:/.L0:/a:)
 * //TODO
    对函数的声明Merge：
    根据名称在symbol_list中查找（通常在链表的结尾，可以从后往前查找），如果没找到则创建一个symbols并做相应初始化后加入symbol_list，如果找到的话将对应项中是否定义改为已定义
    如果是标号声明比如.L0
    直接创建symbols初始化后加入symbol_list
    对全局变量声明a:
    直接创建symbols初始化后加入symbol_list

    如何区分是全局变量还是函数？
        根据标识符is_in_text判断汇编是否进入.text
        is_in_text=true 说明是函数
        is_in_textfalse 说明是全局变量
 *
 * */
void ARM_analyze::_label_handler(string arm)
{

    if (arm.find(".") != string::npos)
    {
        int front = 0;
        int end = arm.find(":");
        string n = arm.substr(front, end);

        symbols *a = new symbols();
        a->defined = true;
        a->value = offset_text;
        a->type = 2;
        a->name = n;
        symbol_list.push_back(a);
        return;
    }
    if (is_in_text == true)
    { //判断进入了text，说明是函数
        int front = 0;
        int end = arm.find(":");
        string n = arm.substr(front, end); //找到函数名
        int i = 0;

        for (; i < symbol_list.size(); i++)
        {
            symbols *b = symbol_list[i];
            if (n == b->name)
                break;
        } //寻找等于此符号名的symbol

        if (i < symbol_list.size())         //找到了
            symbol_list[i]->defined = true; //直接改

        if (i == symbol_list.size()) //没找到，新建插入
        {
            int front = 0;
            int end = arm.find(":");
            string n = arm.substr(front, end);

            symbols *a = new symbols();
            a->defined = true;
            a->value = offset_text;
            a->type = 0;
            a->name = n;
            symbol_list.push_back(a);
        }
    }
    else if (is_in_text == false)
    { //全局变量，直接插入
        int front = 0;
        int end = arm.find(":");
        string n = arm.substr(front, end);

        symbols *a = new symbols();
        a->defined = true;
        a->value = offset_data;
        a->type = 1;
        a->name = n;
        symbol_list.push_back(a);
    }
}

/**
 * Author: ml
 * input: 单独一行的汇编代码    (.word 4/.space 400)
 * //TODO
   这里假定我们在之前生成的代码里.text段里没有.word和.space 这种数据声明语句只存在data段中
   根据数据大小移动offset_data,然后根据相应语句和数据生成data_element并加入data_element_list 比如(.word 4   data_element->op_name="word", data_element->value=4)
 * */
void ARM_analyze::_data_handler(string data_inst)
{
    data_element *d = new data_element();
    string op_name;
    int value;
    // 按空格分成 .word 和 400 两部分
    int split_ndx = 0;
    split_ndx = data_inst.find(' ');
    if (split_ndx == string::npos)
    {
        fprintf(stderr, "[data_handler]: invalid instruction!\n");
        exit(EXIT_FAILURE);
    }
    op_name = data_inst.substr(0, split_ndx);
    value = atoi(data_inst.substr(split_ndx).c_str());
    d->op_name = op_name;
    d->value = value;
    // 加入data组中
    data_element_list.push_back(d);
}

/**
 * Author: tlx
 * input: 单独一行的汇编代码    (pop/push/mov/str/ldr/cmp/mul/add/sub/bl/b/beq/bne/ble/bge/bgt/blt)
 * //TODO
 *  正常的汇编指令：
	根据字符串的拆分，分析相应的操作符和操作数，创建一个arm_assem并填写相应的值后加入arm_assem_list

	特殊注意：
	对于跳转语句 如 bl memset(各种跳转)
	根据跳转的目标标号名称在symbol_list中查找，如果没有找到或找到的对应项中defined=false(该符号没有在此文件中定义) 则根据该名称创建一个reloc_symbol,填写相关信息后加入reloc_symbol_list
    然后留空跳转的相对位移(bl #0)。如果找到了函数或者标号，根据当前指令的位置减去跳转符号的位置(在symbol_list中可以找到)可以得到相对位移。然后跳转语句转化成
            bl  #相对位移
    用该语句创建一个arm_assem并填写相应的值后加入arm_assem_list。
        upd: 以上操作在_lable_fixer中完成

	对于引用全局变量的伪指令要做相关处理 如 ldr r4, =n
	创建一个reloc_symbol，名称为n，填写信息后加入reloc_symbol_list
	然后该语句转换成:
            ldr r4, [pc, #-4]
	然后在该指令后面再留空4个字节为后面链接时重定位data段中的值做准备
        留空语句为 nop 
	将转换后的语句和留空语句分别创建arm_assem后加入arm_assem_list
 * */
/*
    附注：
    输入的 R/r 不区分大小写，不保证输出的 R/r 的大小写统一， 所有形如立即数在输出时删去了前面的 #, 立即数支持负数
    push/pop 指令只支持形如 push {r1, r2, r5} 和 push {r1, r2, r5, r8} 的语句
    mov/cmp 只支持 mov/cmp r2,r4 和 mov/cmp r1, #100
    ldr/str 支持 ldr/str r1, [r2] 和 ldr/str r1, [r2, #8]
        ldr 还支持 ldr r4, =var, 实现方式如上所示
    mul/add/sub 只支持 add r1, r2, r3 和 add r1, r2 #100
    跳转指令的op1是相对跳转距离，以字节为单位！！！！！！
*/
void ARM_analyze::_instruction_handler(string arm)
{
    //TODO 代码实在太丑，没有可读性，有机会重构一下，抽取函数
    
    offset_text += 4;
    
    // get operater
    int index1 = arm.find("\t");
    int index2 = arm.find(" ");
    string opera = arm.substr(index1 + 1, index2 - 1);

    arm_assem *arm_asm = new arm_assem;

    arm_asm->op_name = opera;

    bool flag = true; // used in ldr r1, =var

    if(opera == "pop" || opera == "push")
    {
        int ptr = arm.find("{") + 1;
        if(arm.find("-") != arm.npos)
        {
            while(arm[ptr] != 'r' && arm[ptr] != 'R')
                ++ptr;
            
            int first_reg = arm[++ptr] - '0';
            ++ptr;
            if(arm[ptr] >= '0' && arm[ptr] <= '9')
                first_reg = first_reg * 10 + (arm[ptr] - '0');
            
            while(arm[ptr] != 'r' && arm[ptr] != 'R')
                ++ptr;
            
            int last_reg = arm[++ptr] - '0';
            ++ptr;
            if(arm[ptr] >= '0' && arm[ptr] <= '9')
                last_reg = last_reg * 10 + (arm[ptr] - '0');
            
            for(int i = first_reg; i <= last_reg; i++)
            {
                string reg = "r";
                reg.push_back((char)(i + '0'));
                arm_asm->reglist.push_back(reg);
            }
        }
        else
        {
            int ptr = arm.find("{") + 1;
            for(; arm[ptr] != '}'; ptr++)
            {
                if(arm[ptr] == 'r' || arm[ptr] == 'R')
                {
                    int len = (arm[ptr+2] >= '0' && arm[ptr+2] <= '9')? 3 : 2;
                    string reg = arm.substr(ptr, len);
                    arm_asm->reglist.push_back(reg);
                }
            }
        }
    }
    else if(opera == "mov" || opera == "cmp")
    {
        // find op1
        int ptr = index2;
        while(arm[ptr] != 'R' && arm[ptr] != 'r')
            ptr++;
        int len = (arm[ptr+2] >= '0' && arm[ptr+2] <= '9')? 3 : 2;
        arm_asm->Operands1 = arm.substr(ptr, len);

        // find op2
        while(arm[ptr] != ',')
            ptr++;
        ptr++;
        while(arm[ptr] == ' ')
            ptr++;
        if(arm[ptr] == 'r' || arm[ptr] == 'R') // e.g. mov r1,r2
        {
            len = (arm[ptr+2] >= '0' && arm[ptr+2] <= '9')? 3 : 2;
            arm_asm->Operands2 = arm.substr(ptr, len);
        }
        else if(arm[ptr] == '#') //e.g. mov r1, #100
        {
            ptr++;
            if(arm[ptr] == '-')
                arm_asm->Operands2 = "-";
            ptr++;
            for(int i = 0; ; i++)
                if(arm[i + ptr] < '0' || arm[i + ptr] > '9')
                {
                    len = i;
                    break;
                }
            arm_asm->Operands2.append(arm.substr(ptr, len));
        }
    }
    else if(opera == "ldr" || opera == "str")
    {
        // find op1
        int ptr = index2;
        while(arm[ptr] != 'R' && arm[ptr] != 'r')
            ptr++;
        int len = (arm[ptr+2] >= '0' && arm[ptr+2] <= '9')? 3 : 2;
        arm_asm->Operands1 = arm.substr(ptr, len);

        // find op2
        if(arm.find('=') == arm.npos) // ldr r1, [r2(, #100)]
        {
            while(arm[ptr] != '[')
                ptr++;
            ptr++;
            len = (arm[ptr+2] >= '0' && arm[ptr+2] <= '9')? 3 : 2;
            arm_asm->Operands2 = arm.substr(ptr, len);

            // find op3 if exist
            while(arm[ptr] != ']')
            {
                if(arm[ptr] == '#')
                {
                    ptr++;
                    if(arm[ptr] == '-')
                    {
                        arm_asm->Operands3 = "-";
                        ptr++;
                    }   
                    for(int i = 0; ; i++)
                    if(arm[i + ptr] < '0' || arm[i + ptr] > '9')
                    {
                        len = i;
                        break;
                    }
                    arm_asm->Operands3.append(arm.substr(ptr, len));
                    break;
                }
                ptr++;
            }
        }
        else // ldr r1, =var
        {
            while(arm[ptr] != '=')
                ptr++;
            ptr++;
            len = 0;
            for(; len + ptr < arm.length(); len++)
            {
                if(!( (arm[len+ptr]>='a'&&arm[len+ptr]<='z')
                   || (arm[len+ptr]>='A'&&arm[len+ptr]<='Z')
                   || (arm[len+ptr]>='0'&&arm[len+ptr]<='9')
                   || arm[len+ptr]=='_') )
                   break;
            }
            if(len + ptr == arm.length())
                len--;
            string var = arm.substr(ptr, len-ptr+1);

            reloc_symbol *rel = new reloc_symbol;
            rel->name = var;
            rel->type = 1;
            rel->value = offset_text;
            reloc_symbol_list.push_back(rel);

            arm_asm->Operands2 = "pc";
            arm_asm->Operands3 = "-4";
            
            arm_assem *arm_asm2 = new arm_assem;
            arm_asm2->op_name = "nop";
            arm_assem_list.push_back(arm_asm);
            arm_assem_list.push_back(arm_asm2);
            flag = false;

            offset_text+=4;
        }
        
    }
    else if(opera == "mul" || opera == "add" || opera == "sub")
    {
        // find op1
        int ptr = index2;
        while(arm[ptr] != 'R' && arm[ptr] != 'r')
            ptr++;
        int len = (arm[ptr+2] >= '0' && arm[ptr+2] <= '9')? 3 : 2;
        arm_asm->Operands1 = arm.substr(ptr, len);

        // find op2
        ptr += len;
        while(arm[ptr] != 'R' && arm[ptr] != 'r')
            ptr++;
        len = (arm[ptr+2] >= '0' && arm[ptr+2] <= '9')? 3 : 2;
        arm_asm->Operands2 = arm.substr(ptr, len);

        // find op3
        while(arm[ptr] != ',')
            ptr++;
        ptr++;
        while(arm[ptr] == ' ')
            ptr++;
        if(arm[ptr] == 'r' || arm[ptr] == 'R') // e.g. add r1, r2, r3
        {
            len = (arm[ptr+2] >= '0' && arm[ptr+2] <= '9')? 3 : 2;
            arm_asm->Operands3 = arm.substr(ptr, len);
        }
        else if(arm[ptr] == '#') //e.g. add r1, r2,  #100
        {
            ptr++;
            if(arm[ptr] == '-')
                arm_asm->Operands3 = "-";
            ptr++;
            for(int i = 0; ; i++)
                if(arm[i + ptr] < '0' || arm[i + ptr] > '9')
                {
                    len = i;
                    break;
                }
            arm_asm->Operands3.append(arm.substr(ptr, len));
        }
    }
    else if(opera == "bl" || opera == "b" || opera == "beq" ||
            opera == "bne" || opera == "ble" || opera == "bge" ||
            opera == "bgt" || opera == "blt")
    {
        arm_asm->Operands1 = arm.substr(index2 + 1, arm.length() - index2 + 1);
        arm_asm->Operands2 = to_string(offset_text);
    }

    if(flag)
        arm_assem_list.push_back(arm_asm);
}

// 设计见_instruction_handler的注释
void ARM_analyze::_lable_fixer()
{
    int asm_size = arm_assem_list.size();
    int sym_size = symbol_list.size();

    for(int i = 0; i < asm_size; i++)
    {
        if(arm_assem_list[i]->op_name[0] == 'b') // for all jump instruction
        {
            string label = arm_assem_list[i]->Operands1; // destination
            int asm_off = atoi(arm_assem_list[i]->Operands2.c_str()); // jump instruction's next instruction's offset
            int is_filled = false;

            for(int j = 0; j < sym_size; j++)
                if(symbol_list[j]->type == 0 && symbol_list[j]->name == label && symbol_list[j]->defined == true) //find label
                {
                    arm_assem_list[i]->Operands1 = to_string(symbol_list[j]->value - asm_off); // relative address
                    arm_assem_list[i]->Operands2 = "";
                    is_filled = true;
                }
            
            if(!is_filled) // need reloc
            {
                reloc_symbol *rel = new reloc_symbol;
                rel->name = label;
                rel->type = 0;
                rel->value = asm_off - 4;
                reloc_symbol_list.push_back(rel);
                arm_assem_list[i]->Operands1 = "0";
                arm_assem_list[i]->Operands2 = "";
            }
        }
    }
}

#endif
