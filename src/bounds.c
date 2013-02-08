
#include "block.h"
#include <alloca.h>
#include <float.h>

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	//assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	static char clamp_bbox_char[1000] = "";
	static int debug = 0;
	static double clamp_bbox[2][2] = {{FLT_MAX, -FLT_MAX}, {FLT_MAX, -FLT_MAX}};
	
	int c;
	while (1) {
		static struct option long_options[] = {
			{"bbox", required_argument, 0, 'b'},
			{"debug", no_argument, &debug, 1},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "b:", long_options, &option_index);
		if (c == -1) break;
		
		switch (c) {
			case 0: break;
			case 'b': strncpy(clamp_bbox_char, optarg, sizeof(clamp_bbox_char)); break;
			default: abort();
		}
	}
	
	if (clamp_bbox_char[0] != 0) {
		char * ptr = strtok(clamp_bbox_char, " ,");
		if (ptr == NULL) fprintf(stderr, "ERROR: Usage: %s --bbox=\"0,1,0,1\"	(min_x,max_x,min_y,max_y)", argv[0]);
		clamp_bbox[0][0] = atof(ptr);
		ptr = strtok(NULL, " ,");
		if (ptr == NULL) fprintf(stderr, "ERROR: Usage: %s --bbox=\"0,1,0,1\"	(min_x,max_x,min_y,max_y)", argv[0]);
		clamp_bbox[0][1] = atof(ptr);
		ptr = strtok(NULL, " ,");
		if (ptr == NULL) fprintf(stderr, "ERROR: Usage: %s --bbox=\"0,1,0,1\"	(min_x,max_x,min_y,max_y)", argv[0]);
		clamp_bbox[1][0] = atof(ptr);
		ptr = strtok(NULL, " ,");
		if (ptr == NULL) fprintf(stderr, "ERROR: Usage: %s --bbox=\"0,1,0,1\"	(min_x,max_x,min_y,max_y)", argv[0]);
		clamp_bbox[1][1] = atof(ptr);
		
		fprintf(stderr, "clamp x,y to: {{%f,%f},{%f,%f}}\n", clamp_bbox[0][0], clamp_bbox[0][1], clamp_bbox[1][0], clamp_bbox[1][1]);
	}
	
	struct Block * block = NULL;
	while ((block = read_block(stdin))) {
		
		if (clamp_bbox_char[0] != 0) {
			block = add_command(block, argc, argv);
		}
		
		double bbox[2][2] = {{FLT_MAX, -FLT_MAX}, {FLT_MAX, -FLT_MAX}};
		
		int i;
		for (i = 0 ; i < block->num_rows ; i++) {
			double x = get_x(block, i);
			double y = get_y(block, i);
			
			if (x < bbox[0][0]) bbox[0][0] = x; if (x > bbox[0][1]) bbox[0][1] = x;
			if (y < bbox[1][0]) bbox[1][0] = y; if (y > bbox[1][1]) bbox[1][1] = y;
		}
		
		if (clamp_bbox_char[0] != 0) {
			double dx = clamp_bbox[0][1] - clamp_bbox[0][0];
			double dy = clamp_bbox[1][1] - clamp_bbox[1][0];
			for (i = 0 ; i < block->num_rows ; i++) {
				double x = get_x(block, i);
				double y = get_y(block, i);
				//fprintf(stderr, "%f inside %f,%f (%f), dx = %f\n", x, bbox[0][0], bbox[0][1], ((x-bbox[0][0])/(bbox[0][1]-bbox[0][0])), dx);
				//fprintf(stderr, "%f,%f\n", x, y);
				set_xy(block, i, ((x-bbox[0][0])/(bbox[0][1]-bbox[0][0]))*dx+clamp_bbox[0][0], ((y-bbox[1][0])/(bbox[1][1]-bbox[1][0]))*dy+clamp_bbox[1][0]);
				x = get_x(block, i);
				y = get_y(block, i);
				//fprintf(stderr, "becomes %f,%f\n", x, y);
			}
		} else {
		
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
		}
		
		if (stdout_is_piped()) {
			write_block(stdout, block);
		}
		
		free_block(block);
	}
}
