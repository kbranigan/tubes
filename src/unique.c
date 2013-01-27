
#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char column_name[1000] = "";
  static int sort_that_column = 0;
  static int debug = 0;
  static int reverse = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"column", required_argument, 0, 'c'},
      {"sort", no_argument, 0, 's'},
      {"reverse", no_argument, 0, 'r'},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "c:sr", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'c': strncpy(column_name, optarg, sizeof(column_name)); break;
      case 's': sort_that_column = 1; break;
      case 'r': reverse = 1; break;
      default: abort();
    }
  }
  
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    int column_id = get_column_id_by_name_or_exit(block, column_name);
    
    struct Column * column = get_column(block, column_id);
    
    struct Block * newblock = new_block();
    newblock = copy_all_attributes(newblock, block);
    newblock = copy_all_columns(newblock, block);
    
    newblock = add_command(newblock, argc, argv);
    
    int i;
    for (i = 0 ; i < block->num_rows ; i++)
    {
      void * cell = get_cell(block, i, column_id);
      
      int j;
      for (j = 0 ; j < newblock->num_rows ; j++)
      {
        if (memcmp(cell, get_cell(newblock, j, column_id), column->bsize)==0) 
        {
          // found
          break;
        }
      }
      
      if (j == newblock->num_rows) // not found
      {
        newblock = add_row(newblock);
        memcpy(get_row(newblock, newblock->num_rows-1), get_row(block, i), block->row_bsize);
      }
    }
    
    if (sort_that_column == 1 && column->type == TYPE_INT)
      newblock = sort_block_using_int32_column(newblock, column_id, reverse);
    
    write_block(stdout, newblock);
    free_block(newblock);
    free_block(block);
  }
}
