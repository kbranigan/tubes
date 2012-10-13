
#include "../../block.h"

struct Block * unique(struct Block * block, struct Block * opt)
{
  const char * column_name = get_attribute_value_as_string(opt, "column_name");
  int32_t sort             = get_attribute_value_as_int32(opt, "sort");
  int32_t reverse          = get_attribute_value_as_int32(opt, "reverse");
  
  int column_id = get_column_id_by_name_or_exit(block, (char*)column_name);
  
  struct Column * column = get_column(block, column_id);
  
  struct Block * newblock = new_block();
  newblock = copy_all_attributes(newblock, block);
  newblock = copy_all_columns(newblock, block);
  
  char temp[100];
  snprintf(temp, sizeof(temp), "unique(column:\"%s\", sort:%d, reverse:%d)", column_name, sort, reverse);
  newblock = add_string_attribute(newblock, "command", temp);
  
  int32_t i;
  for (i = 0 ; i < block->num_rows ; i++)
  {
    void * cell = get_cell(block, i, column_id);
    
    int j;
    for (j = 0 ; j < newblock->num_rows ; j++)
      if (memcmp(cell, get_cell(newblock, j, column_id), column->bsize)==0) 
        break;
    
    if (j == newblock->num_rows) // not found
    {
      newblock = add_row(newblock);
      memcpy(get_row(newblock, newblock->num_rows-1), get_row(block, i), block->row_bsize);
    }
  }
  
  if (sort && column->type == TYPE_INT && column->bsize == 4)
    newblock = sort_block_using_int32_column(newblock, column_id, reverse);
  else if (sort)
    fprintf(stderr, "'%s': sort only works on int32 fields\n", __func__);
  
  free_block(block);
  
  return newblock;
}
