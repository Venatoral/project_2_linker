#include <stdint.h>
#include <string>
#include <vector>
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
    Elf32_Addr r_offset;
    Elf32_Word r_info;
}Elf32_Rel;//add by nzb


typedef struct{
    Elf32_Half no;//该节区的序号
    std::string name;//该节区的名字
    Elf32_Word size;//该节区内容的长度(Byte)
    char *content;//该节区的内容 字符串首地址//TODO 可以换成更安全的结构
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
