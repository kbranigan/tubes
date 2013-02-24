
#include <math.h>
#include <float.h>
#include "block.h"

double sqr(double x) {
	return x*x;
}

double dist2(double v[2], double w[2]) { 
	return sqr(v[0] - w[0]) + sqr(v[1] - w[1]);
}

double perpendicular_distance(double p[2], double v[2], double w[2]) {
	double length = dist2(v, w);
	if (length == 0) return dist2(p, v);
	double t = ((p[0] - v[0]) * (w[0] - v[0]) + (p[1] - v[1]) * (w[1] - v[1])) / length;
	//if (t < 0) return dist2(p, v); // for line segment, vs infinite length line
	//if (t > 1) return dist2(p, w); // for line segment, vs infinite length line
	double g[2] = { v[0] + t * (w[0] - v[0]), v[1] + t * (w[1] - v[1]) }; // nearest edge point
	return sqrt(dist2(p, g));
}

void subdivide_and_test(struct Block * block, uint32_t * row_bitmask, double min_distance, uint32_t start, uint32_t stop) {
	//fprintf(stderr, "subdivide_and_test(%d, %d)\n", start, stop);
	if (start == stop || start == stop-1) {
		return;
	} else {
		
		double v[2] = { get_x(block, start), get_y(block, start) };
		double w[2] = { get_x(block, stop),  get_y(block, stop)  };
		
		row_bitmask[start] = 1;
		row_bitmask[stop] = 1;
		
		double max_dist = -FLT_MAX;
		int max_dist_index = -1;
		int i;
		for (i = start ; i < stop ; i++) {
			double p[2] = { get_x(block, i), get_y(block, i) };
			double dist = perpendicular_distance(p, v, w);
			if (dist > max_dist) {
				max_dist_index = i;
				max_dist = dist;
			}
		}
		//fprintf(stderr, "%d to %d = %d\n", start, stop, max_dist_index);
		row_bitmask[start] = 1;
		row_bitmask[stop] = 1;
		//uint32_t half = start + (int)floor((stop-start)/2.0);
		if (max_dist > min_distance) {
			row_bitmask[max_dist_index] = 1;
			subdivide_and_test(block, row_bitmask, min_distance, start, max_dist_index);
			subdivide_and_test(block, row_bitmask, min_distance, max_dist_index, stop);
		}
	}
}

int main(int argc, char ** argv) {
	
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	//static char filename[1000] = "";
	//static int output_header = 1;
	static double min_distance = 0.0001;
	static int debug = 0;
	
	int c;
	while (1) {
		static struct option long_options[] = {
			{"distance", required_argument, 0, 'd'},
			{"debug", no_argument, &debug, 1},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "d:", long_options, &option_index);
		if (c == -1) break;
		
		switch (c) {
			case 0: break;
			case 'd': min_distance = atof(optarg); break;
			default: abort();
		}
	}
	
	struct Block * block = NULL;
	while ((block = read_block(stdin))) {
		
		uint32_t * row_bitmask = (uint32_t *)malloc(sizeof(uint32_t *)*(block->num_rows+1));
		memset(row_bitmask, 0, sizeof(uint32_t *)*(block->num_rows+1));
		
		// foreach shape
		int shape_start_id = 0, shape_end_id;
		while ((shape_end_id = get_next_shape_start(block, shape_start_id))) {
			//int shape_row_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
			
			// foreach part of shape
			int part_start_id = shape_start_id, part_end_id;
			while ((part_end_id = get_next_part_start(block, part_start_id))) {
				//int shape_part_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
				
				double v[2] = { get_x(block, part_start_id), get_y(block, part_start_id) };
				
				double max_dist = -FLT_MAX;
				int max_dist_index = -1;
				
				int i;
				for (i = part_start_id ; i < part_end_id ; i++) {
					double w[2] = { get_x(block, i), get_y(block, i) };
					double dist = sqrt(dist2(v, w));
					if (dist > max_dist) {
						max_dist = dist;
						max_dist_index = i;
					}
				}
				
				subdivide_and_test(block, row_bitmask, min_distance, part_start_id, max_dist_index);
				subdivide_and_test(block, row_bitmask, min_distance, max_dist_index, part_end_id);
				
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
		
		struct Block * temp = new_block_from_row_bitmask(block, row_bitmask);
		free(row_bitmask);
		
		char temp_c[1000] = "";
		sprintf(temp_c, "reduced block of size %d down to %d points", block->num_rows, temp->num_rows);
		temp = add_string_attribute(temp, "simplify_shapes", temp_c);
		
		//write_block(stdout, block);
		write_block(stdout, temp);
		free_block(block);
		free_block(temp);
	}
}













