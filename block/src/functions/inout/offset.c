
#include "../../block.h"

#include <float.h>

struct Block * offset(struct Block * block, struct Block * opt)
{
  const char * column_name = get_attribute_value_as_string(opt, "column_name");
  
  double multiple = 1;
  
  struct Attribute * x_attr = get_attribute_by_name(opt, "x");
  struct Attribute * y_attr = get_attribute_by_name(opt, "y");
  
  if (x_attr != NULL && x_attr->type == TYPE_FLOAT)
  {
    if (x_attr->value_length == 4) x_offset = *(float*)attribute_get_value(x_attr);
    if (x_attr->value_length == 8) x_offset = *(double*)attribute_get_value(x_attr);
  }
  
  if (y_attr != NULL && y_attr->type == TYPE_FLOAT)
  {
    if (y_attr->value_length == 4) y_offset = *(float*)attribute_get_value(y_attr);
    if (y_attr->value_length == 8) y_offset = *(double*)attribute_get_value(y_attr);
  }
  
  int column_id = get_column_id_by_name_or_exit(block, (char*)column_name);
  if (column_id == -1)
  {
    fprintf(stderr, "normalize called on '%s' field, wasn't found\n", column_name);
    return block;
  }
  struct Column * column = get_column(block, column_id);
  
  if (column->type != TYPE_FLOAT)
  {
    fprintf(stderr, "normalize called on '%s' field, which is of type '%s' (only supports floating point)\n", column_name, get_type_name(column->type, column->bsize));
    return block;
  }
  
  char temp[100];
  snprintf(temp, sizeof(temp), "normalize(column:\"%s\")", column_name);
  block = add_string_attribute(block, "command", temp);
  
  double min = DBL_MAX;
  double max = -DBL_MAX;
  
  int32_t i;
  for (i = 0 ; i < block->num_rows ; i++)
  {
    double value = get_cell_as_double(block, i, column_id);
    if (value < min) min = value;
    if (value > max) max = value;
  }
  
  for (i = 0 ; i < block->num_rows ; i++)
  {
    double value = get_cell_as_double(block, i, column_id);
    value = (value-min) / (max-min) * multiple;
    set_cell_from_double(block, i, column_id, value);
  }
  
  return block;
}
