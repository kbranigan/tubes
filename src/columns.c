
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
  static char intfromtime_columns_all[1000] = "";
  static char float_columns_all[1000] = "";
  static int output_header = 1;
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"remove", required_argument, 0, 'r'},
      {"makeint", required_argument, 0, 'i'},
      {"makeintfromtime", required_argument, 0, 't'},
      {"makefloat", required_argument, 0, 'f'},
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
      case 't': strncpy(intfromtime_columns_all, optarg, sizeof(intfromtime_columns_all)); break;
      case 'f': strncpy(float_columns_all, optarg, sizeof(float_columns_all)); break;
      default: abort();
    }
  }
  
  char remove_columns_copy[1000] = "";
  strncpy(remove_columns_copy, remove_columns_all, 1000);
  
  char int_columns_copy[1000] = "";
  strncpy(int_columns_copy, int_columns_all, 1000);

  char intfromtime_columns_copy[1000] = "";
  strncpy(intfromtime_columns_copy, intfromtime_columns_all, 1000);

  char float_columns_copy[1000] = "";
  strncpy(float_columns_copy, float_columns_all, 1000);
  
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
  
  int num_intfromtime_columns = 0;
  char ** intfromtime_columns = NULL;
  pch = strtok(intfromtime_columns_all, ",");
  while (pch != NULL)
  {
    num_intfromtime_columns++;
    intfromtime_columns = realloc(intfromtime_columns, sizeof(char*)*num_intfromtime_columns);
    intfromtime_columns[num_intfromtime_columns-1] = pch;
    pch = strtok(NULL, ",");
  }
  
  int num_float_columns = 0;
  char ** float_columns = NULL;
  pch = strtok(float_columns_all, ",");
  while (pch != NULL)
  {
    num_float_columns++;
    float_columns = realloc(float_columns, sizeof(char*)*num_float_columns);
    float_columns[num_float_columns-1] = pch;
    pch = strtok(NULL, ",");
  }
  
  int i,j;
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    struct Block * newblock = new_block();
    newblock = copy_all_attributes(newblock, block);
    
    if (remove_columns_copy[0] != 0)
      newblock = add_string_attribute(newblock, "remove attributes", remove_columns_copy);
    
    if (int_columns_copy[0] != 0)
      newblock = add_string_attribute(newblock, "convert columns to int", int_columns_copy);
    
    if (intfromtime_columns_copy[0] != 0)
      newblock = add_string_attribute(newblock, "convert columns to time", intfromtime_columns_copy);
    
    if (float_columns_copy[0] != 0)
      newblock = add_string_attribute(newblock, "convert columns to float", float_columns_copy);
    
    int num_remove_column_ids = 0;
    int * remove_column_ids = NULL;
    
    for (i = 0 ; i < block->num_columns ; i++)
    {
      struct Column * col = get_column(block, i);
      for (j = 0 ; j < num_remove_columns ; j++)
      {
        // fprintf(stderr, "%s(%d(%d)) vs %s(%d) = %d\n", 
        //                  column_get_name(col), col->name_length, strlen(column_get_name(col)), 
        //                  remove_columns[j], strlen(remove_columns[j]), 
        //                   strcmp(column_get_name(col), remove_columns[j]));
        if (strcmp(column_get_name(col), remove_columns[j])==0) break;
      }
      if (j == num_remove_columns) // not a column to be removed
      {
        //fprintf(stderr, "%d: %s (%d) - do not remove\n", i, column_get_name(col), strlen(column_get_name(col)));
        for (j = 0 ; j < num_int_columns ; j++)
          if (strcmp(column_get_name(col), int_columns[j])==0) break;
        int l = 0;
        for (l = 0 ; l < num_intfromtime_columns ; l++)
          if (strcmp(column_get_name(col), intfromtime_columns[l])==0) break;
        int k = 0;
        for (k = 0 ; k < num_float_columns ; k++)
          if (strcmp(column_get_name(col), float_columns[k])==0) break;

        if (j == num_int_columns && k == num_float_columns && l == num_intfromtime_columns)
        {
          num_remove_column_ids++;
          remove_column_ids = realloc(remove_column_ids, sizeof(int)*num_remove_column_ids);
          remove_column_ids[num_remove_column_ids-1] = i;
          newblock = _add_column(newblock, col->type, col->bsize, column_get_name(col));
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
        newblock = add_int32_column(newblock, column_get_name(col));
      }
    }
    
    int num_intfromtime_column_ids = 0;
    int * intfromtime_column_ids = NULL;
    
    for (i = 0 ; i < block->num_columns ; i++)
    {
      struct Column * col = get_column(block, i);
      for (j = 0 ; j < num_intfromtime_columns ; j++)
        if (strcmp(column_get_name(col), intfromtime_columns[j])==0) break;
      if (j != num_intfromtime_columns)
      {
        num_intfromtime_column_ids++;
        intfromtime_column_ids = realloc(intfromtime_column_ids, sizeof(int)*num_intfromtime_column_ids);
        intfromtime_column_ids[num_intfromtime_column_ids-1] = i;
        newblock = add_int32_column(newblock, column_get_name(col));
      }
    }
    
    int num_float_column_ids = 0;
    int * float_column_ids = NULL;
    
    for (i = 0 ; i < block->num_columns ; i++)
    {
      struct Column * col = get_column(block, i);
      for (j = 0 ; j < num_float_columns ; j++)
        if (strcmp(column_get_name(col), float_columns[j])==0) break;
      if (j != num_float_columns)
      {
        num_float_column_ids++;
        float_column_ids = realloc(float_column_ids, sizeof(int)*num_float_column_ids);
        float_column_ids[num_float_column_ids-1] = i;
        newblock = add_double_column(newblock, column_get_name(col));
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
        
        memcpy(dst, src, col->bsize);
      }
      for (j = 0 ; j < num_int_column_ids ; j++)
      {
        set_cell_from_int32(newblock, i, j + num_remove_column_ids, get_cell_as_int32(block, i, int_column_ids[j]));
      }
      for (j = 0 ; j < num_intfromtime_column_ids ; j++)
      {
        char * cell = get_cell(block, i, intfromtime_column_ids[j]);
        int len = strlen(cell);
        if (len >= 7)
        { 
          int32_t t = atoi(&cell[len-2]) + atoi(&cell[len-5])*60 + atoi(cell)*60*60;
          set_cell_from_int32(newblock, i, j + num_remove_column_ids + num_int_column_ids, t);
        }
        else
        {
          fprintf(stderr, "ABORTING: %s expecting time to be in #:##:## format. But length invalid. (%d)\n", __func__, len);
          exit(1);
        }
      }
      for (j = 0 ; j < num_float_column_ids ; j++)
      {
        set_cell_from_double(newblock, i, j + num_remove_column_ids + num_int_column_ids + num_intfromtime_column_ids, get_cell_as_double(block, i, float_column_ids[j]));
      }
    }
    free(remove_column_ids);
    free(int_column_ids);
    free(float_column_ids);
    
    write_block(stdout, newblock);
    free_block(newblock);
    free_block(block);
  }
}
