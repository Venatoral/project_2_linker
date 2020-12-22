#ifndef ELF_STRUCT_H
#define ELF_STRUCT_H
#include "./elf_struct.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <elf.h>
// typedef uint16_t Elf32_Half;
// typedef uint32_t Elf32_Word;
// typedef	int32_t  Elf32_Sword;
// typedef uint64_t Elf32_Xword;
// typedef	int64_t  Elf32_Sxword;
// typedef uint32_t Elf32_Addr;
// typedef uint32_t Elf32_Off;
// typedef uint16_t Elf32_Section;
// typedef Elf32_Half Elf32_Versym;

// #define EI_NIDENT (16)
// typedef struct{
//     unsigned char e_ident[EI_NIDENT];	/* Magic number and other info */
//     Elf32_Half e_type;			/* Object file type */
//     Elf32_Half e_machine;		/* Architecture */
//     Elf32_Word e_version;		/* Object file version */
//     Elf32_Addr e_entry;		/* Entry point virtual address */
//     Elf32_Off e_phoff;		/* Program header table file offset */
//     Elf32_Off e_shoff;		/* Section header table file offset */
//     Elf32_Word e_flags;		/* Processor-specific flags */
//     Elf32_Half e_ehsize;		/* ELF header size in bytes */
//     Elf32_Half e_phentsize;		/* Program header table entry size */
//     Elf32_Half e_phnum;		/* Program header table entry count */
//     Elf32_Half e_shentsize;		/* Section header table entry size */
//     Elf32_Half e_shnum;		/* Section header table entry count */
//     Elf32_Half e_shstrndx;		/* Section header string table index */
// }Elf32_Ehdr;

// typedef struct{
//     Elf32_Word sh_name;		/* Section name (string tbl index) */
//     Elf32_Word sh_type;		/* Section type */
//     Elf32_Word sh_flags;		/* Section flags */
//     Elf32_Addr sh_addr;		/* Section virtual addr at execution */
//     Elf32_Off sh_offset;		/* Section file offset */
//     Elf32_Word sh_size;		/* Section size in bytes */
//     Elf32_Word sh_link;		/* Link to another section */
//     Elf32_Word sh_info;		/* Additional section information */
//     Elf32_Word sh_addralign;		/* Section alignment */
//     Elf32_Word sh_entsize;		/* Entry size if section holds table */
// }Elf32_Shdr;

// typedef struct{
//     Elf32_Addr r_offset;
//     Elf32_Word r_info;
// }Elf32_Rel;//add by nzb

// typedef struct{
//     Elf32_Word	st_name;		/* Symbol name (string tbl index) */
//     Elf32_Addr	st_value;		/* Symbol value */
//     Elf32_Word	st_size;		/* Symbol size */
//     unsigned char	st_info;		/* Symbol type and binding */
//     unsigned char	st_other;		/* Symbol visibility */
//     Elf32_Section	st_shndx;		/* Section index */
// }Elf32_Sym;

// typedef struct
// {
//   Elf32_Word	p_type;			/* Segment type */
//   Elf32_Off	p_offset;		/* Segment file offset */
//   Elf32_Addr	p_vaddr;		/* Segment virtual address */
//   Elf32_Addr	p_paddr;		/* Segment physical address */
//   Elf32_Word	p_filesz;		/* Segment size in file */
//   Elf32_Word	p_memsz;		/* Segment size in memory */
//   Elf32_Word	p_flags;		/* Segment flags */
//   Elf32_Word	p_align;		/* Segment alignment */
// } Elf32_Phdr;


typedef struct{
    Elf32_Half no;//该节区的序号
    std::string name;//该节区的名字
    Elf32_Word size;//该节区内容的长度(Byte)
    char *content;//该节区的内容 字符串首地址
}SectionInfo;

//示例
typedef struct{
    //just example
}SectionBss;


/*TODO yrc struct*/
typedef struct{
    int type;     //区分是函数还是全局变量, -1表示NOTYPE(未定义) 0表示函数, 1表示全局变量, 2表示标号
    bool defined; //判断该符号是否在此文件中定义过
    std::string name;  //符号名称
    int value;    //如果是文件中定义的函数 或 标号 会记录函数定义的位置和标号的位置 如果是全局变量在data段中的偏移
    int bind;     //判断符号的作用域是global还是local   0表示global 1表示local
}symbols;

