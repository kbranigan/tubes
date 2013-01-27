
#include "../../block.h"

#include <float.h>

struct Block * offset(struct Block * block, struct Block * opt)
{
  const char * column_name = get_attribute_value_as_string(opt, "column_name");
  double offset            = get_attribute_value_as_double(opt, "offset");
  
  int column_id = get_column_id_by_name_or_exit(block, (char*)column_name);
  if (column_id == -1)
  {
    fprintf(stderr, "%s() called on '%s' field, wasn't found\n", __func__, column_name);
    return block;
  }
  struct Column * column = get_column(block, column_id);
  
  if (column->type != TYPE_FLOAT)
  {
    fprintf(stderr, "%s() called on '%s' field, which is of type '%s' (only supports floating point)\n", __func__, column_name, get_type_name(column->type, column->bsize));
    return block;
  }
  
  char temp[100];
  snprintf(temp, sizeof(temp), "offset(column:\"%s\" by %lf)", column_name, offset);
  block = add_string_attribute(block, "command", temp);
  
  int i;
  for (i = 0 ; i < block->num_rows ; i++)
    set_cell_from_double(block, i, column_id, get_cell_as_double(block, i, column_id) + offset);
  
  return block;
}
