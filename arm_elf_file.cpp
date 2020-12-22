#include "./inc/arm_linker.hpp"
// 输出ElfFile信息到指定文件

// 构造函数，从file_dir_读取文件，构造ElfFile对象
ElfFile::ElfFile(const char* file_dir_) {
    this->elf_dir_ = string(file_dir_);
    
     FILE* fp = fopen(file_dir_, "rb");
    if (!fp)
    {
        perror("fopen");
        cout << "readfile error" << endl;
        exit(EXIT_FAILURE);
    }

    //读入文件头
    fread(&this->ehdr_, sizeof(Elf32_Ehdr), 1, fp);

    //读入节区头
    vector<Elf32_Shdr*> shdr_list;          //节区头部列表

    for (int i = 0; i < this->ehdr_.e_shnum; i++)
    {
        Elf32_Shdr* s = new Elf32_Shdr();
        fread(s, sizeof(Elf32_Shdr), 1, fp);
        shdr_list.push_back(s);
    }

    //读入节区，利用sh_size得到具体节区，同时根据前面的节区头判断本节区的类型
    vector<SectionInfo*> section_info_list;
    //先获得节区名字表。节区名字表本身在最末尾

    int shstrtabind = this->ehdr_.e_shnum - 1;
    int shstrtabsize = shdr_list[shstrtabind]->sh_size;
    int shstrtaboff= shdr_list[shstrtabind]->sh_offset;
    fseek(fp, 0, SEEK_SET);//转移位置。因为offset与文件头相关
    char stringres[65536];//按最大的分配内存，之后继续还可用于获得节区名字
    fread(stringres,shstrtabsize 1, fp + shstrtaboff);//读入数据
    this->shstrtab_ = stringres;//shstrtab读取并转为应有的格式

    char stringres1[65536];//按最大的分配内存，这个stringres1是保存strtab数据所用的。之后继续还可用于获得符号名字
    //下面再单独找strtab，为获得各类名称做准备
    for (int i = 0; i < this->ehdr_.e_shnum; i++)
    {
        if (shdr_list[i]->sh_type == SHT_STRTAB) {
            if (i != shstrtabind) {//不是shstrtab，那么就是strtab，存到string的方式与上文类似
                fseek(fp, 0, SEEK_SET);//转移位置。因为offset与文件头相关
                int strtabsize = shdr_list[i]->sh_size;
                int strtaboff = shdr_list[i]->sh_offset;
                
                fread(stringres1, strtabsize 1, fp + strtaboff);//读入数据
                this->strtab_ = stringres1;//strtab读取并转为应有的格式

            }
        }
    }
    //接下来处理section头和section的名字。这个需要直接用到之前的section头列表
    for (int i = 0; i < this->ehdr_.e_shnum; i++)
    {
        Elf32_Sym* sm = shdr_list[i];
        int need = sm->st_name;
        int head = 0;
        int end = 0;
        char dest[15] = { "" };
        int count = 0;
        for (int k = 0; k <= 65535; k++) {//找到符号的位置，用头尾做处理
            if (count < need - 1 && stringres[k] != '\0') {
                head++;

            }
            if (count < need - 1 && stringres[k] == '\0') {
                count++;
                end = head + 1;//给end赋予初值
                head=head+2;//这样的话就可以令需要得到的字段从\0后开始
                
                
            }
            if (count == need - 1 && stringres[k] != '\0') {
                end++;
            }
            if (count == need - 1 && stringres[k] == '\0') {
                end++; //最后一个/0赋好了
                break;
            }
        }

        char res[end-head+1] = { "" };//真正得到节区名
        for (int m = 0; m < end - head + 1; m++) {
            res[m] = stringres[head];
            head++;
        }
        string na = res;


        this->shdr_names_.push_back(na);//存入节区名
        this->shdr_tbl_.insert(pair<string, Elf32_Shdr *>(na,sm));//存入节区map

    }
    //总体处理其他各种结果
    for (int i = 0; i < this->ehdr_.e_shnum; i++)
    {
        if (shdr_list[i]->sh_type== SHT_PROGBITS) {
            //数据，但对于构造函数似乎无用
        }
        
        if (shdr_list[i]->sh_type == SHT_SYMTAB) {
            //这种情况为符号表

            int symsize = shdr_list[i]->sh_size;
            int symoff = shdr_list[i]->sh_offset;
            int symnum = symsize / sizeof(Elf32_Sym);
            fseek(fp, 0, SEEK_SET);//转移位置。因为offset与文件头相关
            for (int k = 0; k < symnum; k++) {
                Elf32_Sym* sy = new Elf32_Sym();
                fseek(fp, 0, SEEK_SET);//转移位置。因为offset与文件头相关
                fread(sy, sizeof(Elf32_Sym), 1, fp + symoff+k* sizeof(Elf32_Sym));//每次读一个直到读完,因为每次要把指针移到最前面所以要记得加上上次已经读取完成的长度

                char dest[15] = { "" };
                strncpy(dest, stringres1 + sy->st_name, sy->st_size);
                string na = dest;


                this->sym_names_.push_back(na);//符号名字表插入
                this->sym_tbl_.insert(pair<string, Elf32_Sym*>(na, l));//符号表

            }
        }
        if (shdr_list[i]->sh_type == SHT_REL) {
            //这种情况为重定位表
            
            int relsize = shdr_list[i]->sh_size;
            int reloff = shdr_list[i]->sh_offset;
            int relnum = relsize / sizeof(Elf32_Rel);
            for (int k = 0; k < relnum; k++) {
                Elf32_Rel* rel = new Elf32_Rel();
                RelItem* Rel = new RelItem();
                fseek(fp, 0, SEEK_SET);//转移位置。因为offset与文件头相关
                fread(rel, sizeof(Elf32_Rel), 1, fp + reloff + k * sizeof(Elf32_Rel));//每次读一个直到读完,因为每次要把指针移到最前面所以要记得加上上次已经读取完成的长度
                int j=rel->r_info;

                //看有没有和他相对应的elf32_sym的部分可以利用了，遍历之前的map。应该sym里面有relocsym？
                map<string, Elf32_Sym*>::iterator iter;
                iter = this->sym_tbl_.begin();
                string relnam;
                while (iter != this->sym_tbl_.end()) {
                    if (iter->second->st_name == j)
                        relnam = iter->first;//第二个的索引等于第一个，那么应该是一样的，用那个符号的名字就是重定位符号的名字
                }
           

                Rel->seg_name_=  ".rel";//只有一个重定位节区，所以段名字应该都一样
                Rel->rel_ = rel;
                Rel->rel_name_ = relnam;

                this->rel_tbl_.push_back(Rel);//存入



            }
        }
        if (shdr_list[i]->sh_type == SHT_NOBITS) {
            //好像也还没有处理bss的属性
        }
        
    }
    //关闭文件指针
    fclose(fp);
    
  

}

