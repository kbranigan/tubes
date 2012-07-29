
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
      switch (column->type)
      {
        case INT_TYPE:
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
          break;
        }
        case LONG_TYPE:
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
          break;
        }
        case FLOAT_TYPE:
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
          break;
        }
        case DOUBLE_TYPE:
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
          break;
        }
        default:
        {
          int max = 0;
          for (j = 0 ; j < block->num_rows ; j++)
          {
            void * row = get_row(block, j);
            int value = strlen((char*)(row+offsets[i]));
            if (value > max) max = value;
          }
          fprintf(stderr, "%s: max strlen = %d (field length is %d)\n", column_get_name(column), max, column->type);
          break;
        }
        fprintf(stderr, "hehehe %s %d\n", column_get_name(column), column->type);
        break;
      }
    }
    
    //inspect_block(block);
    
    free_block(block);
  }
}
