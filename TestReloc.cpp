/**
compile and run:
clang++ -std=c++17 -O2 -lm TestReloc.cpp -o testreloc
./testreloc arm.s result2.txt
**/
#include "elfStruct.h"
#include "arm_analyze.hpp"
#include "genReloc.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdlib.h>
using namespace std;


vector<symbols *> ARM_analyze::symbol_list;
vector<reloc_symbol *> ARM_analyze::reloc_symbol_list;
vector<arm_assem *> ARM_analyze::arm_assem_list;
vector<data_element *> ARM_analyze::data_element_list;
vector<bss_element *> ARM_analyze::bss_element_list;

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

int main(int argc,char *argv[]){
    //read
    string arm_assemble = readFileIntoString(argv[1]);
    ARM_analyze *_arm_analyze = new ARM_analyze(arm_assemble);
    
    //gen elf
    RelocatableFile *elf_relocfile=new RelocatableFile(argv[2]);
    
    return 0;
}