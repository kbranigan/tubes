
#include "block.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv)
{
  static char filename[1000] = "";
  static int output_header = 1;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"filename", required_argument, 0, 'f'},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "f:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'f': strncpy(filename, optarg, sizeof(filename)); break;
      default: abort();
    }
  }
  
  if (filename[0] == 0 && argc == 2 && argv[1] != NULL)
    strncpy(filename, argv[1], sizeof(filename));
  
  if (filename[0] == 0) { fprintf(stderr, "ERROR %s: filename not provided\n", argv[0]); return EXIT_FAILURE; }
  
  FILE * fp = fopen(filename, "r");
  if (fgetc(fp) != 0xEF || fgetc(fp) != 0xBB || fgetc(fp) != 0xBF) rewind(fp); // skip UTF-8 BOM if it's found

  if (fp != NULL)
  {
    struct Block * block = new_block();
    char header[500] = "";
    if (fgets(header, sizeof(header), fp) == NULL)
    {
      fprintf(stderr, "ERROR: could not read header from '%s'\n", filename);
      exit(1);
    }

    int length = 0;
    char * line_ptr = header;
    char * field;
    while ((field = strsep(&line_ptr, ",")) != NULL)
    {
      length = strlen(field)-1;
      if (field[length] == '\r' || field[length] == '\n') field[length] = 0;
      length = strlen(field)-1;
      if (field[length] == '\r' || field[length] == '\n') field[length] = 0;
      block = add_string_column_with_length(block, field, 1);
    }

    int row_id = 0;
    char row[500] = "";
    while (fgets(row, sizeof(row), fp) != NULL)
    {
      length = strlen(row)-1;
      if (row[length] == '\r' || row[length] == '\n') row[length] = 0;
      length = strlen(row)-1;
      if (row[length] == '\r' || row[length] == '\n') row[length] = 0;
      //fprintf(stderr, "row: \"%s\"\n", row);

      if (length == 0) continue;

      block = add_row(block);

      int column_id = 0;
      line_ptr = row;
      while ((field = strsep(&line_ptr, ",")) != NULL)
      {
        length = (line_ptr == 0) ? strlen(field) : line_ptr - field - 1;

        if (field[0] == '"')
        {
          int count = 0;
          char * start_of_field = field;
          int length = strlen(field);
          while (field[strlen(field)-1] != '"' && count < 100)
          {
            //fprintf(stderr, "last char = %d\n", field[strlen(field)-1]);
            //fprintf(stderr, "count     = %d\n", count);            
            //fprintf(stderr, "line_ptr  = '%s'\n", line_ptr);
            //fprintf(stderr, "field     = '%s'\n", field);
            length = strlen(field);
            //fprintf(stderr, "length    = '%d'\n", length);
            field[length] = ',';
            line_ptr ++;
            //fprintf(stderr, "new field = '%s'\n", field);
            //fprintf(stderr, "new line_ptr  = '%s'\n", line_ptr);
            strsep(&line_ptr, ",");
            count++;
          }
          if (count == 100)
          {
            fprintf(stderr, "major problem reading file in read_csv_fast, perhaps use read_csv instead\n");
          }
          field = start_of_field;
          if (field[0] == '"' && field[strlen(field)-1] == '"')
          {
            field[strlen(field)-1] = 0;
            field++;
          }
        }
        
        //fprintf(stderr, "  cell: \"%s\" (%d)\n", field, length);
        struct Column * column = get_column(block, column_id);
        if (length >= column->bsize)
        {
          //fprintf(stderr, "row %d, column %d becomes %d big (was %d)\n", row_id, column_id, memory_pad(length+1,4), column->bsize);
          block = set_string_column_length(block, column_id, length);
        }
        set_cell_from_string(block, row_id, column_id, field);
        char * cell = get_cell(block, row_id, column_id);
        //fprintf(stderr, "    cell: \"%s\" (%d)\n", cell, length);
        
        //cell[memory_pad(length+1,4)-1] = 0;
        //cell[length] = 0;
        column_id++;
      }
      row_id++;
      //if (row_id > 10)
      //  break;
    }

    write_block(stdout, block);
    free_block(block);
    fclose(fp);
  }
  else
  {
    fprintf(stderr, "'%s' doesn't exist.\n", filename);
  }
  
  return EXIT_SUCCESS;
}
