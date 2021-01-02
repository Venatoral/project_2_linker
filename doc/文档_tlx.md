### 方案设计

基于前述的需求分析和可行性研究，我们可以设计出链接器的具体结构。

经过汇编器的处理，现在已经获得了多个目标文件。但由于可能涉及到对其目标文件的变量或函数的使用，且没有对各个段分配装载内存，它们并不能被直接运行，而是需要链接器对其进行链接和生成。

链接器将生成的多个可重定位目标文件拼接为一个可执行的ELF文件。同时，它并非机械地拼接目标文件，还需要完成汇编阶段无法完成的段地址分配、符号地址计算以及数据/指令内容修正的工作。这三个主要任务涉及了链接器工作的核心流程：地址空间分配、符号解析和重定位。在可重定位目标文件的段表项中，段的虚拟地址都是默认设为0。这是因为在汇编器处理阶段，是不可能知道段的加载地址的。链接器的地址空间分配操作的主要目的是为段指定加载地址。

在确定了段加载地址（简称段基址)后，根据目标文件内符号的段内偏移地址，可以计算得到符号的虚拟地址（简称符号地址）。链接器的符号解析操作并不止于计算符号地址，它还需要分析目标文件之间的符号引用的情况，计算目标文件内引用的外部符号的地址。符号解析之后，所有目标文件的符号地址都已经确定。链接器通过重定位操作，修正代码段或数据段内引用的符号地址。最后，链接器将以上操作处理后的文件信息导出为可执行ELF 文件，完成链接的工作。

基于上述讨论，链接器在实现时需要被分为四个部分：信息收集、地址空间分配、符号解析和重定位。

**1、信息收集**

在链接器进行地址分配和重定位等一系列操作之前，必须获取所有待链接的目标文件的信息，所以需要预先将所有待链接的目标文件读入内存中，并对其中的重要信息进行提取和分析。

首先需要建立ELF文件对象，保存扫描的目标文件信息，因此需要添加必要的ELF文件读取操作。链接器扫描目标文件时主要考虑三个表的信息：段表、符号表、重定位表的信息。其中段表用于指示该目标文件的所有段的基本信息，符号表用于标记该目标文件的所有导出符号，而最重要的重定位表则保存着各个段，尤其是代码段，的重定位信息，这是链接过程的信息来源。

对所有的ELF文件进行读入后，还需要对其进行简单的拼接，将不同目标文件的所有同名段进行合并，储存到同一个表中，为后续的地址空间分配过程做准备。值得注意的是，在段合并时需要考虑段内的对其，而不能简单地对段内容进行连接。

除了对目标文件的段数据进行重新组织,链接器还需要分析目标文件内符号的引用情况。之所以要分析符号的引用信息，是因为在链接器处理的目标文件中，存在未定义的符号，即对其他目标文件符号的引用。为了方便链接器符号解析的处理，一般会定义两个符号集合：一个是导出符号集合，表示所有目标文件内定义的可以被其他目标引用的全局符号集合；另一个是导入符号集合，表示所有目标文件未定义却引用其他目标文件的符号集合。

**2、空间地址分配**

经过信息收集过程后，所有目标文件的段都已经被读入到内存中，以表的形式组织起来。接下来就是对合并好的段进行空间地址的分配，用以确定每个段在运行时载入内存的地址。

链接器为段指定基址,需要从三个方面进行考虑。

首先是段加载的起始地址。该地址是所有加载段的起始位置，在32位Linux系统中，一般设置为0x08048000。

然后是段的拼接顺序。链接器按序扫描目标文件内同名的段，并将段的二进制数据依次摆放。在我们实现的链接器中，只需要按照代码段“.text”、数据段“.data”的顺序，依次处理每个目标文件内该类型的段即可。

最后还要考虑段对齐方式。段对齐包含两个层面:段文件偏移的对齐和段基址的对齐。在可重定位的目标文件内，一般将段的文件偏移对齐设置为4字节，不考虑段基址的对齐（段基址都是0，没有对齐的意义)。在可执行文件内，数据段和代码段的文件偏移对齐方式也为4字节，但段基址的对齐则比较复杂，需要保证段的线性地址与段对应文件偏移相对于段对齐值(即页面大小，Linux下默认为4096字节)取模相等。

