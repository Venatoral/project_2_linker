#include "./inc/arm_linker.hpp"
/**
 * 1. 起始地址 0x08040000  
 * 2.（内存）页对齐：4096字节
 * 3. .text代码段对齐：16字节，其他4字节
 * */

#define BASE_ADDR 0x08040000
#define DESC_ALIGN 4
#define MEM_ALIGN 4096


/*
	name: 段名
	off: 文件偏移地址
	base: 加载基址，修改后提供给其他段
*/
void SegList::allocAddr(string name, unsigned int &base, unsigned int &off)
{
    begin_ = off; //记录对齐前偏移

    //虚拟地址对齐，让所有的段按照4k字节对齐
    if (name != ".bss") //.bss段直接紧跟上一个段，一般是.data,因此定义处理段时需要将.data和.bss放在最后
        base += (MEM_ALIGN - base % MEM_ALIGN) % MEM_ALIGN;

    //偏移地址按4字节对齐
    off += (DESC_ALIGN - off % DESC_ALIGN) % DESC_ALIGN;

    //使虚址和偏移按照4k模同余
    base = base - base % MEM_ALIGN + off % MEM_ALIGN;

    baseAddr_ = base;
    offset_ = off;
    size_ = 0;

    //累加段的大小，填充段的数据
    for (int i = 0; i < owner_list_.size(); ++i)
    {
        size_ += (DESC_ALIGN - size_ % DESC_ALIGN) % DESC_ALIGN; //对齐每个小段，按照4字节

        Elf32_Shdr *seg = owner_list_[i]->shdr_tbl_[name];
        //读取需要合并段的数据
        if (name != ".bss")
        {
            char *buf = new char[seg->sh_size];                         //申请数据缓存
            owner_list_[i]->getData(buf, seg->sh_offset, seg->sh_size); //读取数据

            Block *bloc = new Block();
            bloc->data_ = buf;
            bloc->offset_ = size_;
            bloc->size_ = seg->sh_size;
            blocks.push_back(bloc); //记录数据，对于.bss段数据是空的，不能重定位！没有common段！！！
        }

        //修改每个文件中对应段的addr
        seg->sh_addr = base + size_; //修改每个文件的段虚拟，为了方便计算符号或者重定位的虚址，不需要保存合并后文件偏移
        size_ += seg->sh_size;       //累加段大小
    }

    base += size_;      //累加基址
    if (name != ".bss") //.bss段不修改偏移
        off += size_;
}

/*
	rel_addr:重定位虚拟地址
	type:重定位类型
	sym_addr:重定位符号的虚拟地址
    Todo: 根据rel_addr寻找对应block的‘data’中的位置，并根据方式type（绝对，将对）和符号地址sym_addr来修改地址。
*/
void SegList::relocAddr(unsigned int rel_addr, unsigned char type, unsigned int sym_addr)
{
    int off = rel_addr - baseAddr_;
    int block_num = blocks.size();
    int block_idx = 0;
    for (; block_idx < block_num; block_idx++)
    {
        if (off >= blocks[block_idx]->offset_ && off < blocks[block_idx]->offset_ + blocks[block_idx]->size_)
            break;
    }

    unsigned char *pdata = (unsigned char *)(blocks[block_idx]->data_ + (off - blocks[block_idx]->offset_));
    if (type == 7)
    {                                        // R_386_JMP_SLOT, 需要注意这个地方只需要修改跳转指令的后3个字节
        int delta = sym_addr - rel_addr - 4; // 跳转的距离
        pdata++;
        *pdata = (unsigned char)(delta % 0x100);
        pdata++;
        *pdata = (unsigned char)((delta >> 8) % 0x100);
        pdata++;
        *pdata = (unsigned char)((delta >> 16) % 0x100);
    }
    else if (type == 6)
    { // R_386_GLOB_DAT
        *(unsigned int *)pdata = sym_addr;
    }
}

/**
 * 添加一个目标文件
 * Todo: 构造ElfFile对象，添加到list中
 * */
void Linker::addElf(const char *dir)
{
    ElfFile *elf = new ElfFile(dir);
    this->elfs.push_back(elf);
}

