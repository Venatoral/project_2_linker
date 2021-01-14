### 汇编器的方案设计

经过汇编分析器的处理之后，我们得到存储着汇编代码的部分信息的特定数据结构，此时汇编器通过存储的这部分信息以此生成可重定位文件。 

可重定位文件实质上就是ELF文件，ELF文件结构如下图所示：

<center>

<img src="./image/elf-file-format_2.png" width="auto" height="auto" alt="elf-file-format"></img>
图1.ELF文件格式
</center>

链接视图：  
静态链接器（即编译后参与生成最终ELF过程的链接器，如ld ）会以链接视图解析ELF。编译时生成的 .o（目标文件）以及链接后的 .so （共享库）均可通过链接视图解析，链接视图可以没有段表（如目标文件不会有段表）。  
执行视图：  
动态链接器（即加载器，如x86架构 linux下的 /lib/ld-linux.so.2或者安卓系统下的 /system/linker均为动态链接器）会以执行视图解析ELF并动态链接，执行视图可以没有节表。


汇编器生成ELF文件的主要步骤分为以下几个：生成内容固定节区或0号节区，生成特殊节区，生成ELF文件头，输出ELF文件。

**1. 生成内容固定节区或0号节区**  
首先通过**readelf -S 1**读取使用gcc或clang生成的ELF文件的节头表，使用命令**hexdump 1**查看16进制值，发现节头表为空节。故0号节区填入空节即可。

**2. 生成特殊节区**  
生成特殊节区的步骤实质上是根据汇编分析得到的信息将信息填入到相应的ELF文件相关的结构体中，读取第一部分的汇编分析得到的存储信息的数据结构。  
在本次综设项目中，由第一部分汇编分析所得到存有信息的数据结构有：
|  存储变量名称   | 所存储内容  |
|  ----  | ----  |
| bss_element_list  | 没有初始化的和初始化为0的全局变量的数据声明语句和值 |
| data_element_list  | 在编译阶段(而非运行时)确定的数据的数据声明语句和值 |
| arm_assem_list  | 操作语句中操作符名称，操作数名称，寄存器列表 |
| reloc_symbol_list  | 需要重定位的变量或函数的名称和在.text中的偏移量 |
| symbol_list  | 变量或函数的基本信息 |
汇编器生成ELF文件将依赖于这些信息，填入相应的ELF节区对应的结构体。

**3. 生成ELF文件头**  

ELF文件头主要包含以下信息：  

    ELF文件鉴定：确认文件是否是一个ELF文件，并且提供普通文件特征的信息；  
    文件类型：描述文件是一个重定位文件，或可执行文件,或...；  
    目标结构；  
    ELF文件格式的版本；  
    程序入口地址；  
    程序头表的文件偏移；  
    节头表的文件偏移；  
    ELF头(ELF header)的大小；   
    程序头表的表项大小；  

由于本课设项目需要生成的是可直接执行文件，故文件类型确定，填入相应类型的固定的ELF文件头信息即可。


**4. 输出ELF文件**  

按照elf头部-节区头部表格-节区的顺序组织ELF文件，节区按照节区序号顺序组织，运用C++本身的文件IO API将char*指向的内容依次按照偏移量输出在所创建的空文件的正确位置即可。  

### 汇编器的方案实现

基于上述对汇编器的方案设计，可以分析得到实现时所需的必要功能。

首先是生成可重定位文件的类RelocatableFile，它被设计为如下所示的结构。

```c++
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
```
然后是生成0号节区(内容固定的节区)，具体实现如下：

```c++
void RelocatableFile::genSectionNote()
{
    //生成一些无关紧要or内容固定的节区

    SectionInfo *empty_section = new SectionInfo();
    empty_section->no = cur_sec_no++;
    empty_section->name = "";
    empty_section->size = 0;
    empty_section->content = NULL;
    section_info_list.push_back(empty_section);
}
```