**3、符号解析**

符号解析的主要目的是计算目标文件内符号的线性地址，即可执行文件被加载到进程内存空间之后符号的虚拟地址。目标文件符号表内保存了每个定义的符号相对于所在段基址的偏移，当段的地址空间分配结束后每个段的基址都被确定下来，因此符号地址可以利用段基址加上符号相对段基址的偏移来得到。

但在计算符号地址之前，仍需要做一些准备工作。首先需要扫描目标文件内的符号表，获取符号的定义与引用的信息，前面描述的导出符号集合和导入符号集合。其次，需要对导入符号集合和导出符号集合进行合法性验证。符号验证包含两个方面：符号重定义和符号未定义。一旦出现符号重定义或未定义的情况，链接器的工作就无法继续进行。因此，我们将符号解析分为两个阶段:符号引用验证和符号地址解析。只有符号引用验证通过后，才继续符号地址解析的流程。

**4、重定位**

由于重定位操作依赖于重定位符号的地址，因此在符号解析完成后才能进行重定位。同时，重定位符号及相关信息已经在收集ELF 文件信息时存入了重定位信息表中了

要进行重定位过程必须要知道重定位符号的位置，该符号的真实地址以及重定位的方式。其中符号的真实地址已经在上一步符号解析时被写入了符号表中，可以直接使用，下面分析剩余两个部分。

首先是重定位符号的位置。该地址可以根据重定位段以及重定位位置的段内偏移计算得出。公式描述为：

重定位位置=重定位段基址＋重定位位置的段内偏移

然后是重定位方式。ELF文件标准定义的重定位类型有两种：绝对地址重定位和相对地址重定位。根据不同的重定位类型对段数据进行修正操作是重定位的核心。

绝对地址重定位操作比较简单，需要绝对地址重定位的地方一般都是源于对符号地址的直接引用，由于汇编器不能确定符号的虚拟地址，最终使用0作为占位符填充了引用符号地址的地方。因此，绝对地址重定位操作只需要直接填写重定位符号的虚拟地址到重定位位置即可。

相对地址重定位稍微复杂一点，需要相对地址重定位的地方一般都是源于跳转类指令引用了其他文件的符号地址。虽然汇编器不能确定被引用符号的虚拟地址，但是并不使用0作为占位符填充引用符号地址的地方，而是使用“重定位位置相对于下一条指令地址的偏移”填充该位置。链接器进行相对地址重定位操作时，会计算符号地址相对于重定位位置的偏移，然后将该偏移量累加到重定位位置保存的内容。公式描述为：

相对重定位地址=重定位符号地址–重定位位置＋重定位位置数据内容

根据上面的计算，可以很清晰地看出最终计算的相对重定位地址正是符号地址相对于下一条指令地址的偏移，也正符合跳转类指令对操作数的要求。



### 方案实现

基于上述对链接器的方案设计，可以分析得到实现时所需的必要功能。

首先是存储ELF文件的类ElfFile，它被设计为如下所示的结构。

```c++
class ElfFile {
public:
    string elf_dir_;
    Elf32_Ehdr ehdr_;
    vector<Elf32_Phdr *> phdr_tbl_;
    unordered_map<string, Elf32_Shdr *> shdr_tbl_;
    vector<string> shdr_names_;
    unordered_map<string, Elf32_Sym *> sym_tbl_;
    vector<string> sym_names_;
    vector<RelItem *> rel_tbl_;
    char* shstrtab_;
    char* strtab_;
    unsigned int shstrtabSize_;
    unsigned int strtabSize_;
public:
    ElfFile(){};
    ElfFile(const char *file_dir_);
    void getData(char *buf, Elf32_Off offset, Elf32_Word size);
    int getSegIndex(string seg_name);
    int getSymIndex(string sym_name);
    void addPhdr(Elf32_Phdr *new_phdr);
    void addPhdr(Elf32_Word type,Elf32_Off off,Elf32_Addr vaddr,Elf32_Word filesz,
		Elf32_Word memsz,Elf32_Word flags,Elf32_Word align);
    void addShdr(string shdr_name, Elf32_Shdr *new_shdr);
    void addShdr(string sh_name,Elf32_Word sh_type,Elf32_Word sh_flags,Elf32_Addr sh_addr,Elf32_Off sh_offset,
			Elf32_Word sh_size,Elf32_Word sh_link,Elf32_Word sh_info,Elf32_Word sh_addralign,
			Elf32_Word sh_entsize);
    void addSym(string st_name, Elf32_Sym *);
    void writeElf(const char*dir,int flag);
    ~ElfFile();
};
```