// 搜集段信息和符号关联信息
void Linker::collectInfo()
{
    ElfFile *elf_file;
    string curSecName, curSymName;
    Elf32_Shdr *shdr;
    Elf32_Sym *sym;
    Block *block;
    SymLink *symLink;

    //三个段？
    SegList *segText = new SegList();
    SegList *segData = new SegList();
    SegList *segBss = new SegList();
    seg_lists_.insert(pair<string, SegList *>(".text", segText));
    seg_lists_.insert(pair<string, SegList *>(".data", segData));
    seg_lists_.insert(pair<string, SegList *>(".bss", segBss));

    //收集所有elf类[elfs]的信息（段信息[seg_lists]、符号关联信息[sym_links,sym_def]）
    for (int i = 0; i < this->elfs.size(); i++)
    {
        elf_file = elfs[i];
        //段信息
        for (int j = 0; j < elf_file->shdr_names_.size(); j++)
        {
            curSecName = elf_file->shdr_names_[j];
            shdr = elf_file->shdr_tbl_[curSecName];

            if (curSecName == ".text")
            { //插入segText
                //填写新的数据块block 准备插入到segText->blocks中
                block = new Block();
                block->data_;                     //没有指针信息哇
                block->offset_ = shdr->sh_offset; //文件内的偏移
                block->size_ = shdr->sh_size;

                //由于加入了新的文件elf_file存在text节
                //所以保存text段的segText需要增加这一段
                //需要改text段的总大小
                //需要加入新的文件结构指针和数据块指针
                segText->baseAddr_; //这三项
                segText->begin_;    //不该
                segText->offset_;   //现在填吧
                segText->size_ += shdr->sh_size;
                segText->owner_list_.push_back(elf_file);
                segText->blocks.push_back(block);
            }
            else if (curSecName == ".data")
            { //插入segData
                block = new Block();
                block->data_;
                block->offset_ = shdr->sh_offset; //文件内的偏移
                block->size_ = shdr->sh_size;
                segData->baseAddr_; //这三项
                segData->begin_;    //不该
                segData->offset_;   //现在填吧
                segData->size_ += shdr->sh_size;
                segData->owner_list_.push_back(elf_file);
                segData->blocks.push_back(block);
            }
            else if (curSecName == ".bss")
            { //插入segBss
                block = new Block();
                block->data_;
                block->offset_ = shdr->sh_offset; //文件内的偏移
                block->size_ = shdr->sh_size;
                segBss->baseAddr_; //这三项
                segBss->begin_;    //不该
                segBss->offset_;   //现在填吧
                segBss->size_ += shdr->sh_size;
                segBss->owner_list_.push_back(elf_file);
                segBss->blocks.push_back(block);
            }
            else
            { //其他节区不处理
                ;
            }
        }
        //符号信息
        for (int j = 0; j < elf_file->sym_names_.size(); j++)
        {
            curSymName = elf_file->sym_names_[j];
            sym = elf_file->sym_tbl_[curSymName];

            symLink = new SymLink();
            symLink->sym_name_ = curSymName;
            if (sym->st_shndx == SHN_UNDEF)
            { //未定义的符号
                symLink->prov_ = NULL;
                symLink->recv_ = elf_file; //可以确定的是当前文件引用了该符号
            }
            else
            { //自己定义
                //question:需要区分局部符号、全局符号、弱符号吗
                symLink->prov_ = elf_file;
                symLink->recv_ = elf_file;
            }
            sym_links_.push_back(symLink);
        }
    }
}

// 分配地址空间，重新计算虚拟地址空间，磁盘空间连续分布不重新计算，其他的段全部删除
void Linker::allocAddr()
{
    unsigned int curAddr = BASE_ADDR;                  //当前加载基址
    unsigned int curOff = 52 + 32 * seg_names_.size(); //默认文件偏移,PHT保留.bss段

    for (int i = 0; i < seg_names_.size(); ++i) //按照类型分配地址，不紧邻.data与.bss段
    {
        seg_list[seg_names_[i]]->allocAddr(seg_names_[i], curAddr, curOff); //自动分配
    }
}

