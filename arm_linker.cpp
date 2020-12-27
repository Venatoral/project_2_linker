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
    int align = DESC_ALIGN;
	if(name == ".text") {
		align = 8;
    }
    //偏移地址按4字节对齐
    off += (align - off % align) % align;

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
    unsigned off = rel_addr - baseAddr_;
    int block_num = blocks.size();
    int block_idx = 0;
    for (; block_idx < block_num; block_idx++) {
        if (off >= blocks[block_idx]->offset_ && off < blocks[block_idx]->offset_ + blocks[block_idx]->size_)
            break;
    }

    int *pdata = (int*)(blocks[block_idx]->data_ + off - blocks[block_idx]->offset_);
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
    } else if (type==R_386_32) {
        // 绝对地址修正 
		*pdata = sym_addr;
	} else if(type==R_386_PC32) {
        // 相对地之修正 
		*pdata = sym_addr - rel_addr + (*pdata);
	}
}

/**
 * 添加一个目标文件
 * Todo: 构造ElfFile对象，添加到list中
 * */
void Linker::addElf(const char *dir) {
    printf("读取 %s\n", dir);
    ElfFile *elf = new ElfFile(dir);
    printf("读取 %s 结束\n", dir);
    this->elfs.push_back(elf);
}

// 搜集段信息和符号关联信息
void Linker::collectInfo()
{
    // 扫描输入文件
    for(int i = 0;i < elfs.size(); ++i) {
		ElfFile *elf = elfs[i];
		//记录段表信息
		for(int i = 0; i < seg_names_.size(); i++) {
			if(elf->shdr_tbl_.find(seg_names_[i]) != elf->shdr_tbl_.end()) {
				this->seg_lists_[seg_names_[i]]->owner_list_.push_back(elf);
            }
        }
		// 记录符号引用信息
        for(auto symIt: elf->sym_tbl_) {
            // 所搜该文件的所有有用符号 
            SymLink* symLink = new SymLink();
            symLink->sym_name_ = symIt.first;//记录名字
            if(symIt.second->st_shndx == STN_UNDEF) {
                // 引用符号
                symLink->recv_ = elf;   // 记录引用文件
                symLink->prov_ = NULL;  // 标记未定义
                sym_ref_.push_back(symLink);
                printf("%s---未定义\n",symLink->sym_name_.c_str());
            } else {
                symLink->prov_ = elf;   // 记录定义文件
                symLink->recv_ = NULL;  // 标示该定义符号未被任何文件引用
                sym_def_.push_back(symLink);
                printf("%s---定义\n",symLink->sym_name_.c_str());
            }
		}
	}
    // 遍历未定义符号，并连接至以定义位置
   	for(int i = 0; i < sym_ref_.size(); ++i) {
        // 遍历定义的符号
		for(int j = 0; j < sym_def_.size(); ++j) {
			if(ELF32_ST_BIND(sym_def_[j]->prov_->sym_tbl_[sym_def_[j]->sym_name_]->st_info) != STB_GLOBAL)//只考虑全局符号
				continue;
            //  同名符号
			if(sym_ref_[i]->sym_name_ == sym_def_[j]->sym_name_) {
				sym_ref_[i]->prov_ = sym_def_[j]->prov_; // 记录符号定义的文件信息
				sym_def_[j]->recv_ = sym_def_[j]->prov_; // 该赋值没有意义，只是保证recv不为NULL
			}
		}
        // 未定义
		if(sym_ref_[i]->prov_ == NULL) {
            printf("%s undefined!\n", sym_ref_[i]->sym_name_.c_str());
            exit(EXIT_FAILURE);
        }
	}
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
    for (SymLink *s : sym_def_) {
        Elf32_Sym *sym = s->prov_->sym_tbl_[s->sym_name_];
        string seg_name = s->prov_->shdr_names_[sym->st_shndx];
        sym->st_value += s->prov_->shdr_tbl_[seg_name]->sh_addr;
        printf("name: %s, addr: %8x\n", s->sym_name_.c_str(), sym->st_value);
    }
    printf("解析未定义UND符号...\n");
    for (SymLink *s : sym_ref_) {
        Elf32_Sym *def_sym = s->prov_->sym_tbl_[s->sym_name_];
        Elf32_Sym *ref_sym = s->recv_->sym_tbl_[s->sym_name_];
        ref_sym->st_value = def_sym->st_value;
        printf("UND name: %s, addr: %8x\n", s->sym_name_.c_str(), ref_sym->st_value);
    }
    printf("符号解析完成\n");
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


void Linker::makeExec()
{
	// 初始化文件头
	int* p_id = (int*)(this->elf_exe_.ehdr_.e_ident);
	*p_id = 0x464c457f;
    p_id++;
    *p_id = 0x010101;
    p_id++;
    *p_id = 0;
    p_id++;
    *p_id = 0;
	this->elf_exe_.ehdr_.e_type = ET_EXEC;
	this->elf_exe_.ehdr_.e_machine = EM_386;
	this->elf_exe_.ehdr_.e_version = EV_CURRENT;
	this->elf_exe_.ehdr_.e_flags = 0;
	this->elf_exe_.ehdr_.e_ehsize=52;
	// 数据位置指针
	unsigned int curOff = EHDR_SIZE + PHDR_SIZE * this->seg_names_.size();
	this->elf_exe_.addShdr("", 0, 0, 0, 0, 0, 0, 0, 0, 0);//空段表项
    // ".shstrtab".length()+".symtab".length()+".strtab".length()+3;
	int shstrtabSize = 26; // 段表字符串表大小
	for(int i = 0; i < this->seg_names_.size(); i++) {
		string name = this->seg_names_[i];
		shstrtabSize += name.length() + 1; // 考虑结束符'\0'
		//生成程序头表
		Elf32_Word flags = PF_W|PF_R;//读写
		Elf32_Word filesz = this->seg_lists_[name]->size_;//占用磁盘大小
		if(name == ".text") {
            flags = PF_X|PF_R;
        }
        if(name == ".bss") {
            filesz = 0; // .bss段不占磁盘空间
        }
		this->elf_exe_.addPhdr(PT_LOAD, this->seg_lists_[name]->offset_, this->seg_lists_[name]->baseAddr_,
		filesz,this->seg_lists_[name]->size_,flags,MEM_ALIGN);//添加程序头表项
		//计算有效数据段的大小和偏移,最后一个决定
		curOff=this->seg_lists_[name]->offset_;//修正当前偏移，循环结束后保留的是.bss的基址
		//生成段表项
		Elf32_Word sh_type = SHT_PROGBITS;
		Elf32_Word sh_flags = SHF_ALLOC|SHF_WRITE;
		Elf32_Word sh_align = 4; // 4B
		if(name == ".bss") {
            sh_type = SHT_NOBITS;
        }
		if(name == ".text") {
			sh_flags = SHF_ALLOC|SHF_EXECINSTR;
			sh_align = 8; // 8B
		}
		this->elf_exe_.addShdr(name, sh_type, sh_flags, this->seg_lists_[name]->baseAddr_, this->seg_lists_[name]->offset_,
			this->seg_lists_[name]->size_, SHN_UNDEF, 0, sh_align, 0);//添加一个段表项，暂时按照4字节对齐
	}
	this->elf_exe_.ehdr_.e_phoff = 52;
	this->elf_exe_.ehdr_.e_phentsize = 32;
	this->elf_exe_.ehdr_.e_phnum = this->seg_names_.size();
	// 填充shstrtab数据
	char* str = this->elf_exe_.shstrtab_ = new char[shstrtabSize];
	this->elf_exe_.shstrtabSize_ = shstrtabSize;
	int index = 0;
	//段表串名与索引映射
	map<string, int> shstrIndex;
	shstrIndex[".shstrtab"]=index;
    strcpy(str + index,".shstrtab");
    index += 10;
	shstrIndex[".symtab"]=index;
    strcpy(str + index,".symtab");
    index += 8;
	shstrIndex[".strtab"]=index;
    strcpy(str + index,".strtab");
    index += 8;
	shstrIndex[""] = index - 1;
	for(int i = 0;i<this->seg_names_.size();++i) {
		shstrIndex[this->seg_names_[i]] = index;
		strcpy(str + index, this->seg_names_[i].c_str());
		index += this->seg_names_[i].length() + 1;
	}
	//生成.shstrtab
	this->elf_exe_.addShdr(".shstrtab", SHT_STRTAB, 0, 0,curOff,shstrtabSize,SHN_UNDEF,0,1,0);//.shstrtab
	this->elf_exe_.ehdr_.e_shstrndx = this->elf_exe_.getSegIndex(".shstrtab");//1+this->seg_names_.size();//空表项+所有段数
	curOff += shstrtabSize;//段表偏移
	this->elf_exe_.ehdr_.e_shoff = curOff;
	this->elf_exe_.ehdr_.e_shentsize = 40;
	this->elf_exe_.ehdr_.e_shnum = 4 + this->seg_names_.size();//段表数
	// 生成符号表项
	curOff += SHDR_SIZE * (4 + this->seg_names_.size());//符号表偏移
	// 符号表位置=（空表项+所有段数+段表字符串表项+符号表项+字符串表项）*40
	// .symtab,sh_link 代表.strtab索引，默认在.symtab之后,sh_info不能确定
	this->elf_exe_.addShdr(".symtab",SHT_SYMTAB, 0, 0, curOff, (1 + sym_def_.size()) * SYM_SIZE, 0, 0, 1, 16);
	this->elf_exe_.shdr_tbl_[".symtab"]->sh_link=this->elf_exe_.getSegIndex(".symtab")+1;//。strtab默认在.symtab之后
	int strtabSize = 0; // 字符串表大小
	this->elf_exe_.addSym("",NULL); // 空符号表项
     // 遍历所有符号
	for(int i = 0; i < sym_def_.size(); ++i) {
		string name = sym_def_[i]->sym_name_;
		strtabSize += name.length()+1;
		Elf32_Sym *sym = sym_def_[i]->prov_->sym_tbl_[name];
		sym->st_shndx = this->elf_exe_.getSegIndex(sym_def_[i]->prov_->shdr_names_[sym->st_shndx]);//重定位后可以修改了
		this->elf_exe_.addSym(name,sym);
	}
	//记录程序入口
	this->elf_exe_.ehdr_.e_entry = this->elf_exe_.sym_tbl_["_start"]->st_value;//程序入口地址
	curOff += (1 + sym_def_.size()) * SYM_SIZE;//.strtab偏移
	this->elf_exe_.addShdr(".strtab", SHT_STRTAB, 0, 0, curOff, strtabSize, SHN_UNDEF, 0, 1, 0);//.strtab
	//填充strtab数据
	str=this->elf_exe_.strtab_ = new char[strtabSize];
	this->elf_exe_.strtabSize_ = strtabSize;
	index = 0;
	//串表与索引映射
	map<string, int> strIndex;
	strIndex[""] = strtabSize-1;
	for(int i = 0; i < sym_def_.size(); i++) {
		strIndex[sym_def_[i]->sym_name_] = index;
		strcpy(str + index, sym_def_[i]->sym_name_.c_str());
		index = index + sym_def_[i]->sym_name_.length() + 1;
	}
	//更新符号表name
	for(auto i: this->elf_exe_.sym_tbl_) {
		i.second->st_name = strIndex[i.first];
	}
	//更新段表name
	for(auto i: this->elf_exe_.shdr_tbl_) {
		i.second->sh_name = shstrIndex[i.first];
	}
}
// 输出elf
void Linker::writeExecFile(const char*dir)
{
	this->elf_exe_.writeElf(dir, ELF_WRITE_HEADER);//输出链接后的elf前半段
	//输出重要段数据
	FILE*fp = fopen(dir,"a+");
	char pad[1] = {0};
	for(int i = 0; i < this->seg_names_.size(); i++) {
		SegList* sl = this->seg_lists_[this->seg_names_[i]];
		int padnum = sl->offset_ - sl->begin_;
		while(padnum--) {
            // 填充
			fwrite(pad, 1, 1, fp);
        }
		//输出数据
		if(this->seg_names_[i] != ".bss") {
			Block*old=NULL;
			char instPad[1]={(char)0x90};
			for(int j=0;j<sl->blocks.size();++j) {
				Block*b=sl->blocks[j];
				if(old != NULL) {
                    // 填充小段内的空隙
					padnum=b->offset_- (old->offset_ + old->size_);
					while(padnum --) {
						fwrite(instPad,1,1,fp);//填充
                    }
				}
				old = b;
				fwrite(b->data_, b->size_, 1, fp);
			}
        }
	}
	fclose(fp);
	this->elf_exe_.writeElf(dir, ELF_WRITE_SECTIONS);//输出链接后的elf后半段
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