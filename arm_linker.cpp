#include "./inc/arm_linker.hpp"
#include <cstring>
/**
 * 1. 起始地址 0x08040000  
 * 2.（内存）页对齐：4096字节
 * 3. .text代码段对齐：16字节，其他4字节
 * */

#define BASE_ADDR 0x08040000
#define DESC_ALIGN 4
#define MEM_ALIGN 4096

#define PHDR_SIZE sizeof(Elf32_Phdr)
#define SHDR_SIZE sizeof(Elf32_Shdr)
#define SYM_SIZE sizeof(Elf32_Sym)
#define REL_SIZE sizeof(Elf32_Rel)
#define EHDR_SIZE sizeof(Elf32_Ehdr)

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
    for (; block_idx < block_num; block_idx++) {
        if (off >= blocks[block_idx]->offset_ && off < blocks[block_idx]->offset_ + blocks[block_idx]->size_)
            break;
    }

    unsigned char *pdata = (unsigned char *)(blocks[block_idx]->data_ + (off - blocks[block_idx]->offset_));
    if (type == R_386_JMP_SLOT) { 
        // R_386_JMP_SLOT, 需要注意这个地方只需要修改跳转指令的后3个字节
        int delta = sym_addr - rel_addr - 4; // 跳转的距离
        pdata++;
        *pdata = (unsigned char)(delta % 0x100);
        pdata++;
        *pdata = (unsigned char)((delta >> 8) % 0x100);
        pdata++;
        *pdata = (unsigned char)((delta >> 16) % 0x100);
    } else if (type == R_386_GLOB_DAT) {
        // R_386_GLOB_DAT
        *(unsigned int *)pdata = sym_addr;
    }
}

/**
 * 添加一个目标文件
 * Todo: 构造ElfFile对象，添加到list中
 * */
void Linker::addElf(const char *dir) {
    ElfFile *elf = new ElfFile(dir);
    this->elfs.push_back(elf);
}

// 搜集段信息和符号关联信息
void Linker::collectInfo()
{
    // 扫描输入文件
    for(int i = 0;i < elfs.size(); ++i) {
		ElfFile *elf = elfs[i];
		//记录段表信息
		for(int i=0;i<seg_names_.size();++i) {
			if(elf->shdr_tbl_.find(seg_names_[i]) != elf->shdr_tbl_.end()) {
				this->seg_lists_[seg_names_[i]]->owner_list_.push_back(elf);
            }
        }
		// 记录符号引用信息
		for(auto symIt=elf->sym_tbl_.begin(); symIt!=elf->sym_tbl_.end(); ++symIt) {
            // 所搜该文件的所有有用符号 
            SymLink* symLink=new SymLink();
            symLink->sym_name_=symIt->first;//记录名字
            if(symIt->second->st_shndx==STN_UNDEF) {
                // 引用符号
                symLink->recv_ = elf;   // 记录引用文件
                symLink->prov_ = NULL;  // 标记未定义
                sym_links_.push_back(symLink);
                printf("%s---未定义\n",symLink->sym_name_.c_str());
            } else {
                symLink->prov_ = elf;   // 记录定义文件
                symLink->recv_ = NULL;  // 标示该定义符号未被任何文件引用
                sym_def_.push_back(symLink);
                printf("%s---定义\n",symLink->sym_name_.c_str());
            }
		}
	}
    /* origin
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
    */
}

// 分配地址空间，重新计算虚拟地址空间，磁盘空间连续分布不重新计算，其他的段全部删除
void Linker::allocAddr()
{
    //当前加载基址
    unsigned int curAddr = BASE_ADDR;
    //默认文件偏移,PHT保留.bss段
    unsigned int curOff = EHDR_SIZE + PHDR_SIZE * seg_names_.size(); 
    for (int i = 0; i < seg_names_.size(); ++i) {
        //按照类型分配地址，不紧邻.data与.bss段 
        seg_lists_[seg_names_[i]]->allocAddr(seg_names_[i], curAddr, curOff);
    }
}