// 析构函数，释放内存空间
ElfFile::~ElfFile() {

}

// 从对应文件 offset位置开始，读取size字节到buf中
void ElfFile::getData(char *buf, Elf32_Off offset, Elf32_Word size) {
    FILE* fp = fopen(this->elf_dir_.c_str(), "rb");
    if(!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fseek(fp, offset, SEEK_SET);
    fread(buf, size, 1, fp);
    fclose(fp);
}

// 获取指定段名在段表下标
int ElfFile::getSegIndex(string seg_name) {
    int i = 0;
    while(i < shdr_names_.size()) {
        if(shdr_names_[i] == seg_name) {
            return i;
        }
        i ++;
    }
    // 没找到对应段
    return -1;
}

// 获取指定符号名在符号表下标
int ElfFile::getSymIndex(string sym_name) {
    int i = 0;
    while(i < sym_names_.size()) {
        if(sym_names_[i] == sym_name) {
            return i;
        }
        i ++;
    }
    // 没找到符号
    return -1;
}

// 添加程序头表项
void ElfFile::addPhdr(Elf32_Phdr *new_phdr) {
    this->phdr_tbl_.push_back(new_phdr);
}

// 添加一个段表项
void ElfFile::addShdr(string shdr_name, Elf32_Shdr *new_shdr) {
    this->shdr_names_.push_back(shdr_name);
    this->shdr_tbl_[shdr_name] = new_shdr;
}

// 添加一个符号表项
void ElfFile::addSym(string st_name, Elf32_Sym *new_sym) {
    this->sym_names_.push_back(st_name);
    this->sym_tbl_[st_name] = new_sym;
}

