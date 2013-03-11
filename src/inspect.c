
#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  //assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char column_name[1000] = "";
  static int sort_that_column = 0;
  static int debug = 0;
  static int reverse = 0;
  
	struct Params * params = NULL;
	params = add_string_param(params, "column", 'c', column_name, 0);
	params = add_flag_param(params, "sort", 's', &sort_that_column, 0);
	params = add_flag_param(params, "reverse", 'r', &reverse, 0);
	params = add_flag_param(params, "debug", 'd', &debug, 0);
	eval_params(params, argc, argv);
	
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    if (column_name[0] != 0)
    {
      int column_id = get_column_id_by_name_or_exit(block, column_name);
      
      struct Column * column = get_column(block, column_id);
      
      struct Block * counts = new_block();
      counts = add_int32_column(counts, "count");
      int count_column_id = 0;
      counts = _add_column(counts, column->type, column->bsize, column_get_name(column));
      int value_column_id = 1;
      
      int i;
      for (i = 0 ; i < block->num_rows ; i++)
      {
        void * cell = get_cell(block, i, column_id);
        
        int j;
        for (j = 0 ; j < counts->num_rows ; j++)
        {
          if (memcmp(cell, get_cell(counts, j, value_column_id), column->bsize)==0) 
          {
            set_cell_from_int32(counts, j, count_column_id, get_cell_as_int32(counts, j, count_column_id) + 1);
            break;
          }
        }
        
        if (j == counts->num_rows) // not found
        {
          counts = add_row(counts);
          set_cell_from_int32(counts, j, count_column_id, 1); // initialize count at 1 occurrence
          memcpy(get_cell(counts, j, value_column_id), cell, column->bsize);
        }
      }
      
      if (sort_that_column == 1 && column->type == TYPE_INT)
        counts = sort_block_using_int32_column(counts, count_column_id, reverse);
      
      inspect_block(counts);
      free_block(counts);
    }
    else
      inspect_block(block);
    
		if (stdout_is_piped()) {
			write_block(stdout, block);
		}
		
    free_block(block);
  }
}