// 符号解析，原地计算定义和未定义的符号虚拟地址 (解析先解析已定义符号，然后未定义符号)
void Linker::parseSym()
{
    // 解析以定义符号
    printf("解析定义符号...\n");
    for (SymLink *s : sym_def)
    {
        Elf32_Sym *sym = s->prov_->sym_tbl_[s->sym_name_];
        string seg_name = s->prov_->shdr_names_[sym->st_shndx];
        sym->st_value += s->prov_->shdr_tbl_[seg_name]->sh_offset;
        printf("name: %s, addr: %8x\n", s->sym_name_.c_str(), sym->st_value);
    }
    printf("解析未定义UND符号...\n");
    for (SymLink *s : sym_ref)
    {
        Elf32_Sym *def_sym = s->prov_->sym_tbl_[s->sym_name_];
        Elf32_Sym *ref_sym = s->recv_->sym_tbl_[s->sym_name_];
        ref_sym->st_value = def_sym->st_value;
        printf("UND name: %s, addr: %8x\n", s->sym_name_, ref_sym->st_value);
    }
}

// 符号重定位，根据所有目标文件的重定位项修正符号地址
void Linker::relocate()
{
    int elf_num = elfs.size();
    for (int i = 0; i < elf_num; i++)
    {
        ElfFile *elf = elfs[i];

        int rel_num = elf->rel_tbl_.size();
        for (int j = 0; j < rel_num; j++)
        {
            RelItem *rel = elf->rel_tbl_[j];

            Elf32_Shdr *seg = elf->shdr_tbl_[rel->seg_name_];
            unsigned int rel_addr = seg->sh_addr + rel->rel_->r_offset;

            Elf32_Sym *sym = elf->sym_tbl_[rel->rel_name_];
            unsigned int sym_addr = sym->st_value;

            unsigned char type = (unsigned char)rel->rel_->r_info;

            seg_lists_[rel->seg_name_]->relocAddr(rel_addr, type, sym_addr);
        }
    }
}

#define APPEND_TO_TAB(targetStr, offset, des)  \
    string temp = targetStr + to_string('\0'); \
    elf_exe_.des += temp;                      \
    shstrIndex[targetStr] = index;             \
    index += offset

#define INIT_SHDR(p_name, Sh_type, Sh_flags, Sh_addr, Sh_offset, Sh_size, Sh_link, Sh_info, Sh_addralign, Sh_entsize, Sh_name) \
    p_name = new Elf32_Shdr();                                                                                                 \
    p_name->sh_type = Sh_type;                                                                                                 \
    p_name->sh_flags = Sh_flags;                                                                                               \
    p_name->sh_addr = Sh_addr;                                                                                                 \
    p_name->sh_offset = Sh_offset;                                                                                             \
    p_name->sh_size = Sh_size;                                                                                                 \
    p_name->sh_link = Sh_link;                                                                                                 \
    p_name->sh_info = Sh_info;                                                                                                 \
    p_name->sh_addralign = Sh_addralign;                                                                                       \
    p_name->sh_entsize = Sh_entsize;                                                                                           \
    p_name->sh_name = Sh_name

#define INIT_PHDR(p_name, Ph_type, Ph_offset, Ph_vaddr, Ph_filesz, Ph_flags, Ph_memsz, Ph_align) \
    p_name = new Elf32_Phdr();                                                                   \
    p_name->p_type = Ph_type;                                                                    \
    p_name->p_offset = Ph_offset;                                                                \
    p_name->p_vaddr = Ph_vaddr;                                                                  \
    p_name->p_filesz = Ph_filesz;                                                                \
    p_name->p_flags = Ph_flags;                                                                  \
    p_name->p_memsz = Ph_memsz;                                                                  \
    p_name->p_align = Ph_align

