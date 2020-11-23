#include <iostream>
#include <map>

using namespace std;

#define NOTYPE -1
#define FUNCTION 0
#define GLOBAL_VAR 1
#define LABLE 2
#define PREFIX_LEN 6
void type_handler(string arm)
{
    static map<string, int> typeHandleMap = {
        {"function", FUNCTION},
        {"object", GLOBAL_VAR},
    };
    string name = "";
    for (auto ch : arm.substr(PREFIX_LEN))
        if ((ch <= 'z' && ch >= 'a') || (ch <= 'Z' && ch >= 'A') || (ch <= '9' && ch >= '0') || ch == '_')
            name += ch;
        else
            break;
    string type = arm.substr(arm.find_first_of('%') + 1);
    cout << name << "\t" << type << endl;
}
#undef FUNCTION
#undef GLOBAL_VAR
#undef NOTYPE
#undef LABLE
#undef PREFIX_LEN

int main(int argc, const char** argv) {
    type_handler(".type	main, \%function");
    return 0;
}