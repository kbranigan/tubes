
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/block.h"
#include "../src/block_kdtree.h"

int main(int argc, char ** argv)
{
  fprintf(stderr, "%s has some minor problems that need investigating - joins to the wrong segments.\n", argv[0]);
  
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    //block = set_num_rows(block, 500);
    block = add_command(block, argc, argv);
    
    block = add_double_column(block, "radius");
    blank_column_values(block, "radius");
    
    void * kdtree = create_kdtree_for_block(block);
    
    int32_t x_column_id = get_column_id_by_name(block, "x");
    int32_t y_column_id = get_column_id_by_name(block, "y");
    int32_t nn_column_id = get_column_id_by_name(block, "radius");
    
    int row_id = 0;
    for (row_id = 0 ; row_id < block->num_rows ; row_id++)
    {
      double x = get_cell_as_double(block, row_id, x_column_id);
      double y = get_cell_as_double(block, row_id, y_column_id);
      int32_t nn_row_id = search_kdtree_find_nearest_not_row_id(kdtree, x, y, row_id);
      if (nn_row_id != -1 && nn_row_id)
      {
        double nn_x = get_cell_as_double(block, nn_row_id, x_column_id);
        double nn_y = get_cell_as_double(block, nn_row_id, y_column_id);
        double distance = sqrt((nn_x-x)*(nn_x-x) + (nn_y-y)*(nn_y-y));
        if (distance < 0.00005) distance = 0.00005;
        double * nn_cell = (double*)get_cell(block, row_id, nn_column_id);
        *(nn_cell) = distance;
      }
    }
    
    write_block(stdout, block);
    free_block(block);
  }
}

