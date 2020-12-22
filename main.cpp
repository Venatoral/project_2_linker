#include "./inc/arm_linker.hpp"
#include "./inc/gen_reloc.hpp"
#include "./inc/arm_analyze.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;
string output_file;
vector<string> source_file_list;
vector<string> object_file_list;
bool compile_only = false;
bool debug = true;

// 替换后缀 a.s => a.o
string replace_suffix(string file_name, string new_suffix) {
    if(debug) {
        printf("[Replace_Suffix] Before: %s\n", file_name.c_str());
    }
    int dot_pos = file_name.find_last_of('.');
    string new_name = file_name.substr(0, dot_pos + 1) + new_suffix;
    if(debug) {
        printf("[Replace_Suffix] After: %s\n", new_name.c_str());
    }
    return new_name;
}

// 获得后缀名
string get_suffix(string& file_name) {
    if(debug) {
        printf("[Get_Suffix] %s\n", file_name.substr(file_name.find_last_of('.')).c_str());
    }
    return file_name.substr(file_name.find_last_of('.'));
}

// 解析命令行参数
void parse_commnad(int argc, char* argv[]) {
    for(int i = 1; i < argc; i++) {
        // 命令参数 -c 只编译 -o 指定输出文件
        if(argv[i][0] == '-') {
            if(argv[i][1] == 'c') {
                if(debug) {
                    printf("Program: compile only\n");
                }
                compile_only = true;
            } else if(argv[i][1] == 'o') {
                // 试探性获取输出文件名称
                try {
                    output_file = string(argv[i + 1]);
                    if(debug) {
                        printf("Output: %s\n", output_file.c_str());
                    }
                    i ++;
                } catch(exception e) {
                    output_file = "a.out";
                    if(debug) {
                        printf("Catch exception, output to a.out\n");
                        break;
                    }
                }
            }
            continue;
        }
        // 处理文件名称，支持 .o 和 .s 文件
        string file_name = string(argv[i]);
        string suffix = get_suffix(file_name);
        if(suffix == ".o") {
            object_file_list.push_back(file_name);
        } else if (suffix == ".s") {
            source_file_list.push_back(file_name);
        } else {
            printf("\033[;41mIllegal file: %s\n", file_name.c_str());
            return;
        }
    }
}
string file_to_string(const char* file_name) {
    ifstream fi(file_name);
    //将文件读入到ostringstream对象buf中
    ostringstream buf;
    char ch;
    while (buf && fi.get(ch))
        buf.put(ch);
    //返回与流对象buf关联的字符串
    return buf.str();
}
// 将汇编源文件编译成二进制文件
void compile() {
    printf("开始进行编译\n");
    int source_file_num = source_file_list.size();
    for(int i = 0; i < source_file_num; i ++) {
        ARM_analyze arm_expert;
        // 读取文件
        string content = file_to_string(source_file_list[i].c_str());
        arm_expert.arm_analyze(content);
        // 更改后缀为 o
        string object_file = replace_suffix(source_file_list[i], "o");
        // o 文件还需要加入到 object_file_list 中可能用于之后的链接操作
        object_file_list.push_back(object_file);
        // 产生对应 object 文件
        RelocatableFile reloc_master = RelocatableFile(object_file);
        reloc_master.genRelocFile();
        // 清理ARM_analyze中各种 static的 vector，防止不同文件相互影响
        arm_expert.clear();
    }
    printf("编译完成\n");
}
// 链接
void link() {
    printf("开始进行链接操作\n");
    int object_file_num = object_file_list.size();
    Linker link_master;
    for(int i = 0; i < object_file_num; i++) {
        // 将要链接的文件各个加入到连接器中
        link_master.addElf(object_file_list[i].c_str());
    }
    // 产生可执行文件 output_file
    link_master.link(output_file.c_str());
    printf("链接结束\n");
}
int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Usage: ./prog [source files] [-c]|[-o output_file]");
        exit(EXIT_FAILURE);
    }
    // 解析命令参数
    parse_commnad(argc, argv);
    compile();
    // 如果只是编译则结束
    if(compile_only) {
        return 0;
    }
    // 编译之后进行链接操作，产生output_file可执行文件
    link();
    return 0;
}