
#include "block.h"

enum OPERATOR {
  OPERATOR_DELETE = 1,
  OPERATOR_PASS = 2
};

char operator_names[3][40] = {
  "unknown", "delete", "pass"
};

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char column_name[1000] = "";
  static char value[1000] = "";
  static enum OPERATOR operator = OPERATOR_PASS;
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
      case 'o':
        if      (strcmp(optarg, "PASS")==0)   operator = OPERATOR_PASS;
        else if (strcmp(optarg, "DELETE")==0) operator = OPERATOR_DELETE;
        else { fprintf(stderr, "Invalid operator '%s', must be PASS or DELETE\n", optarg); exit(0); }
        break;
      case 'v': strncpy(value, optarg, sizeof(value)); break;
      default: abort();
    }
  }
  
  if (column_name[0] == 0 || value[0] == 0)
  { fprintf(stderr, "USAGE: %s --column=\"FULL_ADDRESS\" --value=\"29 CAMDEN ST\"\n", argv[0]); exit(0); }
  
  int i,j;
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    int column_id = get_column_id_by_name_or_exit(block, column_name);
    
    struct Column * column = get_column(block, column_id);
    if (column->type != TYPE_CHAR && (column->type != TYPE_INT || column->bsize != 4))
    {
      fprintf(stderr, "column '%s' is not a string or int32_t.\n", column_name);
      write_block(stdout, block);
      free_block(block);
      continue;
    }
    
    struct Block * newblock = new_block();
    newblock = copy_all_attributes(newblock, block);
    
    int ivalue;
    if (column->type == TYPE_INT) ivalue = atoi(value);
    
    for (i = 0 ; i < block->num_rows ; i++)
    {
      char * cell = (char*)get_cell(block, i, column_id);
      
      if (column->type == TYPE_INT && column->bsize == 4)
      {
        if ((operator == OPERATOR_DELETE && (*(int32_t*)cell) != ivalue) || 
            (operator == OPERATOR_PASS   && (*(int32_t*)cell) == ivalue))
        newblock->num_rows++;
      }
      else if (column->type == TYPE_CHAR)
      {
        int comp = strncmp(value, cell, column->bsize);
        //if (newblock->num_rows < 5) fprintf(stderr, "%d: %s vs %s (%d) (%d)\n", i, cell, value, column->bsize, comp);
        if ((operator == OPERATOR_DELETE && comp != 0) ||
            (operator == OPERATOR_PASS   && comp == 0))
        {
          //if (newblock->num_rows < 5) fprintf(stderr, "hi %d: %s vs %s (%d)\n", i, cell, value, column->bsize);
          newblock->num_rows++;
        }
      }
    }
    int new_num_rows = newblock->num_rows;
    newblock->num_rows = 0;
    fprintf(stderr, "%d - %d = %d\n", block->num_rows, block->num_rows-new_num_rows, new_num_rows);
    
    char temp[1000];
    sprintf(temp, "'%s' %s '%s' (removing %d rows)", column_name, operator_names[operator], value, block->num_rows - new_num_rows);
    newblock = add_string_attribute(newblock, "filter", temp);
    newblock = copy_all_columns(newblock, block);
    
    newblock->num_rows = new_num_rows;
    newblock = set_num_rows(newblock, newblock->num_rows);
    
    int newblock_row_id = 0;
    for (i = 0 ; i < block->num_rows ; i++)
    {
      char * cell = (char*)get_cell(block, i, column_id);
      
      if (column->type == TYPE_INT && column->bsize == 4)
      {
        if ((operator == OPERATOR_DELETE && *(int32_t*)cell != ivalue) || 
            (operator == OPERATOR_PASS   && *(int32_t*)cell == ivalue))
        {
          memcpy(get_row(newblock, newblock_row_id), get_row(block, i), block->row_bsize);
          newblock_row_id++;
        }
      }
      else if (column->type == TYPE_CHAR)
      {
        int comp = strncmp(value, cell, column->bsize);
        if ((operator == OPERATOR_DELETE && comp != 0) ||
            (operator == OPERATOR_PASS   && comp == 0))
        {
          memcpy(get_row(newblock, newblock_row_id), get_row(block, i), block->row_bsize);
          newblock_row_id++;
        }
      }
    }
    
    write_block(stdout, newblock);
    free_block(newblock);
    free_block(block);
  }
}
