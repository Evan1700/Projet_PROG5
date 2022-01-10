#ifndef __SECTION_GENERATOR_H__
#define __SECTION_GENERATOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

Elf32_stct *section_container_summoner(void);

void section_container_destroyer(Elf32_stct *container);

void section_containers_display(Elf32_stct *sections);

void section_container_adder(Elf32_stct *head, char *content, char *name, int idx, int size);

void sections_list_destroyer(Elf32_stct *head);

#endif