然后是各个特殊节区的生成，读取汇编分析器所生成的含有信息的数据结构，    
对于该部分内容，在生成节区的同时要记录着一些在Elf Header，节区头部列表，节区名字表需要用到的信息，例如节区序号，节区偏移量。故在生成节区的方法的具体实现中时候注意需要更新或填充这些量。

首先考虑Bss，Data，Rodata的节区的生成，所依赖到的变量分别为bss_element_list和data_element_list。Rodata节区本来是用于存储常量字符串的，但是本次课设项目没有考虑字符串类型，故略。

```c++
void RelocatableFile::genSectionBss()
{
    //input: bss_element_list

    SectionBss x;
    Elf32_Word sz = 0; //x的大小

    SectionInfo *bss = new SectionInfo();
    bss->no = cur_sec_no++;
    bss->name = ".bss";
    {
        bss->size = (Elf32_Word)sz;
        bss->content = (char *)malloc((size_t)bss->size);
    }
    section_info_list.push_back(bss);
}

void RelocatableFile::genSectionData()
{
    //input: data_element_list

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
    data->size = round(data->size, 4);
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
    // 本次课设没有考虑字符串类型，略
}
```

然后是生成存储可执行机器代码的节区，所依赖的信息域为arm_assem_list，遍历改信息域，通过trans_*(……)方法将具体的汇编语句转化为机器代码并填充到该节区的内容中。  

```c++
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
        /** 此处省略若干段重复性极高的代码 **/
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
    text->size = arm_machine.size();
    text->size = round(text->size, 4);
    section_info_list.push_back(text);
}
```
然后是生成符号表和特殊符号表节区。所依赖的数据域为symbol_list，其中.strtab表的内容在生成.symtab的时候已经生成，直接改变指针指向即可。

```c++
#define NOTYPE -1
#define FUNCTION 0
#define GLOBAL_VAR 1
#define LABEL 2
#define LOCAL 1
#define GLOBAL 0
void RelocatableFile::genSectionSymtab() {
    SectionInfo *symtab = new SectionInfo();
    symtab->no = cur_sec_no++;
    symtab->name = ".symtab";
    int sym_num = ARM_analyze::symbol_list.size();
    // 具体内容
    Elf32_Sym* content = new Elf32_Sym[sym_num];
    Elf32_Sym temp;
    // .strtab
    strtabContent[0] = '\0';
    int offset = 1;
    int curIndex = 0;
    for (auto symbol : ARM_analyze::symbol_list) {
        temp.st_info = ELF32_ST_INFO(symbol->bind, symbol->type);
        temp.st_name = offset;
        if (symbol->name.length() != 0) {   
            for (auto ch : symbol->name) {
                strtabContent[offset++] = ch;
            }
        }
        strtabContent[offset++] = '\0';
        temp.st_value = symbol->value;
        temp.st_size = symbol->name.length();
        temp.st_other = 0;
        //Question: st_shndx如何从symbol_list获取？
        content[curIndex] = temp;
        if (symbol->bind == LOCAL) {
            symtab_local_last_idx = curIndex;
        }
        curIndex ++;
    }
    strtabContentSize = offset;
    symtab->size = (Elf32_Word)(sym_num * sizeof(Elf32_Sym));
    symtab->content = (char *)content;
    section_info_list.emplace_back(symtab);
}
#undef LOCAL 
#undef GLOBAL 
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
```

之后是生成节区名字表，节区头部列表，elf文件头，这几个特殊节区没有依赖的信息域，而是通过前面生成的特殊节区，记录整个ELF文件的总体信息