typedef struct{
    int type;    //需要重定位的是函数还是全局变量
    std::string name; //符号名称
    int value;    //需要重定位的地方在.text中的偏移
                  //！！！当为 type = 0 时，value 为该跳转语句的偏移，需要再往后移动几个bit才是重定位地址！！！
}reloc_symbol;

typedef struct{
    std::string op_name; //操作符名称 "mov" "ldr" "str"等
    std::string Operands1;   //操作数
    std::string Operands2;
    std::string Operands3;
    std::vector<std::string> reglist; //如果操作符是push或pop  需要一个寄存器列表
}arm_assem;

typedef struct{
    std::string op_name; //数据声明语句 .word或.space
    int value;      //声明数据的值
}data_element;

typedef struct{
    std::string name;    //未定义的变量的名称
}bss_element;

//！！！以下放elf.h原有的宏定义！！！
/* Fields in the e_ident array.  The EI_* macros are indices into the
   array.  The macros under each EI_* macro are the values the byte
   may have.  */

#define EI_MAG0		0		/* File identification byte 0 index */
#define ELFMAG0		0x7f		/* Magic number byte 0 */

#define EI_MAG1		1		/* File identification byte 1 index */
#define ELFMAG1		'E'		/* Magic number byte 1 */

#define EI_MAG2		2		/* File identification byte 2 index */
#define ELFMAG2		'L'		/* Magic number byte 2 */

#define EI_MAG3		3		/* File identification byte 3 index */
#define ELFMAG3		'F'		/* Magic number byte 3 */

/* Conglomeration of the identification bytes, for easy testing as a word.  */
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

#define EI_CLASS	4		/* File class byte index */
#define ELFCLASSNONE	0		/* Invalid class */
#define ELFCLASS32	1		/* 32-bit objects */
#define ELFCLASS64	2		/* 64-bit objects */
#define ELFCLASSNUM	3

#define EI_DATA		5		/* Data encoding byte index */
#define ELFDATANONE	0		/* Invalid data encoding */
#define ELFDATA2LSB	1		/* 2's complement, little endian */
#define ELFDATA2MSB	2		/* 2's complement, big endian */
#define ELFDATANUM	3

#define EI_VERSION	6		/* File version byte index */
					/* Value must be EV_CURRENT */

#define EI_OSABI	7		/* OS ABI identification */
#define ELFOSABI_NONE		0	/* UNIX System V ABI */
#define ELFOSABI_SYSV		0	/* Alias.  */
#define ELFOSABI_HPUX		1	/* HP-UX */
#define ELFOSABI_NETBSD		2	/* NetBSD.  */
#define ELFOSABI_GNU		3	/* Object uses GNU ELF extensions.  */
#define ELFOSABI_LINUX		ELFOSABI_GNU /* Compatibility alias.  */
#define ELFOSABI_SOLARIS	6	/* Sun Solaris.  */
#define ELFOSABI_AIX		7	/* IBM AIX.  */
#define ELFOSABI_IRIX		8	/* SGI Irix.  */
#define ELFOSABI_FREEBSD	9	/* FreeBSD.  */
#define ELFOSABI_TRU64		10	/* Compaq TRU64 UNIX.  */
#define ELFOSABI_MODESTO	11	/* Novell Modesto.  */
#define ELFOSABI_OPENBSD	12	/* OpenBSD.  */
#define ELFOSABI_ARM_AEABI	64	/* ARM EABI */
#define ELFOSABI_ARM		97	/* ARM */
#define ELFOSABI_STANDALONE	255	/* Standalone (embedded) application */

#define EI_ABIVERSION	8		/* ABI version */

#define EI_PAD		9		/* Byte index of padding bytes */

/* Legal values for e_type (object file type).  */

#define ET_NONE		0		/* No file type */
#define ET_REL		1		/* Relocatable file */
#define ET_EXEC		2		/* Executable file */
#define ET_DYN		3		/* Shared object file */
#define ET_CORE		4		/* Core file */
#define	ET_NUM		5		/* Number of defined types */
#define ET_LOOS		0xfe00		/* OS-specific range start */
#define ET_HIOS		0xfeff		/* OS-specific range end */
#define ET_LOPROC	0xff00		/* Processor-specific range start */
#define ET_HIPROC	0xffff		/* Processor-specific range end */

