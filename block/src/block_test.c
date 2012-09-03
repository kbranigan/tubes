
#include "block.h"

struct Block * test_block()
{
  struct Block * block = new_block();
  
  ////
  
  block = add_int_attribute(block, "testing int", 5);
  block = add_long_attribute(block, "testing long", 500);
  block = add_float_attribute(block, "testing float", 5.1);
  block = add_double_attribute(block, "testing double", 6.1);
  block = add_string_attribute(block, "testing string", "hello there!");
  block = add_int_attribute(block, "testing after a string", 7);
  
  ////
  
  block = add_int_column(block, "id");
  block = add_float_column(block, "lat");
  block = add_float_column(block, "lng");
  block = add_string_column_with_length(block, "address", 20);
  
  ////
  
  int row_id;
  block = set_num_rows(block, 5);
  for (row_id = 0 ; row_id < 5 ; row_id++)
  {
    set_cell(block, row_id, 0, &row_id);
    float f = 10.0 * row_id + 10;
    set_cell(block, row_id, 1, &f);
    f = 100.0 * row_id + 100;
    set_cell(block, row_id, 2, &f);
    sprintf(get_cell(block, row_id, 3), "lol to you row %d!", row_id);
  }
  
  ////
  
  int i;
  for (i = 0 ; i < 5 ; i++)
  {
    char lol[40] = "";
    sprintf(lol, "lol to you number %d", i);
    block = add_row_with_data(block, block->num_columns, i, i + 1 + i*0.125, i + 1 + i*10.125, lol);
  }
  
  ////
  
  block = add_int_column(block, "dd");
  
  for (i = 0 ; i < block->num_rows ; i++)
  {
    set_cell(block, i, block->num_columns-1, (void*)&i);
  }
  
  block = add_string_column_with_length(block, "well", 10);
  char hello[10] = "idonnoman";
  for (i = 0 ; i < block->num_rows ; i++)
  {
    set_cell(block, i, block->num_columns-1, (void*)hello);
  }
  
  ////
  
  return block;
}

int main(int argc, char ** argv)
{
  if (stdout_is_piped())
  {
    struct Block * block = test_block();
    block = add_command(block, argc, argv);
    write_block(stdout, block);
  }
  else
  {
    struct Block * block = read_block(stdin);
    inspect_block(block);
  }
  
  return EXIT_SUCCESS;
}
