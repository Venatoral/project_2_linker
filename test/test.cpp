#include <elf.h>
#include <stdio.h>

int main() {
    printf("Ehdr: %d\n", sizeof(Elf32_Ehdr));
    printf("Phdr: %d\n", sizeof(Elf32_Phdr));
    printf("Shdr: %d\n", sizeof(Elf32_Shdr));
    printf("Sym: %d\n", sizeof(Elf32_Sym));
    printf("Rel: %d\n", sizeof(Elf32_Rel));
    return 0;
}