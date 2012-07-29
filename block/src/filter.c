
#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char column_name[1000] = "";
  static char value[1000] = "";
  static char operator[10] = "IS";
  static int output_header = 1;
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"column", required_argument, 0, 'c'},
      {"value", required_argument, 0, 'v'},
      {"operator", required_argument, 0, 'o'},
      //{"add", no_argument, &output_header, 1},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "c:v:o:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'c': strncpy(column_name, optarg, sizeof(column_name)); break;
      case 'v': strncpy(value, optarg, sizeof(value)); break;
      case 'o': strncpy(operator, optarg, sizeof(operator)); break;
      default: abort();
    }
  }
  
  if (column_name[0] == 0) { fprintf(stderr, "%s: column required\n", argv[0]); exit(0); }
  if (value[0] == 0) { fprintf(stderr, "%s: value required\n", argv[0]); exit(0); }
  if (operator[0] == 0) { fprintf(stderr, "%s: operator required\n", argv[0]); exit(0); }
  
  int i,j;
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    int column_id = find_column_id_by_name(block, column_name);
    if (column_id == -1)
    {
      fprintf(stderr, "column '%s' not found.\n", column_name);
      write_block(stdout, block);
      free_block(block);
      continue;
    }
    struct Column * column = get_column(block, column_id);
    if (!column_is_string(column))
    {
      fprintf(stderr, "column '%s' is not a string (only string based filtering supported for now).\n", column_name);
      write_block(stdout, block);
      free_block(block);
      continue;
    }
    
    struct Block * newblock = new_block();
    for (i = 0 ; i < block->num_attributes ; i++)
    {
      struct Attribute * attr = get_attribute(block, i);
      newblock = _add_attribute(newblock, attr->type, attribute_get_name(attr), attribute_get_value(attr));//&attr->name, &attr->name + attr->name_length);
    }
    
    for (i = 0 ; i < block->num_rows ; i++)
    {
      char * cell = (char*)get_cell(block, i, column_id);//(get_row(block, i)+column_offset);
      
      if ((strcmp(operator, "NOT")==0 && strncmp(value, cell, column->type) != 0) || 
          (strcmp(operator, "IS")==0 && strncmp(value, cell, column->type) == 0))
        newblock->num_rows++;
    }
    
    int new_num_rows = newblock->num_rows;
    newblock->num_rows = 0;
    
    char temp[1000];
    sprintf(temp, "'%s' => '%s' (removing %d rows)", column_name, value, block->num_rows - new_num_rows);
    newblock = add_string_attribute(newblock, "filter", temp);
    
    for (i = 0 ; i < block->num_columns ; i++)
    {
      struct Column * col = get_column(block, i);
      newblock = _add_column(newblock, col->type, column_get_name(col));
    }
    
    newblock->num_rows = new_num_rows;
    newblock = set_num_rows(newblock, newblock->num_rows);
    
    int newblock_row_id = 0;
    for (i = 0 ; i < block->num_rows ; i++)
    {
      char * cell = (char*)get_cell(block, i, column_id); //(get_row(block, row_id)+column_offset);
      
      if ((strcmp(operator, "NOT")==0 && strncmp(value, cell, column->type) != 0) || 
          (strcmp(operator, "IS")==0 && strncmp(value, cell, column->type) == 0))
      {
        memcpy(get_row(newblock, newblock_row_id), get_row(block, i), block->row_bsize);
        newblock_row_id++;
      }
    }
    
    write_block(stdout, newblock);
    free_block(newblock);
    free_block(block);
  }
}
