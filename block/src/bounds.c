
#include "block.h"
#include <alloca.h>

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  //assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    long * offsets = alloca(sizeof(long)*block->num_columns);
    memset(offsets, 0, sizeof(long)*block->num_columns);
    
    int i,j;
    for (i = 0 ; i < block->num_columns ; i++)
      offsets[i] = get_cell(block, 0, i) - get_row(block, 0);
    
    for (i = 0 ; i < block->num_columns ; i++)
    {
      struct Column * column = get_column(block, i);
      if (column->type == TYPE_INT && column->bsize == 4)
      {
        int min = *(int*)(get_row(block, 0)+offsets[i]);
        int max = min;
        for (j = 0 ; j < block->num_rows ; j++)
        {
          void * row = get_row(block, j);
          int value = *(int*)(row+offsets[i]);
          if (value > max) max = value;
          if (value < min) min = value;
        }
        fprintf(stderr, "%s: %d to %d\n", column_get_name(column), min, max);
      }
      else if (column->type == TYPE_INT && column->bsize == 8)
      {
        long min = *(long*)(get_row(block, 0)+offsets[i]);
        long max = min;
        for (j = 0 ; j < block->num_rows ; j++)
        {
          void * row = get_row(block, j);
          long value = *(long*)(row+offsets[i]);
          if (value > max) max = value;
          if (value < min) min = value;
        }
        fprintf(stderr, "%s: %ld to %ld\n", column_get_name(column), min, max);
      }
      else if (column->type == TYPE_FLOAT && column->bsize == 4)
      {
        float min = *(float*)(get_row(block, 0)+offsets[i]);
        float max = min;
        for (j = 0 ; j < block->num_rows ; j++)
        {
          void * row = get_row(block, j);
          float value = *(float*)(row+offsets[i]);
          if (value > max) max = value;
          if (value < min) min = value;
        }
        fprintf(stderr, "%s: %f to %f\n", column_get_name(column), min, max);
      }
      else if (column->type == TYPE_FLOAT && column->bsize == 8)
      {
        double min = *(double*)(get_row(block, 0)+offsets[i]);
        double max = min;
        for (j = 0 ; j < block->num_rows ; j++)
        {
          void * row = get_row(block, j);
          double value = *(double*)(row+offsets[i]);
          if (value > max) max = value;
          if (value < min) min = value;
        }
        fprintf(stderr, "%s: %lf to %lf\n", column_get_name(column), min, max);
      }
      else if (column->type == TYPE_CHAR)
      {
        int max = 0, count = 0;
        for (j = 0 ; j < block->num_rows ; j++)
        {
          void * row = get_row(block, j);
          int value = strlen((char*)(row+offsets[i]));
          if (value > max) { max = value; count = 1; }
          else if (value == max) count++;
        }
        fprintf(stderr, "%s: max strlen = %d (field size is %d), %d rows at that length\n", column_get_name(column), max, column->bsize, count);
      }
      else
        fprintf(stderr, "doesn't do %s %d\n", column_get_name(column), column->type);
    }
    free_block(block);
  }
}
