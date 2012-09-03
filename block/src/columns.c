
#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char remove_columns_all[1000] = "";
  static char int_columns_all[1000] = "";
  static int output_header = 1;
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"remove", required_argument, 0, 'r'},
      {"makeint", required_argument, 0, 'i'},
      //{"add", no_argument, &output_header, 1},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "r:i:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'r': strncpy(remove_columns_all, optarg, sizeof(remove_columns_all)); break;
      case 'i': strncpy(int_columns_all, optarg, sizeof(int_columns_all)); break;
      default: abort();
    }
  }
  
  char remove_columns_copy[1000] = "";
  strncpy(remove_columns_copy, remove_columns_all, 1000);
  
  char int_columns_copy[1000] = "";
  strncpy(int_columns_copy, int_columns_all, 1000);
  
  int num_remove_columns = 0;
  char ** remove_columns = NULL;
  char * pch = strtok(remove_columns_all, ",");
  while (pch != NULL)
  {
    num_remove_columns++;
    remove_columns = realloc(remove_columns, sizeof(char*)*num_remove_columns);
    remove_columns[num_remove_columns-1] = pch;
    pch = strtok(NULL, ",");
  }
  
  int num_int_columns = 0;
  char ** int_columns = NULL;
  pch = strtok(int_columns_all, ",");
  while (pch != NULL)
  {
    num_int_columns++;
    int_columns = realloc(int_columns, sizeof(char*)*num_int_columns);
    int_columns[num_int_columns-1] = pch;
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
    
    if (int_columns_copy[0] != 0)
      newblock = add_string_attribute(newblock, "convert attributes to int", int_columns_copy);
    
    int num_remove_column_ids = 0;
    int * remove_column_ids = NULL;
    
    for (i = 0 ; i < block->num_columns ; i++)
    {
      struct Column * col = get_column(block, i);
      for (j = 0 ; j < num_remove_columns ; j++)
        if (strcmp(column_get_name(col), remove_columns[j])==0) break;
      if (j == num_remove_columns)
      {
        for (j = 0 ; j < num_int_columns ; j++)
          if (strcmp(column_get_name(col), int_columns[j])==0) break;
        if (j == num_int_columns)
        {
          num_remove_column_ids++;
          remove_column_ids = realloc(remove_column_ids, sizeof(int)*num_remove_column_ids);
          remove_column_ids[num_remove_column_ids-1] = i;
          newblock = _add_column(newblock, col->type, column_get_name(col));
        }
      }
    }
    
    int num_int_column_ids = 0;
    int * int_column_ids = NULL;
    
    for (i = 0 ; i < block->num_columns ; i++)
    {
      struct Column * col = get_column(block, i);
      for (j = 0 ; j < num_int_columns ; j++)
        if (strcmp(column_get_name(col), int_columns[j])==0) break;
      if (j != num_int_columns)
      {
        num_int_column_ids++;
        int_column_ids = realloc(int_column_ids, sizeof(int)*num_int_column_ids);
        int_column_ids[num_int_column_ids-1] = i;
        newblock = add_int_column(newblock, column_get_name(col));
      }
    }
    
    newblock = set_num_rows(newblock, block->num_rows);
    
    for (i = 0 ; i < newblock->num_rows ; i++)
    {
      for (j = 0 ; j < num_remove_column_ids ; j++)
      {
        void * dst = get_cell(newblock, i, j);
        void * src = get_cell(block, i, remove_column_ids[j]);
        struct Column * col = get_column(block, remove_column_ids[j]);
        
        memcpy(dst, src, get_type_size(col->type));
      }
      for (j = 0 ; j < num_int_column_ids ; j++)
      {
        //void * dst = get_cell(newblock, i, j);
        //void * src = get_cell(block, i, int_column_ids[j]);
        //struct Column * col = get_column(block, int_column_ids[j]);
        
        set_cell_from_int(newblock, i, j + num_remove_column_ids, get_cell_as_int(block, i, int_column_ids[j]));
      }
    }
    free(remove_column_ids);
    free(int_column_ids);
    
    write_block(stdout, newblock);
    free_block(newblock);
    free_block(block);
  }
}
