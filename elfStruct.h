#include <stdint.h>
#include <string>
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef	int32_t  Elf32_Sword;
typedef uint64_t Elf32_Xword;
typedef	int64_t  Elf32_Sxword;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Section;
typedef Elf32_Half Elf32_Versym;

#define EI_NIDENT (16)
typedef struct{
    unsigned char e_ident[EI_NIDENT];	/* Magic number and other info */
    Elf32_Half e_type;			/* Object file type */
    Elf32_Half e_machine;		/* Architecture */
    Elf32_Word e_version;		/* Object file version */
    Elf32_Addr e_entry;		/* Entry point virtual address */
    Elf32_Off e_phoff;		/* Program header table file offset */
    Elf32_Off e_shoff;		/* Section header table file offset */
    Elf32_Word e_flags;		/* Processor-specific flags */
    Elf32_Half e_ehsize;		/* ELF header size in bytes */
    Elf32_Half e_phentsize;		/* Program header table entry size */
    Elf32_Half e_phnum;		/* Program header table entry count */
    Elf32_Half e_shentsize;		/* Section header table entry size */
    Elf32_Half e_shnum;		/* Section header table entry count */
    Elf32_Half e_shstrndx;		/* Section header string table index */
}Elf32_Ehdr;

typedef struct{
    Elf32_Word sh_name;		/* Section name (string tbl index) */
    Elf32_Word sh_type;		/* Section type */
    Elf32_Word sh_flags;		/* Section flags */
    Elf32_Addr sh_addr;		/* Section virtual addr at execution */
    Elf32_Off sh_offset;		/* Section file offset */
    Elf32_Word sh_size;		/* Section size in bytes */
    Elf32_Word sh_link;		/* Link to another section */
    Elf32_Word sh_info;		/* Additional section information */
    Elf32_Word sh_addralign;		/* Section alignment */
    Elf32_Word sh_entsize;		/* Entry size if section holds table */
}Elf32_Shdr;

typedef struct{
    Elf32_Half no;//该节区的序号
    string name;//该节区的名字
    Elf32_Word size;//该节区内容的长度(Byte)
    char *content;//该节区的内容 字符串首地址//TODO 可以换成更安全的结构
}SectionInfo;

//示例
typedef struct{
    //just example
}SectionBss;


/*TODO yrc struct*/
typedef struct{
    //TODO
}symbols;

typedef struct{
    //TODO
}reloc_symbol;

typedef struct{
    //TODO
}arm_assem;

typedef struct{
    //TODO
}data_element;

typedef struct{
    //TODO
}bss_element;