/* Legal values for e_version (version).  */

#define EV_NONE		0		/* Invalid ELF version */
#define EV_CURRENT	1		/* Current version */
#define EV_NUM		2

/* Legal values for sh_type (section type).  */

#define SHT_NULL	  0		/* Section header table entry unused */
#define SHT_PROGBITS	  1		/* Program data */
#define SHT_SYMTAB	  2		/* Symbol table */
#define SHT_STRTAB	  3		/* String table */
#define SHT_RELA	  4		/* Relocation entries with addends */
#define SHT_HASH	  5		/* Symbol hash table */
#define SHT_DYNAMIC	  6		/* Dynamic linking information */
#define SHT_NOTE	  7		/* Notes */
#define SHT_NOBITS	  8		/* Program space with no data (bss) */
#define SHT_REL		  9		/* Relocation entries, no addends */
#define SHT_SHLIB	  10		/* Reserved */
#define SHT_DYNSYM	  11		/* Dynamic linker symbol table */
#define SHT_INIT_ARRAY	  14		/* Array of constructors */
#define SHT_FINI_ARRAY	  15		/* Array of destructors */
#define SHT_PREINIT_ARRAY 16		/* Array of pre-constructors */
#define SHT_GROUP	  17		/* Section group */
#define SHT_SYMTAB_SHNDX  18		/* Extended section indeces */
#define	SHT_NUM		  19		/* Number of defined types.  */
#define SHT_LOOS	  0x60000000	/* Start OS-specific.  */
#define SHT_GNU_ATTRIBUTES 0x6ffffff5	/* Object attributes.  */
#define SHT_GNU_HASH	  0x6ffffff6	/* GNU-style hash table.  */
#define SHT_GNU_LIBLIST	  0x6ffffff7	/* Prelink library list */
#define SHT_CHECKSUM	  0x6ffffff8	/* Checksum for DSO content.  */
#define SHT_LOSUNW	  0x6ffffffa	/* Sun-specific low bound.  */
#define SHT_SUNW_move	  0x6ffffffa
#define SHT_SUNW_COMDAT   0x6ffffffb
#define SHT_SUNW_syminfo  0x6ffffffc
#define SHT_GNU_verdef	  0x6ffffffd	/* Version definition section.  */
#define SHT_GNU_verneed	  0x6ffffffe	/* Version needs section.  */
#define SHT_GNU_versym	  0x6fffffff	/* Version symbol table.  */
#define SHT_HISUNW	  0x6fffffff	/* Sun-specific high bound.  */
#define SHT_HIOS	  0x6fffffff	/* End OS-specific type */
#define SHT_LOPROC	  0x70000000	/* Start of processor-specific */
#define SHT_HIPROC	  0x7fffffff	/* End of processor-specific */
#define SHT_LOUSER	  0x80000000	/* Start of application-specific */
#define SHT_HIUSER	  0x8fffffff	/* End of application-specific */

/* Legal values for sh_flags (section flags).  */

#define SHF_WRITE	     (1 << 0)	/* Writable */
#define SHF_ALLOC	     (1 << 1)	/* Occupies memory during execution */
#define SHF_EXECINSTR	     (1 << 2)	/* Executable */
#define SHF_MERGE	     (1 << 4)	/* Might be merged */
#define SHF_STRINGS	     (1 << 5)	/* Contains nul-terminated strings */
#define SHF_INFO_LINK	     (1 << 6)	/* `sh_info' contains SHT index */
#define SHF_LINK_ORDER	     (1 << 7)	/* Preserve order after combining */
#define SHF_OS_NONCONFORMING (1 << 8)	/* Non-standard OS specific handling required */
#define SHF_GROUP	     (1 << 9)	/* Section is member of a group.  */
#define SHF_TLS		     (1 << 10)	/* Section hold thread-local data.  */
#define SHF_COMPRESSED	     (1 << 11)	/* Section with compressed data. */
#define SHF_MASKOS	     0x0ff00000	/* OS-specific.  */
#define SHF_MASKPROC	     0xf0000000	/* Processor-specific */
#define SHF_ORDERED	     (1 << 30)	/* Special ordering requirement (Solaris).  */
#define SHF_EXCLUDE	     (1U << 31)	/* Section is excluded unless referenced or allocated (Solaris).*/


#endif