
#include "block.h"

int main(int argc, char ** argv)
{
  static char filename[1000] = "";
  static int output_header = 1;
  static int output_quotes = 1;
  static int output_CRLF = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"filename", required_argument, 0, 'f'},
      {"header", no_argument, &output_header, 1},
      {"no-header", no_argument, &output_header, 0},
      {"quotes", no_argument, &output_quotes, 1},
      {"no-quotes", no_argument, &output_quotes, 0},
      {"output-CRLF", no_argument, &output_CRLF, 1},
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
  
  FILE * fp = (filename[0] != 0) ? fopen(filename, "w") : NULL;
  
  if (fp == NULL || filename[0] == 0)
  {
    fprintf(stderr, "ERROR: Usage: %s --filename=[file_name]\n", argv[0]);
    return -1;
  }
  
  struct Block * block = read_block(stdin);
  
  if (block == NULL)
  {
    fprintf(stderr, "%s: ERROR, requires piped input data\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  // header
  if (output_header)
  {
    int column_id = 0;
    for (column_id = 0 ; column_id < block->num_columns ; column_id++)
    {
      if (output_quotes)
        fprintf(fp, "%s\"%s\"", ((column_id == 0)?"":","), column_get_name(get_column(block, column_id)));
      else
        fprintf(fp, "%s%s", ((column_id == 0)?"":","), column_get_name(get_column(block, column_id)));
    }
    if (output_CRLF)
      fprintf(fp, "\r\n");
    else
      fprintf(fp, "\n");
  }
  
  int row_id = 0;
  for (row_id = 0 ; row_id < block->num_rows ; row_id++)
  {
    int column_id = 0;
    for (column_id = 0 ; column_id < block->num_columns ; column_id++)
    {
      if (column_id > 0) fprintf(fp, ",");
      
      struct Column * column = get_column(block, column_id);
      if (column->type == TYPE_CHAR)
      {
        char * cell = get_cell(block, row_id, column_id);
        if (strstr(cell, ",")==NULL)
          fprintf(fp, "%s", cell);
        else
        {
          if (strstr(cell, "\"")==NULL)
            fprintf(fp, "\"%s\"", cell);
          else
          {
            fputc('"', fp);
            int char_id = 0;
            while (cell[char_id] != 0)
            {
              if (cell[char_id] == '"') fputc('"', fp);
              fputc(cell[char_id], fp);
              char_id++;
            }
            fputc('"', fp);
          }
        }
      }
      else
        fprintf_cell(fp, block, row_id, column_id);
    }
    if (output_CRLF)
      fprintf(fp, "\r\n");
    else
      fprintf(fp, "\n");
  }
  
  int ignored_block_count = 0;
  struct Block * bad_block = NULL;
  while ((bad_block = read_block(stdin)))
  {
    ignored_block_count++;
    free_block(bad_block);
  }
  
  if (ignored_block_count > 0)
    fprintf(stderr, "%s: Warning - %d ignored additional block(s) - only handles the first block piped in\n", argv[0], ignored_block_count);
  
  fprintf(stderr, "%s: Finished.  %d rows, %d columns written to '%s', enjoy!\n", argv[0], block->num_rows, block->num_columns, filename);
  free_block(block);
  
  return EXIT_SUCCESS;
}
