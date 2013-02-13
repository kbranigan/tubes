
#include "block.h"
#include <math.h>

/*

The design of this script is _very_ specific.  It takes multiple blocks, and assumes they are all line_loops.
If any line loop shares a two point edge with a line loop in the following block then merge them, using that edge as the joint.

Two points are considered the same if they are within 0.001% of the bbox width/height.

Also, it compares on red, green and blue, if they exist.

*/

int main(int argc, char ** argv) {
	
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	/*
	static char filename[1000] = "";
	static int debug = 0;
	
	int c;
	while (1) {
		static struct option long_options[] = {
			{"filename", required_argument, 0, 'f'},
			{"debug", no_argument, &debug, 1},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "d:f:", long_options, &option_index);
		if (c == -1) break;
		
		switch (c) {
			case 0: break;
			case 'f': strncpy(filename, optarg, sizeof(filename)); break;
			default: abort();
		}
	}
	*/
	
	struct Block * prev_block = NULL;
	struct Block * block = NULL;
	while ((block = read_block(stdin))) {
		
		if (prev_block != NULL) {
			
			int i, j;
			for (i = 0 ; i < block->num_rows ; i++) {
				double x = get_x(block, i), y = get_y(block, i);
				for (j = 0 ; j < prev_block->num_rows ; j++) {
					double px = get_x(prev_block, j), py = get_y(prev_block, j);
					if (fabs(x-px) < 0.000001 && fabs(y-py) < 0.000001) {
						
						fprintf(stderr, "%d,%d (%f, %f)\n", i, j, x, y);
						
						
					}
				}
			}
			
			/*
			int prev_shape_start_id = 0, prev_shape_end_id;
			while ((prev_shape_end_id = get_next_shape_start(prev_block, prev_shape_start_id))) {
				//int shape_row_id = get_cell_as_int32(prev_block, shape_start_id, shape_row_id_column_id);
				//fprintf(stderr, "%d-%d\n", prev_shape_start_id, prev_shape_end_id);
				
				// foreach part of shape
				int prev_part_start_id = prev_shape_start_id, prev_part_end_id;
				while ((prev_part_end_id = get_next_part_start(prev_block, prev_part_start_id))) {
					//int shape_part_id = get_cell_as_int32(prev_block, shape_start_id, shape_row_id_column_id);
					//fprintf(stderr, "  %d-%d\n", prev_part_start_id, prev_part_end_id);
					
					////////////////////////////////////////////////////////////////////////////////////
					
					int shape_start_id = 0, shape_end_id;
					while ((shape_end_id = get_next_shape_start(block, shape_start_id))) {
						//int shape_row_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
						//fprintf(stderr, "%d-%d\n", shape_start_id, shape_end_id);
						
						// foreach part of shape
						int part_start_id = shape_start_id, part_end_id;
						while ((part_end_id = get_next_part_start(block, part_start_id))) {
							//int shape_part_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
							//fprintf(stderr, "  %d-%d\n", part_start_id, part_end_id);
							
							////////////////////////////////////////////////////////////////////////////////////
							
							int i, j;
							for (i = prev_part_start_id ; i < prev_part_end_id ; i++) {
								double px = get_x(prev_block, i), py = get_y(prev_block, i);
								
								for (j = part_start_id ; i < part_end_id ; i++) {
									double x = get_x(block, j), y = get_y(block, j);
									
									//fprintf(stderr, "%f %f %f %f\n", px, x, py, y);
									
									if (fabs(43.718605-y) < 0.0001) { 
										fprintf(stderr, ";");
									}
									
									if (fabs(px-x) < 0.001 && fabs(py-y) < 0.001) {
										fprintf(stderr, ".");
									}
									//break;
								}
								//break;
							}
							
							////////////////////////////////////////////////////////////////////////////////////
							
							if (part_end_id == shape_end_id) {
								break; // last part of shape
							}
							part_start_id = part_end_id;
						}
						
						if (shape_end_id == block->num_rows) {
							break; // last shape
						}
						shape_start_id = shape_end_id;
					}
					
					////////////////////////////////////////////////////////////////////////////////////
					
					if (prev_part_end_id == prev_shape_end_id) {
						break; // last part of prev_shape
					}
					prev_part_start_id = prev_part_end_id;
				}
				
				if (prev_shape_end_id == prev_block->num_rows) {
					break; // last prev_shape
				}
				prev_shape_start_id = prev_shape_end_id;
			}*/
		} else {
			
		}
		
		//write_block(stdout, block);
		prev_block = block;
	}
	if (block != NULL) {
		//write_block(stdout, block);
		//free_block(block);
	}
}












