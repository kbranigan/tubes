
#include "block.h"
#include <stdlib.h>
#include <assert.h>

int main(int argc, char ** argv)
{
  int i;
  
  struct Block * block = new_block();
  int32_t old_num_attributes, old_attributes_bsize;
  int32_t old_num_columns, old_columns_bsize;
  int32_t old_num_rows, old_data_bsize;
  
  ////// add attribute
  old_num_attributes = block->num_attributes;
  old_attributes_bsize = block->attributes_bsize;
  block = add_string_attribute(block, "test", "lovely");
  assert(block->num_attributes==old_num_attributes+1);
  assert(block->attributes_bsize==old_attributes_bsize+32);
  assert(get_attribute_id_by_name(block, "test")==block->num_attributes-1);
  assert(get_attribute(block, get_attribute_id_by_name(block, "test")) != NULL);
  assert(strcmp(attribute_get_name(get_attribute(block, get_attribute_id_by_name(block, "test"))), "test")==0);
  assert(strcmp(attribute_get_value(get_attribute(block, get_attribute_id_by_name(block, "test"))), "lovely")==0);
  
  ////// add attribute
  old_num_attributes = block->num_attributes;
  old_attributes_bsize = block->attributes_bsize;
  block = add_int32_attribute(block, "testint", 15);
  assert(block->num_attributes==old_num_attributes+1);
  assert(block->attributes_bsize==old_attributes_bsize+32);
  assert(get_attribute_id_by_name(block, "testint")==block->num_attributes-1);
  assert(get_attribute(block, get_attribute_id_by_name(block, "testint")) != NULL);
  assert(strcmp(attribute_get_name(get_attribute(block, get_attribute_id_by_name(block, "testint"))), "testint")==0);
  assert(*(int32_t*)attribute_get_value(get_attribute(block, get_attribute_id_by_name(block, "testint")))==15);
  
    ////// add column
    old_num_columns = block->num_columns;
    old_columns_bsize = block->columns_bsize;
    block = add_int32_column(block, "intcol");
    assert(block->num_columns==old_num_columns+1);
    assert(block->columns_bsize==old_columns_bsize+24);
    assert(get_column_id_by_name(block, "intcol")==block->num_columns-1);
    assert(get_column(block, get_column_id_by_name(block, "intcol")) != NULL);
    assert(strcmp(column_get_name(get_column(block, get_column_id_by_name(block, "intcol"))), "intcol")==0);
  
  ////// add attribute
  old_num_attributes = block->num_attributes;
  old_attributes_bsize = block->attributes_bsize;
  block = add_int64_attribute(block, "testlong", 50);
  assert(block->num_attributes==old_num_attributes+1);
  assert(block->attributes_bsize==old_attributes_bsize+36);
  assert(get_attribute_id_by_name(block, "testlong")==block->num_attributes-1);
  assert(get_attribute(block, get_attribute_id_by_name(block, "testlong")) != NULL);
  assert(strcmp(attribute_get_name(get_attribute(block, get_attribute_id_by_name(block, "testlong"))), "testlong")==0);
  assert(*(int32_t*)attribute_get_value(get_attribute(block, get_attribute_id_by_name(block, "testlong")))==50);
  
  ////// general test
  assert(*(int32_t*)attribute_get_value(get_attribute(block, get_attribute_id_by_name(block, "testint")))==15);
  assert(strcmp(column_get_name(get_column(block, get_column_id_by_name(block, "intcol"))), "intcol")==0);
  
  ////// add attribute
  old_num_attributes = block->num_attributes;
  old_attributes_bsize = block->attributes_bsize;
  block = add_string_attribute(block, "testlongstring", "fargfargfarg");
  assert(block->num_attributes==old_num_attributes+1);
  assert(block->attributes_bsize==old_attributes_bsize+48);
  assert(get_attribute_id_by_name(block, "testlongstring")==block->num_attributes-1);
  assert(get_attribute(block, get_attribute_id_by_name(block, "testlongstring")) != NULL);
  assert(strcmp(attribute_get_name(get_attribute(block, get_attribute_id_by_name(block, "testlongstring"))), "testlongstring")==0);
  assert(strcmp(attribute_get_value(get_attribute(block, get_attribute_id_by_name(block, "testlongstring"))), "fargfargfarg")==0);
  
    ////// add 10 rows
    for (i = 0 ; i < 10 ; i++)
    {
      old_num_rows = block->num_rows;
      old_data_bsize = block->data_bsize;
      block = add_row(block);
      set_cell_from_int(block, i, 0, i*15);
      assert(block->num_rows==old_num_rows+1);
      assert(block->data_bsize==old_data_bsize+block->row_bsize);
      assert(get_cell_as_int32(block, i, 0)==i*15);
    }
      
  ////// general test
  assert(*(int32_t*)attribute_get_value(get_attribute(block, get_attribute_id_by_name(block, "testint")))==15);
  assert(strcmp(column_get_name(get_column(block, get_column_id_by_name(block, "intcol"))), "intcol")==0);
  for (i = 0 ; i < block->num_rows ; i++)
    assert(get_cell_as_int32(block, i, 0)==i*15);
  
  ////// add attribute
  old_num_attributes = block->num_attributes;
  old_attributes_bsize = block->attributes_bsize;
  block = add_string_attribute(block, "testattrafterdata", "hello");
  assert(block->num_attributes==old_num_attributes+1);
  assert(block->attributes_bsize==old_attributes_bsize+44);
  assert(get_attribute_id_by_name(block, "testattrafterdata")==block->num_attributes-1);
  assert(get_attribute(block, get_attribute_id_by_name(block, "testattrafterdata")) != NULL);
  assert(strcmp(attribute_get_name(get_attribute(block, get_attribute_id_by_name(block, "testattrafterdata"))), "testattrafterdata")==0);
  assert(strcmp(attribute_get_value(get_attribute(block, get_attribute_id_by_name(block, "testattrafterdata"))), "hello")==0);
  
  ////// general test
  assert(*(int32_t*)attribute_get_value(get_attribute(block, get_attribute_id_by_name(block, "testint")))==15);
  assert(strcmp(column_get_name(get_column(block, get_column_id_by_name(block, "intcol"))), "intcol")==0);
  for (i = 0 ; i < block->num_rows ; i++)
    assert(get_cell_as_int32(block, i, 0)==i*15);
  
    ////// add column
    old_num_columns = block->num_columns;
    old_columns_bsize = block->columns_bsize;
    block = add_string_column_with_length(block, "strcol2", 20);
    assert(block->num_columns==old_num_columns+1);
    //fprintf(stderr, "%d %d\n", block->columns_bsize, old_columns_bsize);
    assert(block->columns_bsize==old_columns_bsize+28);
    assert(get_column_id_by_name(block, "strcol2")==block->num_columns-1);
    assert(get_column(block, get_column_id_by_name(block, "strcol2")) != NULL);
    assert(strcmp(column_get_name(get_column(block, get_column_id_by_name(block, "strcol2"))), "strcol2")==0);
    blank_column_values(block, "strcol2");
    for (i = 0 ; i < block->num_rows ; i++)
      sprintf(get_cell(block, i, 1), "string row %d", i);
    
    ////// add column
    old_num_columns = block->num_columns;
    old_columns_bsize = block->columns_bsize;
    block = add_int32_column(block, "intcol2");
    assert(block->num_columns==old_num_columns+1);
    assert(block->columns_bsize==old_columns_bsize+28);
    assert(get_column_id_by_name(block, "intcol2")==block->num_columns-1);
    assert(get_column(block, get_column_id_by_name(block, "intcol2")) != NULL);
    assert(strcmp(column_get_name(get_column(block, get_column_id_by_name(block, "intcol2"))), "intcol2")==0);
    blank_column_values(block, "intcol2");
    for (i = 0 ; i < block->num_rows ; i++)
      set_cell_from_int(block, i, 2, 15*block->num_rows - i*15);
    
    ////// general test
    assert(*(int32_t*)attribute_get_value(get_attribute(block, get_attribute_id_by_name(block, "testint")))==15);
    assert(strcmp(column_get_name(get_column(block, get_column_id_by_name(block, "intcol"))), "intcol")==0);
    for (i = 0 ; i < block->num_rows ; i++)
    {
      assert(get_cell_as_int32(block, i, 0)==i*15);
      char temp[20]; sprintf(temp, "string row %d", i);
      assert(strcmp(temp, get_cell(block, i, 1))==0);
      assert(get_cell_as_int32(block, i, 2)==(15*block->num_rows - i*15));
    }
    
    ////// add column
    old_num_columns = block->num_columns;
    old_columns_bsize = block->columns_bsize;
    block = add_int64_column(block, "longcol1");
    assert(block->num_columns==old_num_columns+1);
    assert(block->columns_bsize==old_columns_bsize+28);
    assert(get_column_id_by_name(block, "longcol1")==block->num_columns-1);
    assert(get_column(block, get_column_id_by_name(block, "longcol1")) != NULL);
    assert(strcmp(column_get_name(get_column(block, get_column_id_by_name(block, "longcol1"))), "longcol1")==0);
    blank_column_values(block, "longcol1");
    for (i = 0 ; i < block->num_rows ; i++)
      set_cell_from_int(block, i, 3, 11*i);
  
  //block = add_row(block);
  
  inspect_block(block);
  fprintf(stderr, "done\n");
}























