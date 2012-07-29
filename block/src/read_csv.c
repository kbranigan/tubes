
#include "block.h"
#include "bsv.c"

int main(int argc, char ** argv)
{
  static char filename[1000] = "";
  static int output_header = 1;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"filename", required_argument, 0, 'f'},
      {"header", no_argument, &output_header, 1},
      {"no-header", no_argument, &output_header, 0},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "f:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'f': strncpy(filename, optarg, sizeof(filename)); break;
      default: abort();
    }
  }
  
  if (filename[0] == 0 && argc == 2 && argv[1] != NULL)
    strncpy(filename, argv[1], sizeof(filename));
  
  if (filename[0] == 0) { fprintf(stderr, "ERROR %s: filename not provided\n", argv[0]); return EXIT_FAILURE; }
  
  struct Block * block = bsv(filename, EXTRACT_DATA);
  write_block(stdout, block);
  free_block(block);
  
  return EXIT_SUCCESS;
}
