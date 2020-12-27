#include "./inc/arm_linker.hpp"
#include <iostream>
#include <string.h>

// 构造函数，从file_dir_读取文件，构造ElfFile对象
ElfFile::ElfFile(const char* file_dir_) {
    this->elf_dir_ = string(file_dir_);
	FILE*fp = fopen(file_dir_, "rb");
    if(!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    rewind(fp);
    Elf32_Ehdr ehdr;
    // 读取文件头
	fread(&ehdr, sizeof(Elf32_Ehdr), 1, fp);
    this->ehdr_ = ehdr;
    // 读取段表字符串表项
	fseek(fp, ehdr.e_shoff + ehdr.e_shentsize * ehdr.e_shstrndx, 0);
	Elf32_Shdr shstrTab;
	fread(&shstrTab, ehdr.e_shentsize, 1, fp);
    // 读取段表字符串表
	char* shstrTabData = new char[shstrTab.sh_size];
	fseek(fp, shstrTab.sh_offset, 0);
	fread(shstrTabData, shstrTab.sh_size, 1, fp);

	fseek(fp, ehdr.e_shoff, 0);//段表位置
    // 读取sections
	for(int i = 0; i < ehdr.e_shnum; i++) {
		Elf32_Shdr *shdr = new Elf32_Shdr();
        // 读取段表项[非空]
		fread(shdr, ehdr.e_shentsize, 1, fp);
		string name(shstrTabData + shdr->sh_name);
		this->shdr_names_.push_back(name);
		if(name.empty()) {
        // 删除空段表项
			delete shdr;
        } else {
			this->shdr_tbl_[name] = shdr;
		}
	}
    // 清空段表字符串表
	delete []shstrTabData;
	Elf32_Shdr *strTab = this->shdr_tbl_[".strtab"];//字符串表信息
	char* strTabData = new char[strTab->sh_size];
    // 读取字符串表
	fseek(fp, strTab->sh_offset, 0);
	fread(strTabData, strTab->sh_size, 1, fp);
    // 符号表信息
	Elf32_Shdr *sh_symTab = this->shdr_tbl_[".symtab"];
	fseek(fp, sh_symTab->sh_offset, 0);
	int symNum = sh_symTab->sh_size / sh_symTab->sh_entsize;
    // 按照序列记录符号表所有信息，方便重定位符号查询
	vector<Elf32_Sym*> symList;
    // 读取符号 
	for(int i = 0; i < symNum; i++){
		Elf32_Sym *sym = new Elf32_Sym();
		fread(sym, sizeof(Elf32_Sym), 1, fp);// 读取符号项[非空]
		symList.push_back(sym);//添加到符号序列
		string name(strTabData + sym->st_name);
        // 无名符号，对于链接没有意义,按照链接器设计需要记录全局和局部符号，避免名字冲突
		if(name.empty() || name.size() == 0) {
			delete sym;//删除空符号项
        } else {
			this->sym_tbl_[name] = sym;//加入符号表
		}
	}
    // 所有段的重定位项整合
	for(auto tbl: this->shdr_tbl_) {
        // 重定位段 
		if(tbl.first.find(".rel") == 0) {
            // 重定位表信息
			Elf32_Shdr *sh_relTab = this->shdr_tbl_[tbl.first];
			fseek(fp, sh_relTab->sh_offset, 0);
			int relNum = sh_relTab->sh_size / sh_relTab->sh_entsize;
			for(int j = 0; j < relNum; j++) {
				Elf32_Rel * rel = new Elf32_Rel();
                // 读取重定位项
				fread(rel, sizeof(Elf32_Rel), 1, fp);
                // 获得重定位符号名字
				string name(strTabData + symList[ELF32_R_SYM(rel->r_info)]->st_name);
				//使用shdrNames[sh_relTab->sh_info]访问目标段更标准
				this->rel_tbl_.push_back(new RelItem(tbl.first.substr(4), name, rel));
			}
		}
	}
	delete []strTabData;
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


void ElfFile::addPhdr(Elf32_Word type,Elf32_Off off,Elf32_Addr vaddr,Elf32_Word filesz,
		Elf32_Word memsz,Elf32_Word flags,Elf32_Word align) {
	Elf32_Phdr*ph=new Elf32_Phdr();
	ph->p_type=type;
	ph->p_offset=off;
	ph->p_vaddr=ph->p_paddr=vaddr;
	ph->p_filesz=filesz;
	ph->p_memsz=memsz;
	ph->p_flags=flags;
	ph->p_align=align;
	phdr_tbl_.push_back(ph);
}
// 添加一个段表项
void ElfFile::addShdr(string shdr_name, Elf32_Shdr *new_shdr) {
    this->shdr_names_.push_back(shdr_name);
    this->shdr_tbl_[shdr_name] = new_shdr;
}

void ElfFile::addShdr(string sh_name,Elf32_Word sh_type,Elf32_Word sh_flags,Elf32_Addr sh_addr,Elf32_Off sh_offset,
			Elf32_Word sh_size,Elf32_Word sh_link,Elf32_Word sh_info,Elf32_Word sh_addralign,
			Elf32_Word sh_entsize)
{
	Elf32_Shdr*sh=new Elf32_Shdr();
	sh->sh_name=0;
	sh->sh_type=sh_type;
	sh->sh_flags=sh_flags;
	sh->sh_addr=sh_addr;
	sh->sh_offset=sh_offset;
	sh->sh_size=sh_size;
	sh->sh_link=sh_link;
	sh->sh_info=sh_info;
	sh->sh_addralign=sh_addralign;
	sh->sh_entsize=sh_entsize;
	shdr_tbl_[sh_name]=sh;
	shdr_names_.push_back(sh_name);
}
// 添加一个符号表项
void ElfFile::addSym(string st_name, Elf32_Sym *new_sym) {
	Elf32_Sym* sym = sym_tbl_[st_name] = new Elf32_Sym();
	if(st_name=="") {
		sym->st_name = 0;
		sym->st_value = 0;
		sym->st_size = 0;
		sym->st_info = 0;
		sym->st_other = 0;
		sym->st_shndx = 0;
	} else {
		sym->st_name=0;
		sym->st_value = new_sym->st_value;
		sym->st_size = new_sym->st_size;
		sym->st_info = new_sym->st_info;
		sym->st_other = new_sym->st_other;
		sym->st_shndx = new_sym->st_shndx;
	}
	sym_names_.push_back(st_name);
}

void ElfFile::writeElf(const char*dir,int flag) {
	if(flag == ELF_WRITE_HEADER) {
		FILE* fp = fopen(dir, "w+");
		fwrite(&ehdr_, ehdr_.e_ehsize, 1, fp);
        // 程序头表
		if(!phdr_tbl_.empty()) {
			for(int i = 0; i < phdr_tbl_.size(); i++) {
				fwrite(phdr_tbl_[i], ehdr_.e_phentsize, 1, fp);
            }
		}
		fclose(fp);
	} else if(flag == ELF_WRITE_SECTIONS) {
		FILE*fp = fopen(dir, "a+");
        // .shstrtab
		fwrite(shstrtab_, shstrtabSize_, 1, fp);
        // 段表
		for(int i = 0; i < shdr_names_.size(); i++) {
			Elf32_Shdr* sh = shdr_tbl_[shdr_names_[i]];
			fwrite(sh, ehdr_.e_shentsize, 1, fp);
		}
        // 符号表
		for(int i = 0; i < sym_names_.size(); i++) {
			Elf32_Sym* sym = sym_tbl_[sym_names_[i]];
			fwrite(sym, sizeof(Elf32_Sym), 1, fp);
		}
        // .strtab
		fwrite(strtab_, strtabSize_, 1, fp);
		fclose(fp);
	}
}