```c++
void RelocatableFile::genSectionShstrtab()
{
    SectionInfo *shstrtab = new SectionInfo();
    shstrtab->no = cur_sec_no++;
    shstrtab->name = ".shstrtab";
    shstrtab->size = 0;
    section_info_list.push_back(shstrtab);
    for (int i = 0; i < section_info_list.size(); i++)
    {
        shstrtab->size += section_info_list[i]->name.length() + 1;
    }
    shstrtab->content = (char *)malloc(shstrtab->size);
    memset(shstrtab->content, 0, (shstrtab->size) * sizeof(char));
    for (int i = 0, off = 0; i < section_info_list.size(); i++)
    {
        strncpy(shstrtab->content + off, section_info_list[i]->name.c_str(), section_info_list[i]->name.length());
        off += section_info_list[i]->name.length() + 1;
    }
}

void RelocatableFile::genShdrList()
{

    Elf32_Off cur_sec_off = sizeof(Elf32_Shdr) * section_info_list.size()  + sizeof(Elf32_Ehdr);
    Elf32_Off name_off=0;

    for (int i = 0; i < section_info_list.size(); i++)
    {
        Elf32_Shdr *newShdr = new Elf32_Shdr();
        newShdr->sh_name = name_off;
        newShdr->sh_type = getShType(section_info_list[i]->name);
        newShdr->sh_flags = getShFlags(section_info_list[i]->name);
        newShdr->sh_addr = 0; 

        newShdr->sh_offset = cur_sec_off;
        cur_sec_off += section_info_list[i]->size;
        name_off+=section_info_list[i]->name.length()+1;

        newShdr->sh_size = section_info_list[i]->size;
        newShdr->sh_link = getShLink(section_info_list[i]->name);
        newShdr->sh_info = getShInfo(section_info_list[i]->name);
        newShdr->sh_addralign = getShAddralign(section_info_list[i]->name);
        newShdr->sh_entsize = getShEntsize(section_info_list[i]->name);
        shdr_list.push_back(newShdr);
    }
}

void RelocatableFile::genElfHeader()
{

    memset(elf_header.e_ident, 0, sizeof(elf_header.e_ident));
    elf_header.e_ident[EI_MAG0] = ELFMAG0;
    elf_header.e_ident[EI_MAG1] = ELFMAG1;
    elf_header.e_ident[EI_MAG2] = ELFMAG2;
    elf_header.e_ident[EI_MAG3] = ELFMAG3;
    elf_header.e_ident[EI_CLASS] = ELFCLASS32;
    elf_header.e_ident[EI_DATA] = ELFDATA2LSB;
    elf_header.e_ident[EI_VERSION] = EV_CURRENT;

    elf_header.e_type = ET_EXEC;//reloc->exec
    elf_header.e_machine = EM_ARM; //ARM
    elf_header.e_version = EV_CURRENT;
    elf_header.e_entry = 0;                  
    elf_header.e_phoff = 0;                  
    elf_header.e_shoff = sizeof(Elf32_Ehdr); 
    elf_header.e_flags = 0x05000400;          
    elf_header.e_ehsize = sizeof(Elf32_Ehdr);

    elf_header.e_phentsize = 0; 
    elf_header.e_phnum = 0;     

    elf_header.e_shentsize = sizeof(Elf32_Shdr);
    elf_header.e_shnum = section_info_list.size();
    elf_header.e_shstrndx = section_info_list.size() - 1; 
}
```

最后将内存中的内容直接输出到所创建的ELF文件中即可，过程如下：

```c++
void RelocatableFile::genFile()
{
    //elf文件按这个顺序组织：elf头部-节区头部表格-节区
    //节区头部列表和节区列表：按照节区序号顺序组织

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
    fseek(fp, this->elf_header.e_shoff, 0);
    for (int i = 0; i < this->shdr_list.size(); i++) {
        fwrite(this->shdr_list[i], sizeof(Elf32_Shdr), 1, fp);
    }
    // 输出节区
    for (int i = 0; i < this->section_info_list.size(); i++) {
        int index = this->section_info_list[i]->no;
        int off = this->shdr_list[index]->sh_offset;
        int size = this->section_info_list[index]->size;
        printf("%d\n", size);
        fseek(fp, off, 0);
        fwrite(this->section_info_list[i]->content, size, 1, fp);
    }
    fclose(fp);
}
```

实现完上述及其相关方法后，通过总控方法**void genRelocFile()**将这些方法有序地执行，生成ELF文件，并且释放动态分配的内存即可(该部分实现在了RelocatableFile的析构函数中)，至此，ELF文件成功生成。


