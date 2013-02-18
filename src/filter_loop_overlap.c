
#include "block.h"
#include <math.h>

int main(int argc, char ** argv) {
	
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	static int debug = 0;
	static double distance = 0.000001;
	
	int c;
	while (1) {
		static struct option long_options[] = {
			//{"filename", required_argument, 0, 'f'},
			{"distance", required_argument, 0, 'd'},
			//{"header", no_argument, &output_header, 1},
			//{"no-header", no_argument, &output_header, 0},
			{"debug", no_argument, &debug, 1},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "d:", long_options, &option_index);
		if (c == -1) break;
		
		switch (c) {
			case 0: break;
			case 'd': distance = atof(optarg); break;
			default: abort();
		}
	}
	
	
	struct Block * block = NULL;
	while ((block = read_block(stdin))) {
		
		const char * shape_type = get_attribute_value_as_string(block, "shape_type");
		
		int is_line_loop = 0;
		if (shape_type != NULL && strcmp(shape_type, "line_loop") == 0) {
			is_line_loop = 1;
		}
		
		int filter_count = 0;
		
		// foreach shape
		int shape_start_id = 0, shape_end_id;
		while ((shape_end_id = get_next_shape_start(block, shape_start_id))) {
			//int shape_row_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
			
			// foreach part of shape
			int part_start_id = shape_start_id, part_end_id;
			while ((part_end_id = get_next_part_start(block, part_start_id))) {
				//int shape_part_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
				
				int i, j;
				// find if there is any overlap in the end of the shape 
				double x = get_x(block, part_start_id), y = get_y(block, part_start_id);
				for (i = part_end_id-1 ; i > part_start_id ; i--) {
					double px = get_x(block, i), py = get_y(block, i);
					if (fabs(x-px) < distance && fabs(y-py) < distance) {
						// found a match some place with the first point, continue and see if it goes all the way
						fprintf(stderr, "matches %d with %d (shape has %d points)\n", part_start_id, i, part_end_id - part_start_id);
						for (j = i ; j < part_end_id ; j++) {
							fprintf(stderr, "%d\n", j);
						}
						break;
					}
				}
				
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
		
		char temp[1000];
		sprintf(temp, "filtered %d rows by distance < %f", filter_count, distance);
		block = add_string_attribute(block, "filter", temp);
		
		write_block(stdout, block);
		free_block(block);
	}
}
