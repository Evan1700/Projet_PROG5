#include "linker.h"

int main(int argc, char **argv)
{
  char *file_content = NULL;
  Elf32_file elf;

  file_content = load_file_content("loading.o");
  storage_elf_content(file_content, &elf);
}