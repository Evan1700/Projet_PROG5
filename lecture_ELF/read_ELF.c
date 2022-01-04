#include "read_ELF.h"

char **options_read(int argc, char **argv, Exec_options *exec_op, hexdump_option *hexdump)
{
  char **files = NULL;
  const char *const short_options = "haHSx:";
  // Lecture des arguments
  while (1)
  {
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"all", no_argument, 0, 'a'},
        {"file-header", no_argument, 0, 'H'},
        {"section-headers", no_argument, 0, 'S'},
        {"sections", no_argument, 0, 'S'},
        {"hex-dump", required_argument, 0, 'x'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, short_options, long_options, &option_index);

    if (c == -1)
      break;

    switch (c)
    {
    case 'h': // help display
      free(files);
      print_usage(stdout, EXIT_SUCCESS, argv[0]);

    case 'a': // all informations display
      exec_op->header = true;
      exec_op->section_headers = true;
      break;

    case 'H': // Header display
      exec_op->header = true;
      break;

    case 'S': // Section headers display
      exec_op->section_headers = true;
      break;

    case 'x': // display a section
      int tmp;
      if (sscanf(optarg, "%d", &tmp) == 1) // it's integer
      {
        hexdump->is_string = true;
        hexdump->section_number = atoi(optarg);
      }
      else // it's string
      {
        hexdump->is_string = false;
        hexdump->section_name = calloc(strlen(optarg) + 1, sizeof(char));
        strcpy(hexdump->section_name, optarg);
      }
      exec_op->hexdump = true;
      break;

    case '?': // error usage
      print_usage(stderr, EXIT_FAILURE, argv[0]);

    default: // Error
      exit(EXIT_FAILURE);
    }
  }

  if (optind == argc) // If there are no given files
  {
    print_usage(stderr, EXIT_FAILURE, argv[0]);
  }
  else if (!exec_op->header && !exec_op->section_headers && !exec_op->hexdump)
  {
    print_usage(stderr, EXIT_FAILURE, argv[0]);
  }
  else if (optind == argc - 1) // If there is only one file given
  {
    exec_op->nb_files = 1;
    files = calloc(1, sizeof(char *));                         // Allocation of the array to a single element to store the file name
    files[0] = calloc(strlen(argv[optind]) + 1, sizeof(char)); // Allocation to store the file path
    strcpy(files[0], argv[optind]);                            // Storing the file path
  }
  else // if there are several files given
  {
    exec_op->nb_files = argc - optind;                 // Number of file paths given
    files = calloc(exec_op->nb_files, sizeof(char *)); // Multi-element array allocation to store file paths.
    for (int i = 0; i < exec_op->nb_files; i++)
    {
      files[i] = calloc(strlen(argv[optind + i]) + 1, sizeof(char)); // Allocation to store the file path
      strcpy(files[i], argv[optind + i]);                            // Storing the file path
    }
  }
  return files;
}

char **init_execution(int argc, char **argv, Exec_options *exec_op, hexdump_option *hexdump)
{
  exec_op->header = false;
  exec_op->section_headers = false;
  exec_op->big_endian_file = false;
  exec_op->hexdump = true;

  hexdump->is_string = false;

  return options_read(argc, argv, exec_op, hexdump);
}

void header_read(Elf32_Ehdr *ehdr, FILE *filename)
{
  fread(ehdr, 1, sizeof(Elf32_Ehdr), filename);
}

