#ifndef ARM_ANALYZE_HPP
#define ARM_ANALYZE_HPP

#include <cstdio>
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
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
void ARM_analyze::_type_handler(string arm)
{
}

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
void ARM_analyze::_data_handler(string arm)
{
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
    
	对于引用全局变量的伪指令要做相关处理 如 ldr r4, =n
	创建一个reloc_symbol，名称为n，填写信息后加入reloc_symbol_list
	然后该语句转换成:
            ldr r4, [pc, #-4]
	然后在该指令后面再留空4个字节为后面链接时重定位data段中的值做准备
	留空语句其实就是	.word	0x00000000
	将转换后的语句和留空语句分别创建arm_assem后加入arm_assem_list
 * */
void ARM_analyze::_instruction_handler(string arm)
{
    //拆分得到操作符
    int index1 = arm.find("\t");
    int index2 = arm.find(" ");
    string opera = arm.substr(index1 + 1, index2 - 1);

    //TODO继续拆分得到操作数

    //TODO如果操作数里发现'=' 或操作符里存在跳转 做特殊处理

    //TODO将相关信息加入列表
}

#endif
