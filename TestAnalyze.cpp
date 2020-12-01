/**
 * 运行指令
 * clang++ -std=c++17 -O2 -lm main.cpp -o compiler  
   ./compiler arm.s result.txt
 * */

#include "arm_analyze.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <stdlib.h>

vector<symbols *> ARM_analyze::symbol_list;
vector<reloc_symbol *> ARM_analyze::reloc_symbol_list;
vector<arm_assem *> ARM_analyze::arm_assem_list;
vector<data_element *> ARM_analyze::data_element_list;
vector<bss_element *> ARM_analyze::bss_element_list;

//从文件读入到string里
string readFileIntoString(char *filename)
{
    ifstream ifile(filename);
    //将文件读入到ostringstream对象buf中
    ostringstream buf;
    char ch;
    while (buf && ifile.get(ch))
        buf.put(ch);
    //返回与流对象buf关联的字符串
    return buf.str();
}

int main(int argc, char const *argv[])
{
    char *fn = "arm.s";
    string arm_assemble;
    arm_assemble = readFileIntoString(fn);

    freopen(argv[2], "w", stdout);

    ARM_analyze *_arm_analyze = new ARM_analyze(arm_assemble);

    cout << "symbol_list:\n";
    for (int i = 0; i < ARM_analyze::symbol_list.size(); i++)
    {
        cout << "\tbind:" << ARM_analyze::symbol_list[i]->bind << "\t";
        cout << "\tdefined:" << ARM_analyze::symbol_list[i]->defined << "\t";
        cout << "\tname:" << ARM_analyze::symbol_list[i]->name << "\t";
        cout << "\ttype:" << ARM_analyze::symbol_list[i]->type << "\t";
        cout << "\tvalue:" << ARM_analyze::symbol_list[i]->value << "\t";
        cout << "\n";
    }

    cout << "reloc_symbol_list:\n";
    for (int i = 0; i < ARM_analyze::reloc_symbol_list.size(); i++)
    {
        cout << "\tname:" << ARM_analyze::reloc_symbol_list[i]->name << "\t";
        cout << "\ttype:" << ARM_analyze::reloc_symbol_list[i]->type << "\t";
        cout << "\tvalue:" << ARM_analyze::reloc_symbol_list[i]->value << "\t";
        cout << "\n";
    }
    cout << "arm_assem_list:\n";
    for (int i = 0; i < ARM_analyze::arm_assem_list.size(); i++)
    {
        cout << "\top_name:" << ARM_analyze::arm_assem_list[i]->op_name << "\t";
        cout << "\tOperands1:" << ARM_analyze::arm_assem_list[i]->Operands1 << "\t";
        cout << "\tOperands2:" << ARM_analyze::arm_assem_list[i]->Operands2 << "\t";
        cout << "\tOperands3:" << ARM_analyze::arm_assem_list[i]->Operands3 << "\t";

        cout << "\tReg_list:";
        for (int j = 0; j < ARM_analyze::arm_assem_list[i]->reglist.size(); j++)
        {
            cout << ARM_analyze::arm_assem_list[i]->reglist[j]<< "\t";
        }
        cout << "\n";
    }
    cout << "data_element_list:\n";
    for (int i = 0; i < ARM_analyze::data_element_list.size(); i++)
    {
        cout << "\tname:" << ARM_analyze::data_element_list[i]->name << "\t";
        cout << "\tvalue:" << ARM_analyze::data_element_list[i]->value << "\t";
        cout << "\n";
    }
    cout << "bss_element_list:\n";
    for (int i = 0; i < ARM_analyze::bss_element_list.size(); i++)
    {
        cout << "\tname:" << ARM_analyze::bss_element_list[i]->name << "\t";
        cout << "\n";
    }
}