// 符号解析，原地计算定义和未定义的符号虚拟地址 (解析先解析已定义符号，然后未定义符号)
void Linker::parseSym()
{
    // 解析以定义符号
    printf("解析定义符号...\n");
    for (SymLink *s : sym_def) {
        Elf32_Sym *sym = s->prov_->sym_tbl_[s->sym_name_];
        string seg_name = s->prov_->shdr_names_[sym->st_shndx];
        sym->st_value += s->prov_->shdr_tbl_[seg_name]->sh_offset;
        printf("name: %s, addr: %8x\n", s->sym_name_.c_str(), sym->st_value);
    }
    printf("解析未定义UND符号...\n");
    for (SymLink *s : sym_ref) {
        Elf32_Sym *def_sym = s->prov_->sym_tbl_[s->sym_name_];
        Elf32_Sym *ref_sym = s->recv_->sym_tbl_[s->sym_name_];
        ref_sym->st_value = def_sym->st_value;
        printf("UND name: %s, addr: %8x\n", s->sym_name_.c_str(), ref_sym->st_value);
    }
}

// 符号重定位，根据所有目标文件的重定位项修正符号地址
void Linker::relocate()
{
    int elf_num = elfs.size();
    for (int i = 0; i < elf_num; i++) {
        ElfFile *elf = elfs[i];

        int rel_num = elf->rel_tbl_.size();
        for (int j = 0; j < rel_num; j++) {
            RelItem *rel = elf->rel_tbl_[j];

            Elf32_Shdr *seg = elf->shdr_tbl_[rel->seg_name_];
            unsigned int rel_addr = seg->sh_addr + rel->rel_->r_offset;

            Elf32_Sym *sym = elf->sym_tbl_[rel->rel_name_];
            unsigned int sym_addr = sym->st_value;

            // origin 
            // unsigned char type = (unsigned char)rel->rel_->r_info;
            // change by ml
            unsigned char type = ELF32_R_TYPE(rel->rel_->r_info);
            seg_lists_[rel->seg_name_]->relocAddr(rel_addr, type, sym_addr);
        }
    }
}

