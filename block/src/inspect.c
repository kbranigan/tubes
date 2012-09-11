
#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  //assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char column_name[1000] = "";
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"column", required_argument, 0, 'c'},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "c:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'c': strncpy(column_name, optarg, sizeof(column_name)); break;
      default: abort();
    }
  }
  
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    int column_id = get_column_id_by_name(block, column_name);
    if (column_id != -1)
    {
      struct Column * column = get_column(block, column_id);
      int column_offset = get_cell(block, 0, column_id) - get_row(block, 0);
      
      int num_counts = 0;
      int * counts = NULL;
      void ** values = NULL;
      
      int i;
      for (i = 0 ; i < block->num_rows ; i++)
      {
        void * cell = get_row(block, i) + column_offset;
        int found = 0;
        
        int j;
        for (j = 0 ; j < num_counts ; j++)
        {
          //if (column->bsize == 0 && strncmp(cell, (char*)values[j], column->type)==0) { counts[j]++; found = 1; }
          if (memcmp(cell, values[j], column->bsize)==0) { counts[j]++; found = 1; }
        }
        
        if (found == 0)
        {
          num_counts++;
          counts = (int*)realloc(counts, sizeof(int)*num_counts);
          values = (void**)realloc(values, sizeof(void*)*num_counts);
          counts[num_counts-1] = 1;
          values[num_counts-1] = cell;
        }
        //if (strcmp(cell, "Local")!=0) break;
      }
      
      for (i = 0 ; i < num_counts ; i++)
      {
        fprintf(stderr, "%6d %s\n", counts[i], (char*)values[i]);
      }
      
      free(counts);
    }
    else
      inspect_block(block);
    
    free_block(block);
  }
}