从代码中可以看出ElfFile主要包括如下几个字段：

文件名称elf_dir表示每个ELF文件的名称，程序头表phdr_tbl 和段表shdr_tbl，它们分别存储程序头表和段表的相关信息，其中段表被设计为unordered_map，用以通过段名快速查找到其对应项。

然后是符号表sym_tbl，和段表一样，为了加速查找，它也被设计为unordered_map，在大量访问符号表的情况下提升性能。

最后是重定位表rel_tbl，它是由RelItem类型组成的一个数组。其中结构体RelItem被定义为：

```c++
struct RelItem {
public:
    string seg_name_;
    string rel_name_;
    Elf32_Rel *rel_;
public:
    RelItem(){};
    RelItem(string seg_name, string rel_name, Elf32_Rel* rel) {
        this->seg_name_ = seg_name;
        this->rel_name_ = rel_name;
        this->rel_ = rel;
    }
};
```

在ElfFile中，还有用以读入各项数据的public方法，通过在构造方法中的调用，可以实现对一个ELF文件的读入。



然后是执行链接过程的Linker类。

```c++
class Linker {
    vector<string> seg_names_;
    ElfFile elf_exe_;
    ElfFile *start_owner_;
public:
    vector<ElfFile *> elfs;
    unordered_map<string, SegList *> seg_lists_;
    vector<SymLink *> sym_ref_;
    vector<SymLink *> sym_def_;
public:
    Linker();
    void addElf(const char *dir);
    void collectInfo();
    void allocAddr();
    void parseSym();
    void relocate();
    void makeExec();
    void writeExecFile(const char *dir);
    void link(const char *dir);
    ~Linker();
};
```

从类中的各个方法的名称可以很明显的看出，代码执行了信息收集，分配地址空间，符号解析和重定位功能，这与之前方案设计时的分析一致。

首先是信息收集过程，该部分主要有两个函数构成：addElf()用以添加一个目标文件，collectInfo()用以搜集段信息和符号关联信息。

函数collectInfo()代码如下所示：

```c++
void Linker::collectInfo()
{
    for(int i = 0;i < elfs.size(); ++i) {
		ElfFile *elf = elfs[i];
		for(int i = 0; i < seg_names_.size(); i++) {
			if(elf->shdr_tbl_.find(seg_names_[i]) != elf->shdr_tbl_.end()) {
				this->seg_lists_[seg_names_[i]]->owner_list_.push_back(elf);
            }
        }
        for(auto symIt: elf->sym_tbl_) {
            SymLink* symLink = new SymLink();
            symLink->sym_name_ = symIt.first;
            if(symIt.second->st_shndx == STN_UNDEF) {
                symLink->recv_ = elf;
                symLink->prov_ = NULL;
                sym_ref_.push_back(symLink);
                printf("%s---未定义\n",symLink->sym_name_.c_str());
            } else {
                symLink->prov_ = elf;
                symLink->recv_ = NULL;
                sym_def_.push_back(symLink);
                printf("%s---定义\n",symLink->sym_name_.c_str());
            }
		}
	}
   	for(int i = 0; i < sym_ref_.size(); ++i) {
		for(int j = 0; j < sym_def_.size(); ++j) {
			if(ELF32_ST_BIND(sym_def_[j]->prov_->sym_tbl_[sym_def_[j]->sym_name_]->st_info) != STB_GLOBAL)
				continue;
			if(sym_ref_[i]->sym_name_ == sym_def_[j]->sym_name_) {
				sym_ref_[i]->prov_ = sym_def_[j]->prov_;
				sym_def_[j]->recv_ = sym_def_[j]->prov_;
			}
		}
		if(sym_ref_[i]->prov_ == NULL) {
            printf("%s undefined!\n", sym_ref_[i]->sym_name_.c_str());
            exit(EXIT_FAILURE);
        }
	}
}
```