#define APPEND_TO_TAB(targetStr, offset, des)   \
    strcpy(des + index, targetStr);             \
    shstrIndex[targetStr] = index;              \
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
    // 头部 Elf32_Ehdr
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
    elf_exe_.ehdr_.e_ehsize = EHDR_SIZE;
    unsigned int curOff = EHDR_SIZE + PHDR_SIZE * seg_names_.size();
    Elf32_Shdr *pshdr = NULL;
    Elf32_Phdr *pphdr = NULL;
    INIT_SHDR(pshdr, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    elf_exe_.addShdr("", pshdr);
    int shstrtabSize = 26;
    for (auto segName : seg_names_) {
        string name = segName;
        shstrtabSize += name.size() + 1;
        Elf32_Word flags = PF_W | PF_R;
        Elf32_Word filesz = seg_lists_[name]->size_;
        if (name == ".text") {
            flags = PF_W | PF_R;
        } else if (name == ".bss") {
            filesz = 0;
        }
        INIT_PHDR(pphdr, PT_LOAD, seg_lists_[name]->offset_, seg_lists_[name]->baseAddr_, filesz, flags, seg_lists_[name]->size_, MEM_ALIGN);
        elf_exe_.addPhdr(pphdr);
        curOff = seg_lists_[name]->offset_;
        Elf32_Word sh_type = SHT_PROGBITS;
        Elf32_Word sh_flags = SHF_ALLOC | SHF_WRITE;
        Elf32_Word sh_align = 4;
        if (name == ".bss") {
            sh_type = SHT_NOBITS;
        } else if (name == ".text") {
            sh_flags = SHF_ALLOC | SHF_EXECINSTR;
            sh_align = 16;
        }
        INIT_SHDR(pshdr, sh_type, sh_flags, seg_lists_[name]->baseAddr_, seg_lists_[name]->offset_, seg_lists_[name]->size_, SHN_UNDEF, 0, sh_align, 0, 0);
        elf_exe_.addShdr(name, pshdr);
    }
    elf_exe_.ehdr_.e_phoff = EHDR_SIZE;
    elf_exe_.ehdr_.e_phentsize = PHDR_SIZE;
    elf_exe_.ehdr_.e_phnum = seg_lists_.size();
    int index = 0;
    //shstrtab_和strtab_建议改为char*，string会将结束符'\0'当作正常字符处理
    map<string, int> shstrIndex;
    elf_exe_.shstrtab_ = new char[shstrtabSize];
    APPEND_TO_TAB(".shstrtab", 10, elf_exe_.shstrtab_);
    APPEND_TO_TAB(".symtab", 8, elf_exe_.shstrtab_);
    APPEND_TO_TAB(".strtab", 8, elf_exe_.shstrtab_);
    shstrIndex[""] = index - 1;
    for (string name : seg_names_) {
        shstrIndex[name] = index;
        strcpy(elf_exe_.shstrtab_ + index, name.c_str());
        index += name.size() + 1;
    }
    INIT_SHDR(pshdr, SHT_STRTAB, 0, 0, curOff, shstrtabSize, SHN_UNDEF, 0, 1, 0, 0);
    elf_exe_.addShdr(".shstrtab", pshdr);
    elf_exe_.ehdr_.e_shstrndx = elf_exe_.getSegIndex(".shstrtab");
    curOff += shstrtabSize;
    elf_exe_.ehdr_.e_shoff = curOff;
    elf_exe_.ehdr_.e_shentsize = SHDR_SIZE;
    elf_exe_.ehdr_.e_shnum = 4 + seg_names_.size();
    curOff += SHDR_SIZE * (4 + seg_names_.size());
    int tempNum = (1 + sym_def_.size()) * SYM_SIZE;
    INIT_SHDR(pshdr, SHT_SYMTAB, 0, 0, curOff, tempNum, 0, 0, 1, SYM_SIZE, 0);
    elf_exe_.addShdr(".symtab", pshdr);
    int strtabSize = 0;
    elf_exe_.addSym("", NULL);
    for (auto symlink : sym_def_) {
        string name = symlink->sym_name_;
        strtabSize += name.size() + 1;
        Elf32_Sym *Sym = symlink->prov_->sym_tbl_[name];
        Sym->st_shndx = elf_exe_.getSegIndex(symlink->prov_->shdr_names_[Sym->st_shndx]);
        elf_exe_.addSym(name, Sym);
    }
    elf_exe_.ehdr_.e_entry = elf_exe_.sym_tbl_["main"]->st_value;
    curOff += (1 + sym_def_.size()) * SYM_SIZE;
    INIT_SHDR(pshdr, SHT_STRTAB, 0, 0, curOff, strtabSize, SHN_UNDEF, 0, 1, 0, 0);
    elf_exe_.addShdr(".strtab", pshdr);
    index = 0;
    map<string, int> strIndex;
    strIndex[""] = strtabSize - 1;
    string str;
    printf("s1\n");
    for (auto symlink : sym_def_) {
        printf("Debug: %s\n", symlink->sym_name_.c_str());
        strIndex[symlink->sym_name_] = index;
        str += symlink->sym_name_ + to_string('\0');
        index += symlink->sym_name_.size() + 1;
    }
    printf("s2\n");
    for (auto &item : elf_exe_.sym_tbl_) {
        if(!item.second) {
            printf("shit!\n");
            continue;
        }
        item.second->st_name = strIndex[item.first];
    }
    printf("s3\n");
    for (auto &item : elf_exe_.shdr_tbl_) {
        item.second->sh_name = shstrIndex[item.first];
    }
    printf("s4\n");
}
#undef APPEND_TO_TAB
#undef INIT_SHDR
#undef INIT_PHDR

