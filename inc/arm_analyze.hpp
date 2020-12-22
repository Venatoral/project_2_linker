#ifndef ARM_ANALYZE_HPP
#define ARM_ANALYZE_HPP

#include <cstdio>
#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
#include <map>

#include "./elf_struct.h"

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
    void clear();
     //逐行读取arm汇编代码arm_assemble将每一行汇编交给arm_handler向不同模块分发
    void arm_analyze(string arm_assemble);
private:
    void arm_handler(string arm);          //分发arm代码
    void _globl_handler(string arm);       //处理  .global  语句
    void _type_handler(string arm);        //处理  .type   语句
    void _label_handler(string arm);       //处理  Lable   语句
    void _data_handler(string arm);        //处理  .space/.word   语句
    void _instruction_handler(string arm); //处理一般的指令
    void _lable_fixer();                   //在所有指令生成后回去处理指令中的跳转标号
    string get_next_reg(string arm, int &index);
public:
    // 构造函数
    ARM_analyze(string arm_assemble) {
        arm_analyze(arm_assemble);
    }
    ARM_analyze() {};
    ~ARM_analyze() {
        this->clear();
    }
};
#endif
