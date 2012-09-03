
#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  char * all_columns = NULL;
  int num_columns = 0;
  char ** columns = NULL;
  int * column_ids = NULL;
  
  char * column_name = NULL;
  
  int length = 0;
  
  static char filename[1000] = "";
  static int output_header = 1;
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"columns", required_argument, 0, 'c'},
      {"name", required_argument, 0, 'n'},
      //{"header", no_argument, &output_header, 1},
      //{"no-header", no_argument, &output_header, 0},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "c:n:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'c': all_columns = malloc(strlen(optarg)+1); strncpy(all_columns, optarg, strlen(optarg)); break;
      case 'n': column_name = malloc(strlen(optarg)+1); strncpy(column_name, optarg, strlen(optarg)); break;
      default: abort();
    }
  }
  
  if (all_columns == NULL || column_name == NULL)
  {
    fprintf(stderr, "Usage: %s --columns=\"ADDRESS,LF_NAME\" --name=\"FULL_ADDRESS\"\n", argv[0]);
    exit(0);
  }
  
  char * ptr = strtok(all_columns, ",");
  while (ptr != NULL)
  {
    num_columns++;
    columns = realloc(columns, num_columns*sizeof(char*));
    columns[num_columns-1] = malloc(strlen(ptr)+1);
    strncpy(columns[num_columns-1], ptr, strlen(ptr));
    columns[num_columns-1][strlen(ptr)] = 0;
    column_ids = realloc(column_ids, num_columns*sizeof(int));
    column_ids[num_columns-1] = 0;
    ptr = strtok(NULL, ",");
  }
  
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    int length = 0;
    int i;
    for (i = 0 ; i < num_columns ; i++)
    {
      column_ids[i] = get_column_id_by_name(block, columns[i]);
      if (column_ids[i] == -1) fprintf(stderr, "column %s not found\n", columns[i]);
      struct Column * column = get_column(block, column_ids[i]);
      length += get_type_size(column->type) + 1; // (plus one for space and term-null)
    }
    
    block = add_string_column_with_length(block, column_name, length);
    
    int row_id;
    for (row_id = 0 ; row_id < block->num_rows ; row_id++)
    {
      char * cell = get_cell(block, row_id, block->num_columns-1);
      strncpy(cell, get_cell(block, row_id, column_ids[0]), length);
      
      int column_id;
      for (column_id = 1 ; column_id < num_columns ; column_id++)
      {
        strncat(cell, " ", length);
        strncat(cell, get_cell(block, row_id, column_ids[column_id]), length);
      }
    }
    
    write_block(stdout, block);
    free_block(block);
  }
  free(all_columns);
  free(column_name);
  while (num_columns > 0) free(columns[--num_columns]);
  free(columns);
}