// 输出elf
void Linker::writeExecFile(const char *dir)
{
    // 打开文件
    FILE *fp = fopen(dir, "wb"); //这里先特殊规定一个名字
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    // 将fp文件放到开头
    rewind(fp);
    // 输出文件头
    Elf32_Ehdr ehdr = this->elf_exe_.ehdr_;
    fwrite(&this->elf_exe_.ehdr_, EHDR_SIZE, 1, fp);
    // 输出程序头
    for(auto phdr: this->elf_exe_.phdr_tbl_) {
        fwrite(phdr, ehdr.e_phentsize, 1, fp);
    }
    // 输出节区
    char pad[1] = {0};
    for(int i=0;i<seg_names_.size();++i) {
		SegList* sl = seg_lists_[seg_names_[i]];
		int padnum = sl->offset_ - sl->begin_;
        // 填充
		while(padnum--) {
			fwrite(pad, 1, 1,fp);
        }
		//输出数据
		if(seg_names_[i] != ".bss") {
			Block*old = nullptr;
			char instPad[1]={(char)0x90};
			for(int j = 0; j < sl->blocks.size(); j++) {
				Block*b=sl->blocks[j];
                // 填充小段内的空隙
				if(old) {
					padnum = b->offset_- (old->offset_ + old->size_);
                    // 填充
					while(padnum--) {
						fwrite(instPad, 1, 1, fp);
                    }
				}
				old = b;
				fwrite(b->data_, b->size_ , 1, fp);
            }
        }
    }
    // 输出节区头
    fseek(fp, ehdr.e_shoff, SEEK_SET);
    for (auto hiter: this->elf_exe_.shdr_tbl_) {
        fwrite(hiter.second, ehdr.e_shentsize, 1, fp);
    }
    fclose(fp);
    // 输出节区
    //先根据seglists输出所有的数据区
    // for(auto iter: this->seg_lists_) {
    //     SegList *seg = iter.second;
    //     vector<Block *> b = seg->blocks;
    //     // 定位到对应偏移
    //     fseek(fp, seg->offset_ - seg->baseAddr_, SEEK_SET);
    //     for (int i = 0; i < b.size(); i++) {
    //         Block *w = b[i];
    //         fwrite(w->data_, w->size_, 1, fp);
    //     }
    // }
    // //再输出elf_exe中剩下的节区，主要是符号表、重定位表、shstrtab、strtab
    // //符号表
    // map<string, Elf32_Sym *>::iterator symiter;
    // symiter = this->elf_exe_.sym_tbl_.begin();

    // while (symiter != this->elf_exe_.sym_tbl_.end()) {
    //     Elf32_Sym *su = new Elf32_Sym();
    //     su = symiter->second;
    //     fwrite(su, su->st_size, 1, fp);
    // }
    // //shstrtab
    // fwrite(&this->elf_exe_.shstrtab_, this->elf_exe_.shstrtab_.length(), 1, fp);
    // //strtab
    // fwrite(&this->elf_exe_.strtab_, this->elf_exe_.strtab_.length(), 1, fp);
    // //重定位表
    // vector<RelItem *> f = this->elf_exe_.rel_tbl_;
    // for (int i = 0; i < f.size(); i++)
    // {
    //     Elf32_Rel *r = new Elf32_Rel();
    //     r = f[i]->rel_;
    //     fwrite(r, REL_SIZE, 1, fp);
    // }
}

// 链接（其实就是顺序调用了上述过程)
// dir : 输出可执行文件的地址
void Linker::link(const char *dir)
{
    // 收集信息
    printf("收集elf信息...\n");
    collectInfo();
    // 分配地址空间
    printf("分配地址空间...\n");
    allocAddr();
    // 解析符号
    printf("符号解析...\n");
    parseSym();
    // 重定位
    printf("重定位...\n");
    relocate();
    // 生成可执行文件
    printf("组装可执行文件...\n");
    makeExec();
    // 输出可执行文件到dir
    printf("开始写文件...\n");
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

Linker::~Linker() {
    ;
}