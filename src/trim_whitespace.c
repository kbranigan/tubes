
#include "block.h"

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	add_command_in_foreach(argc, argv);

  char * all_columns = NULL;
  int num_columns = 0;
  char ** columns = NULL;
  int * column_ids = NULL;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"columns", required_argument, 0, 'c'},
      //{"header", no_argument, &output_header, 1},
      //{"no-header", no_argument, &output_header, 0},
      //{"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "c:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'c': all_columns = malloc(strlen(optarg)+1); strncpy(all_columns, optarg, strlen(optarg)); break;
      default: abort();
    }
  }
  
  if (all_columns == NULL)
  {
    fprintf(stderr, "Usage: %s --columns=\"ADDRESS,LF_NAME\"\n", argv[0]);
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
	while ((block = read_block(stdin))) {
    int length = 0;
    int i;
    for (i = 0 ; i < num_columns ; i++)
    {
      column_ids[i] = get_column_id_by_name(block, columns[i]);
      if (column_ids[i] == -1) fprintf(stderr, "column %s not found\n", columns[i]);
      struct Column * column = get_column(block, column_ids[i]);
      length += column->bsize + 1; //get_type_size(column->type) + 1; // (plus one for space and term-null)
    }


    int row_id;
    for (row_id = 0 ; row_id < block->num_rows ; row_id++)
    {
    	int column_id_index;
    	for (column_id_index = 0 ; column_id_index < num_columns ; column_id_index++)
    	{
    		int column_id = column_ids[column_id_index];
    		struct Column * column = get_column(block, column_id);
      	
				char * cell = get_cell(block, row_id, column_id);
				while (cell[0] == ' ') memmove(cell, &cell[1], column->bsize);
				while (cell[strlen(cell)-1] == ' ') cell[strlen(cell)-1] = '\0';
    	}
    }

		write_block(stdout, block);
		free_block(block);
	}
}