Elf32_Shdr_named *section_headers_read(Exec_options *exec_op, FILE *filename, Elf32_Ehdr *ehdr)
{
  Elf32_Shdr_named *shdr_named = calloc(1, sizeof(Elf32_Shdr_named));
  shdr_named->shnum = ehdr->e_shnum;
  int last_entry = ehdr->e_shnum - 1;
  char *Section_Names;

  // Allocation
  shdr_named->shdr = calloc(shdr_named->shnum, sizeof(Elf32_Shdr));
  shdr_named->names = calloc(shdr_named->shnum, sizeof(char *));

  // Reading of the last entry
  fseek(filename, ehdr->e_shoff + last_entry * sizeof(Elf32_Shdr), SEEK_SET);
  fread(&shdr_named->shdr[last_entry], 1, sizeof(Elf32_Shdr), filename);

  // endianess
  if (exec_op->big_endian_file)
    section_headers_endianess(&shdr_named->shdr[last_entry]);

  // Allocation
  Section_Names = calloc(1, shdr_named->shdr[last_entry].sh_size);

  // Recovering the names of the section headers
  fseek(filename, shdr_named->shdr[last_entry].sh_offset, SEEK_SET);
  fread(Section_Names, 1, shdr_named->shdr[last_entry].sh_size, filename);

  // Reading section headers
  for (int i = 0; i < shdr_named->shnum; i++)
  {
    fseek(filename, ehdr->e_shoff + i * sizeof(Elf32_Shdr), SEEK_SET);
    fread(&shdr_named->shdr[i], 1, sizeof(Elf32_Shdr), filename);

    if (exec_op->big_endian_file)
      section_headers_endianess(&shdr_named->shdr[i]);

    // Association of the name with the header
    if (shdr_named->shdr[i].sh_name)
    {
      char *name = "";
      name = Section_Names + shdr_named->shdr[i].sh_name;
      shdr_named->names[i] = calloc(strlen(name) + 1, sizeof(char));
      strcpy(shdr_named->names[i], name);
    }
  }
  free(Section_Names);
  return shdr_named;
}

void free_shdr_named(Elf32_Shdr_named *shdr_named)
{
  // Free'd all inside allocation of the structure
  for (int i = 0; i < shdr_named->shnum; i++)
  {
    free(shdr_named->names[i]);
  }
  free(shdr_named->names);
  free(shdr_named->shdr);
  free(shdr_named);
}

void run(Exec_options *exec_op, char *files[])
{
  FILE *filename = NULL;
  Elf32_Ehdr ehdr; // File header informations structure

  for (int i = 0; i < exec_op->nb_files; i++)
  {
    filename = fopen(files[i], "rb"); // Opening the file for binary read
    // Checking file openned
    if (filename == NULL)
    {
      if (exec_op->nb_files == 1) // if only one file
      {
        printf("read-elf: Error: '%s': No such file\n", files[0]);
        free(files[i]);
        free(files);
        exit(EXIT_FAILURE);
      }
      else // if there are several files
      {
        printf("read-elf: Error: '%s': No such file\n", files[i]);
      }
    }
    else
    {
      // if there are several files to read
      if (exec_op->nb_files > 1)
        printf("File: %s\n", files[i]);

      // READING HEADER
      header_read(&ehdr, filename);

      // Detection of big or little endian
      if (ehdr.e_ident[EI_DATA] == ELFDATA2MSB)
      {
        exec_op->big_endian_file = true;
        header_endianess(&ehdr);
      }

      // Display informations
      if (exec_op->header)
      {
        print_entete(&ehdr); // Print file header
      }

      if (exec_op->section_headers && ehdr.e_shnum > 0)
      {
        if (!exec_op->header)
        {
          printf("There are %d section header, starting at offset 0x%x:\n", ehdr.e_shnum, ehdr.e_shoff);
        }

        // Reading section headers
        Elf32_Shdr_named *shdr_named = section_headers_read(exec_op, filename, &ehdr);

        // Display section headers
        print_section_headers(shdr_named);

        free_shdr_named(shdr_named);
      }
      else if (exec_op->section_headers && ehdr.e_shnum == 0)
      {
        printf("There is no section in this file !\n");
      }

      // Closing file
      fclose(filename);

      // Conditions for displaying multiple files
      if (exec_op->nb_files > 1)
        printf("\n");

    }
    free(files[i]);
  }
}

int main(int argc, char *argv[])
{
  Exec_options exec_op;
  hexdump_option hexdump;

  // Checking the minimum number of arguments
  if (argc < 2)
    print_usage(stderr, EXIT_FAILURE, argv[0]);

  // Input detections
  char **files = init_execution(argc, argv, &exec_op, &hexdump);

  // Execution
  run(&exec_op, files);

  if (!hexdump.is_string)
    free(hexdump.section_name);
  free(files);
  return 0;
}
