

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "../src/block.h"
#include "../src/block_kdtree.h"

#pragma pack(1)
struct Row {
  double x;
  double y;
  int32_t GEO_ID;
  char ADDRESS[16];
  char NAME[44];
  char LF_NAME[32];
  char FULL_ADDRESS[52];
};

int main(int argc, char ** argv)
{
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    fprintf(stderr, "%ld vs %d\n", sizeof(struct Row), block->row_bsize);
    
    int i;
    for (i = 0 ; i < block->num_columns ; i++)
      fprintf(stderr, "%d: %ld\n", i, (long)get_cell(block, 0, i) - (long)get_cell(block, 0, 0));
    
    fprintf(stderr, "\n");
    
    fprintf(stderr, "0: %ld\n", offsetof(struct Row, x));
    fprintf(stderr, "1: %ld\n", offsetof(struct Row, y));
    fprintf(stderr, "2: %ld\n", offsetof(struct Row, GEO_ID));
    fprintf(stderr, "3: %ld\n", offsetof(struct Row, ADDRESS));
    fprintf(stderr, "4: %ld\n", offsetof(struct Row, NAME));
    fprintf(stderr, "5: %ld\n", offsetof(struct Row, LF_NAME));
    fprintf(stderr, "6: %ld\n", offsetof(struct Row, FULL_ADDRESS));
    
    //block = set_num_rows(block, 15);
    /*int block_num_tickets_column_id = get_column_id_by_name(block, "num_tickets");
    
    int total_num_tickets = 0;
    int row_id;
    for (row_id = 0 ; row_id < block->num_rows ; row_id++)
    {
      int num_tickets = get_cell_as_int(block, row_id, block_num_tickets_column_id);
      total_num_tickets += num_tickets;
    }
    
    fprintf(stderr, "total_num_tickets = %d\n", total_num_tickets);
    int32_t * cell_offsets = get_cell_offsets(block);
    int i;
    for (i = 0 ; i < block->num_columns ; i++)
      fprintf(stderr, "%d ", cell_offsets[i]);
    fprintf(stderr, "\n");
    block = add_int_column(block, "hi");
    for (i = 0 ; i < block->num_columns ; i++)
      fprintf(stderr, "%d ", cell_offsets[i]);
    fprintf(stderr, "\n");*/
    
    void *kdtree = create_kdtree_for_block(block);
    //struct kdtree_results
    
    //write_block(stdout, block);
    free_block(block);
  }
}
