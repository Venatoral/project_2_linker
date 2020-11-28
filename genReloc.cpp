#include <cstdio>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include "elfstruct.h"
using namespace std;

class RelocatableFile {
private:
    string file_name_;
public:
    //读入
    vector<symbols*> symbol_list;//函数和全局变量的符号 函数的偏移
    vector<reloc_symbol*> reloc_symbol_list;//重定位符号和重定位位置...
    vector<arm_assem*> arm_assem_list;//储存分析好的汇编代码...一一对应...
    vector<data_element*> data_element_list;//data段变量名称和值
    vector<bss_element*> bss_element_list;//bss段变量名称和值

    //生成的结构
    vector<SectionInfo*> section_info_list;//节区列表
    vector<Elf32_Shdr*> shdr_list;//节区头部列表
    Elf32_Ehdr elf_header;//elf文件头

    //生成的过程中需要用到的变量
    Elf32_Half cur_sec_no=0;//记录现在是第几个节区 0号节区有留空规则 每生成一个节区记得自增

    //生成.o文件使用的函数列表
public:
    // ml: 可以把 freeMalloc() 的工作放到析构函数？
    ~RelocatableFile() {
        #define FREE_LIST(x) \
            for(int i = 0; i < x.size(); i++) { \
                free(x[i]); \
            }
        FREE_LIST(this->symbol_list)
        FREE_LIST(this->reloc_symbol_list)
        FREE_LIST(this->arm_assem_list)
        FREE_LIST(this->data_element_list)
        FREE_LIST(this->bss_element_list)
        FREE_LIST(section_info_list)
        FREE_LIST(shdr_list)
        #undef FREE_LIST
    }
    // ml: 构造函数指定文件名称
    RelocatableFile(string file_name) {
        this->file_name_ = file_name;
    }

    void genSectionNote();//生成一些无关紧要or内容固定的节区or第0号节区

    //tlx
    void genSectionBss();//生成特殊节区-未初始化数据（初始化为0 不占用文件空间）--给这个函数写了个示意可以参考看看
    void genSectionData();//生成特殊节区-已初始化数据
    void genSectionRodata();//生成特殊节区-只读数据（如printf函数里面的格式控制字符串）

    //yrc
    void genSectionText();//生成特殊节区-可执行代码
    
    //nzb
    void genSectionReloc();//生成特殊节区-重定位表

    //zyj
    void genSectionSymtab();//生成特殊节区-符号表
    void genSectionStrtab();//生成特殊节区-符号名字表

    
    //lt
    void genSectionShstrtab();//生成特殊节区-节区名字表
    void genShdrList();//生成节区头部列表
    void genElfHeader();//生成elf文件头

    //ml
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

    SectionInfo* bss=new SectionInfo();
    bss->no=cur_sec_no++;
    bss->name=".bss";

    {
        //TODO 根据bss_element_list生成节区内容存到[X]里
        //TODO 存内容的时候维护sz
        bss->size=(Elf32_Word)sz;
        bss->content=(char *)malloc((size_t)bss->size);
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
    SectionInfo *rel=new SectionInfo();
    rel->no=cur_sec_no;
    cur_sec_no++;
    rel->name=".rel";

    //计算节区大小
    rel->size=reloc_symbol_list.size()*sizeof(Elf32_Rel);
    rel->content=(char *)malloc(rel->size);
    //填入节区内容
    for(int i=0,off=0;i<reloc_symbol_list.size();i++){

        Elf32_Rel *r=new Elf32_Rel();
        int j=0;
        for(;j<symbol_list.size();j++){//找到符号表索引，为确定r_info作准备。
            if(reloc_symbol_list[i]->name==symbol_list[j]->name)
                break;

        }
        r->r_info=j<<8;//ELF32_R_SYM(i)的逆过程
        r->r_offset=reloc_symbol_list[i]->value;

        if(reloc_symbol_list[i]->type==0){//0是函数，类型应该是R_386_JMP_SLOT    7
            r->r_info=r->r_info+7;//ELF32_R_TYPE(i)的逆过程
        };
        if(reloc_symbol_list[i]->type==1){//1是全局变量，类型应该是R_386_GLOB_DAT    6
            r->r_info=r->r_info+6;//ELF32_R_TYPE(i)的逆过程
        };


        memcpy(rel->content+off,r,sizeof(Elf32_Rel));
        off+=sizeof(Elf32_Rel);
    }
    //存到列表里
    section_info_list.push_back(rel);

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

//lt
void RelocatableFile::genSectionShstrtab(){
    //生成特殊节区-节区名字表
    //input: section_info_list,this
    //output: section_info_list.push_back(xxx);
    SectionInfo *shstrtab=new SectionInfo();
    shstrtab->no=cur_sec_no;
    cur_sec_no++;
    shstrtab->name=".shstrtab";
    shstrtab->size=0;
    shstrtab->content=NULL;
    section_info_list.push_back(shstrtab);

    //计算节区大小
    for(int i=0;i<cur_sec_no;i++){
        shstrtab->size+=section_info_list[i]->name.length();
    }
    shstrtab->content=(char *)malloc(shstrtab->size);

    //填入节区内容
    for(int i=0,off=0;i<cur_sec_no;i++){
        off+=section_info_list[i]->name.length();
        strncpy(shstrtab->content+off,section_info_list[i]->name,section_info_list[i]->name.length());
    }
    //之前已经push_back了
}

//lt
void RelocatableFile::genShdrList(){
    //生成节区头部列表
    //input: section_info_list
    //output: shdr_list
}

//lt
void RelocatableFile::genElfHeader(){
    //生成elf文件头
    //input: section_info_list,shdr_list
    //output: elf_header
}


void RelocatableFile::genFile(){
    //输出到文件
    //input: section_info_list,shdr_list,elf_header
    //output: FILE*
    // 打开文件
    FILE* fp = fopen(this->file_name_.c_str(), "wb");
    if(!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    // 输出文件头
    fwrite(&this->elf_header, sizeof(Elf32_Ehdr), 1, fp);
    // 输出节区
    for(int i = 0; i < this->section_info_list.size(); i++) {
        int size = this->section_info_list[i]->size;
        // 是否已经 align过 ?
        /**
         * Todo 如果没有align,则加上
         *  size = round(size, BASE);
         * 其中一般Section中BASE = 4, 而.text段为16
         * */
        fwrite(this->section_info_list[i]->content, size, 1, fp);
    }
    // 输出节区头
    for(int i = 0; i < this->shdr_list.size(); i++) {
        fwrite(this->shdr_list[i], sizeof(Elf32_Shdr), 1, fp);
    }
    fclose(fp);
}

