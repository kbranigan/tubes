
#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char remove_columns[1000] = "";
  static int output_header = 1;
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"remove", required_argument, 0, 'r'},
      //{"add", no_argument, &output_header, 1},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "r:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'r': strncpy(remove_columns, optarg, sizeof(remove_columns)); break;
      default: abort();
    }
  }
  
  char remove_columns_copy[1000] = "";
  strncpy(remove_columns_copy, remove_columns, 1000);
  
  int num_columns = 0;
  char ** columns = NULL;
  char * pch = strtok(remove_columns, ",");
  while (pch != NULL)
  {
    num_columns++;
    columns = realloc(columns, sizeof(char*)*num_columns);
    columns[num_columns-1] = pch;
    pch = strtok(NULL, ",");
  }
  
  int i,j;
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    struct Block * newblock = new_block();
    for (i = 0 ; i < block->num_attributes ; i++)
    {
      struct Attribute * attr = get_attribute(block, i);
      
      newblock = _add_attribute(newblock, attr->type, attribute_get_name(attr), attribute_get_value(attr));//&attr->name, &attr->name + attr->name_length);
    }
    
    if (remove_columns_copy[0] != 0)
      newblock = add_string_attribute(newblock, "remove attributes", remove_columns_copy);
    
    int num_column_ids = 0;
    int * column_ids = NULL;
    
    for (i = 0 ; i < block->num_columns ; i++)
    {
      struct Column * col = get_column(block, i);
      for (j = 0 ; j < num_columns ; j++)
        if (strcmp(column_get_name(col), columns[j])==0) break;
      if (j == num_columns)
      {
        num_column_ids++;
        column_ids = realloc(column_ids, sizeof(int)*num_column_ids);
        column_ids[num_column_ids-1] = i;
        newblock = _add_column(newblock, col->type, column_get_name(col));
      }
    }
    
    newblock = set_num_rows(newblock, block->num_rows);
    
    for (i = 0 ; i < newblock->num_rows ; i++)
    {
      for (j = 0 ; j < num_column_ids ; j++)
      {
        void * dst = get_cell(newblock, i, j);
        void * src = get_cell(block, i, column_ids[j]);
        struct Column * col = get_column(block, column_ids[j]);
        
        int size;
        if (col->type == INT_TYPE) size = sizeof(int);
        else if (col->type == LONG_TYPE) size = sizeof(long);
        else if (col->type == FLOAT_TYPE) size = sizeof(float);
        else if (col->type == DOUBLE_TYPE) size = sizeof(double);
        else size = col->type;
        
        memcpy(dst, src, size);
      }
    }
    free(column_ids);
    
    write_block(stdout, newblock);
    free_block(newblock);
    free_block(block);
  }
}
