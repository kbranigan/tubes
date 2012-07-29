
#include "block.h"

int main(int argc, char ** argv)
{
  /*
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  //assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char filename[1000] = "";
  static int output_header = 1;
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"filename", required_argument, 0, 'f'},
      {"header", no_argument, &output_header, 1},
      {"no-header", no_argument, &output_header, 0},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "d:f:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'f': strncpy(filename, optarg, sizeof(filename)); break;
      default: abort();
    }
  }
  */
  
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    write_block(stdout, block);
    free_block(block);
  }
}