然后是地址空间分配过程，该功能由函数allocAddr()来完成，实现分配地址空间，重新计算虚拟地址空间，并将其他的段全部删除的功能。

由于各段的信息需要合并存储，还需要定义适当的类SegList来存储相关信息。

类SegList的定义如下所示:

```c++
class SegList {
public:
    unsigned int baseAddr_;
    unsigned int begin_;
    unsigned int offset_;
    unsigned int size_;
    vector<ElfFile *> owner_list_;
    vector<Block *> blocks;
    void allocAddr(string name, unsigned int &base, unsigned int &off);
    void relocAddr(unsigned int rel_addr, unsigned char type, unsigned int sym_addr);
};
```

函数allocAddr()则可以直接调用类SegList的allocAddr方法，如下所示：

```c++
void SegList::allocAddr(string name, unsigned int &base, unsigned int &off)
{
    begin_ = off;
    if (name != ".bss") {
        base += (MEM_ALIGN - base % MEM_ALIGN) % MEM_ALIGN;
    }
    int align = DESC_ALIGN;
	if(name == ".text") {
		align = 8;
    }
    off += (align - off % align) % align;
    base = base - base % MEM_ALIGN + off % MEM_ALIGN;
    baseAddr_ = base;
    offset_ = off;
    size_ = 0;
    for (int i = 0; i < owner_list_.size(); ++i)
    {
        size_ += (DESC_ALIGN - size_ % DESC_ALIGN) % DESC_ALIGN;
        Elf32_Shdr *seg = owner_list_[i]->shdr_tbl_[name];
        if (name != ".bss") {
            char *buf = new char[seg->sh_size];
            owner_list_[i]->getData(buf, seg->sh_offset, seg->sh_size);

            Block *bloc = new Block();
            bloc->data_ = buf;
            bloc->offset_ = size_;
            bloc->size_ = seg->sh_size;
            blocks.push_back(bloc);
        }
        seg->sh_addr = base + size_;
        size_ += seg->sh_size;
    }
    base += size_;
    if (name != ".bss") {
        off += size_;
    }
}
```



然后是符号解析过程，该部分由函数parseSym()来完成，需要计算定义和未定义的符号虚拟地址。

在所有准备工作完成后，需要进行重定位，由函数relocate()完成，根据所有目标文件的重定位项修正符号地址。

函数relocate()的实现如下所示：

```c++
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
            unsigned char type = ELF32_R_TYPE(rel->rel_->r_info);
            seg_lists_[rel->seg_name_]->relocAddr(rel_addr, type, sym_addr);
        }
    }
}
```

其中调用的类SegList的成员方法relocAddr()的实现如下所示：

```c++
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
        int delta = sym_addr - rel_addr - 4;
        pdata++;
        *pdata = (unsigned char)(delta % 0x100);
        pdata++;
        *pdata = (unsigned char)((delta >> 8) % 0x100);
        pdata++;
        *pdata = (unsigned char)((delta >> 16) % 0x100);
    } else if (type == R_386_GLOB_DAT) {
        *(unsigned int *)pdata = sym_addr;
    } else if (type == R_386_32) {
		*pdata = sym_addr;
	} else if(type == R_386_PC32) {
		*pdata = sym_addr - rel_addr + (*pdata);
	} else if (type == R_ARM_ABS12) {}
}
```



执行完所有链接相关工作后，需要整合输出可执行文件。该部分由函数makeExec()来组装可执行文件，然后再利用函数writeExecFile(const char *dir)将组装好的可执行文件输出为ELF格式文件。

至此就可以完成链接的全部过程，得到可以运行的可执行文件了。























