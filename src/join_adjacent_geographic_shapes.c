
#include "block.h"
#include <math.h>

struct Block * each_block(struct Block * block);
int each_shape(struct Block * block, uint32_t shape_start_id, uint32_t shape_end_id);
int each_part(struct Block * block, uint32_t part_start_id, uint32_t part_end_id);

/*

The design of this script is _very_ specific.  It takes multiple blocks, and assumes they are all line_loops.
If any line loop shares a two point edge with a line loop in the following block then merge them, using that edge as the joint.

Two points are considered the same if they are within 0.001% of the bbox width/height.

Also, it compares on red, green and blue, if they exist.

*/

struct Block * prev_block = NULL;

struct Block * each_block(struct Block * block) {
	
	if (prev_block == NULL) {
		prev_block = block;
	} else {
		foreach_shape(block, &each_shape);
		write_block(stdout, prev_block);
		free_block(prev_block);
	}
	return block;
}

int each_shape(struct Block * block, uint32_t shape_start_id, uint32_t shape_end_id) {
	foreach_part(block, shape_start_id, shape_end_id, &each_part);
	return 0;
}

int each_part(struct Block * block, uint32_t part_start_id, uint32_t part_end_id) {
	if (prev_block == NULL || prev_block == block) return 0;
	
	int i, j, i2, j2;
	for (i = part_start_id ; i < part_end_id-1 ; i++) {
		int i2 = (i == part_start_id ? part_end_id-1 : i-1);
		double x1 = get_x(block, i),  y1 = get_y(block, i),
					 x2 = get_x(block, i2), y2 = get_y(block, i2);
		
		// foreach prev_block shape
		int prev_shape_start_id = 0, prev_shape_end_id;
		while ((prev_shape_end_id = get_next_shape_start(prev_block, prev_shape_start_id))) {
			//int shape_row_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
			
			// foreach part of shape
			int prev_part_start_id = prev_shape_start_id, prev_part_end_id;
			while ((prev_part_end_id = get_next_part_start(prev_block, prev_part_start_id))) {
				//int shape_part_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
				
				for (j = prev_part_start_id ; j < prev_part_end_id-1 ; j++) {
					int j2 = (j == prev_part_end_id-1 ? prev_part_start_id : j+1);
					double px1 = get_x(prev_block, j),  py1 = get_y(prev_block, j),
								 px2 = get_x(prev_block, j2), py2 = get_y(prev_block, j2);
					
					if ((fabs(x1-px1) < 0.000001 && fabs(y1-py1) < 0.000001) && 
							(fabs(x2-px2) < 0.000001 && fabs(y2-py2) < 0.000001)) {
						if (i > i2) {
							//fprintf(stderr, "haha\n");
						} else if (j < j2) {
							
							fprintf(stderr, "insert block between %d and %d of prev_block\n", j, j2);
							int old_prev_block_num_rows = prev_block->num_rows;
							prev_block = set_num_rows(prev_block, prev_block->num_rows + block->num_rows - 2);
							memmove(get_row(prev_block, j + block->num_rows - 1), get_row(prev_block, j2), prev_block->row_bsize * (old_prev_block_num_rows - j + 1));
							
							int32_t shape_row_id_column_id = get_column_id_by_name(prev_block, "shape_row_id");
							int32_t shape_row_id = get_cell_as_int32(prev_block, j, shape_row_id_column_id);
							
							int k;
							for (k = j2 ; k < j2 + block->num_rows ; k++) {
								memcpy(get_row(prev_block, k), get_row(block, k-j2), prev_block->row_bsize);
								set_cell_from_int32(prev_block, k, shape_row_id_column_id, shape_row_id);
							}
							
						} else {
							fprintf(stderr, "ERROR: uh oh\n");
						}
					}
				}
				
				if (prev_part_end_id == prev_shape_end_id) {
					break; // last part of shape
				}
				prev_part_start_id = prev_part_end_id;
			}
			
			if (prev_shape_end_id == block->num_rows) {
				break; // last shape
			}
			prev_shape_start_id = prev_shape_end_id;
		}
		
	}
	
	return 0;
}

int main(int argc, char ** argv) {
	
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	add_command_in_foreach(argc, argv);
	
	foreach_block(stdin, NULL, 0, &each_block);
	
}