// 组装可执行文件
void Linker::makeExec()
{
    int *p_id = (int *)elf_exe_.ehdr_.e_ident;
    //魔数填充
    *p_id = 0x464c457f;
    p_id++;
    *p_id = 0x010101;
    p_id++;
    *p_id = 0;
    p_id++;
    *p_id = 0;
    elf_exe_.ehdr_.e_type = ET_EXEC;
    elf_exe_.ehdr_.e_machine = EM_386;
    elf_exe_.ehdr_.e_version = EV_CURRENT;
    elf_exe_.ehdr_.e_flags = 0;
    elf_exe_.ehdr_.e_ehsize = 52;
    unsigned int curOff = 52 + 32 * seg_names_.size();
    Elf32_Shdr *pshdr = NULL;
    Elf32_Phdr *pphdr = NULL;
    INIT_SHDR(pshdr, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    elf_exe_.addShdr("", pshdr);
    int shstrtabSize = 26;
    for (auto segName : seg_names_)
    {
        string name = segName;
        shstrtabSize += name.size() + 1;
        Elf32_Word flags = PF_W | PF_R;
        Elf32_Word filesz = seg_lists_[name]->size_;
        if (name == ".text")
            flags = PF_W | PF_R;
        if (name == ".bss")
            filesz = 0;
        INIT_PHDR(pphdr, PT_LOAD, seg_lists_[name]->offset_, seg_lists_[name]->baseAddr_, filesz, flags, seg_lists_[name]->size_, MEM_ALIGN);
        elf_exe_.addPhdr(pphdr);
        curOff = seg_lists_[name]->offset_;
        Elf32_Word sh_type = SHT_PROGBITS;
        Elf32_Word sh_flags = SHF_ALLOC | SHF_WRITE;
        Elf32_Word sh_align = 4;
        if (name == ".bss")
            sh_type = SHT_NOBITS;
        if (name == ".text")
        {
            sh_flags = SHF_ALLOC | SHF_EXECINSTR;
            sh_align = 16;
        }
        INIT_SHDR(pshdr, sh_type, sh_flags, seg_lists_[name]->baseAddr_, seg_lists_[name]->offset_, seg_lists_[name]->size_, SHN_UNDEF, 0, sh_align, 0, 0);
        elf_exe_.addShdr(name, pshdr);
    }
    elf_exe_.ehdr_.e_phoff = 52;
    elf_exe_.ehdr_.e_phentsize = 32;
    elf_exe_.ehdr_.e_phnum = seg_lists_.size();
    int index = 0;
    //shstrtab_和strtab_建议改为char*，string会将结束符'\0'当作正常字符处理
    map<string, int> shstrIndex;
    elf_exe_.shstrtab_ = "";
    APPEND_TO_TAB(".shstrtab", 10, shstrtab_);
    APPEND_TO_TAB(".symtab", 8, shstrtab_);
    APPEND_TO_TAB(".strtab", 8, shstrtab_);
    shstrIndex[""] = index - 1;
    for (auto name : seg_names_)
    {
        shstrIndex[name] = index;
        elf_exe_.shstrtab_ += name + to_string('\0');
        index += name.size() + 1;
    }
    INIT_SHDR(pshdr, SHT_STRTAB, 0, 0, curOff, shstrtabSize, SHN_UNDEF, 0, 1, 0, 0);
    elf_exe_.addShdr(".shstrtab", pshdr);
    elf_exe_.ehdr_.e_shstrndx = elf_exe_.getSegIndex(".shstrtab");
    curOff += shstrtabSize;
    elf_exe_.ehdr_.e_shoff = curOff;
    elf_exe_.ehdr_.e_shentsize = 40;
    elf_exe_.ehdr_.e_shnum = 4 + seg_names_.size();
    curOff += 40 * (4 + seg_names_.size());
    int tempNum = (1 + sym_def_.size()) * 16;
    INIT_SHDR(pshdr, SHT_SYMTAB, 0, 0, curOff, tempNum, 0, 0, 1, 16, 0);
    elf_exe_.addShdr(".symtab", pshdr);
    int strtabSize = 0;
    elf_exe_.addSym("", NULL);
    for (auto symlink : sym_def_)
    {
        string name = symlink->sym_name_;
        strtabSize += name.size() + 1;
        Elf32_Sym *Sym = symlink->prov_->sym_tbl_[name];
        Sym->st_shndx = elf_exe_.getSegIndex(symlink->prov_->shdr_names_[Sym->st_shndx]);
        elf_exe_.addSym(name, Sym);
    }
    elf_exe_.ehdr_.e_entry = elf_exe_.sym_tbl_["_start"]->st_value;
    curOff += (1 + sym_def_.size()) * 16;
    INIT_SHDR(pshdr, SHT_STRTAB, 0, 0, curOff, strtabSize, SHN_UNDEF, 0, 1, 0, 0);
    elf_exe_.addShdr(".strtab", pshdr);
    index = 0;
    map<string, int> strIndex;
    strIndex[""] = strtabSize - 1;
    string str;
    for (auto symlink : sym_def_)
    {
        strIndex[symlink->sym_name_] = index;
        str += symlink->sym_name_ + to_string('\0');
        index += symlink->sym_name_.size() + 1;
    }
    for (auto &item : elf_exe_.sym_tbl_)
        item.second->st_name = strIndex[item.first];
    for (auto &item : elf_exe_.shdr_tbl_)
        item.second->sh_name = shstrIndex[item.first];
}
#undef APPEND_TO_TAB
#undef INIT_SHDR
#undef INIT_PHDR

// 输出elf
void Linker::writeExecFile(const char *dir)
{
    // 打开文件
    FILE *fp = fopen("elf.o", "wb"); //这里先特殊规定一个名字
    if (!fp)
    {
        perror("fopen");
        cout << "write error" << endl;
        exit(EXIT_FAILURE);
    }
    //输出文件头
    fwrite(&this->elf_exe_.ehdr_, sizeof(Elf32_Ehdr), 1, fp);

    // 输出节区头

    map<string, Elf32_Shdr *>::iterator hiter;
    hiter = this->elf_exe_.shdr_tbl_.begin();
    while (hiter != this->shdr_tbl_.end())
    {
        Elf32_Shdr *shdr = new Elf32_Shdr();
        shdr = hiter->second;
        fwrite(shdr, sizeof(Elf32_Ehdr), 1, fp);
    }

    // 输出节区
    //先根据seglists输出所有的数据区
    map<string, SegList *>::iterator iter;
    iter = this->seg_lists_.begin();

    while (iter != this->seg_lists_.end())
    {
        SegList *seg = new SegList();
        seg = iter->second;
        vector<Block *> b = seg->blocks;
        for (int i = 0; i < b.size(); i++)
        {
            Block *w = new Block();
            w = b[i];
            fwrite(w->data_, w->size_, 1, fp);
        }
    }
    //再输出elf_exe中剩下的节区，主要是符号表、重定位表、shstrtab、strtab
    //符号表
    map<string, Elf32_Sym *>::iterator symiter;
    symiter = this->elf_exe_.sym_tbl_.begin();

    while (symiter != this->elf_exe_.sym_tbl_.end())
    {
        Elf32_Sym *su = new Elf32_Sym();
        su = symiter->second;
        fwrite(su, su->st_size, 1, fp);
    }
    //shstrtab
    fwrite(&this->elf_exe_.shstrtab_, this->elf_exe_.shstrtab_.length(), 1, fp);
    //strtab
    fwrite(&this->elf_exe_.strtab_, this->elf_exe_.strtab_.length(), 1, fp);
    //重定位表
    vector<RelItem *> f = this->elf_exe_.rel_tbl_;
    for (int i = 0; i < f.size(); i++)
    {
        Elf32_Rel *r = new Elf32_Rel();
        r = f[i];
        fwrite(r, sizeof(Elf32_Rel), 1, fp);
    }
    fclose(fp);
}

// 链接（其实就是顺序调用了上述过程)
// dir : 输出可执行文件的地址
bool Linker::link(const char *dir)
{
    // 收集信息
    collectInfo();
    // 分配地址空间
    allocAddr();
    // 解析符号
    parseSym();
    // 重定位
    relocate();
    // 生成可执行文件
    makeExec();
    // 输出可执行文件到dir
    writeExecFile(dir);
}

Linker::Linker() {
    this->seg_names_.push_back(".text");
    this->seg_names_.push_back(".data");
    this->seg_names_.push_back(".bss");
    for(int i = 0; i < this->seg_names_.size(); i++) {
        this->seg_lists_[this->seg_names_[i]] = new SegList();
    }
}