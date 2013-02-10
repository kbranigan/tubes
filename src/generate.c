
#include "block.h"
#include <math.h>

int main(int argc, char ** argv) {
	
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	//assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	static char shape[100] = "circle";
	static int debug = 0;
	
	int c;
	while (1) {
		static struct option long_options[] = {
			{"shape", required_argument, 0, 's'},
			{"debug", no_argument, &debug, 1},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "s:", long_options, &option_index);
		if (c == -1) break;
		
		switch (c) {
			case 0: break;
			case 's': strncpy(shape, optarg, sizeof(shape)); break;
			default: abort();
		}
	}
	
	struct Block * block = new_block();
	block = add_string_attribute(block, "shape_type", "line_loop");
	block = add_xy_columns(block);
	block = add_int32_column(block, "shape_row_id");
	block = add_int32_column(block, "shape_part_id");
	
	if (strcmp(shape, "circle")==0) {
		int i;
		for (i = 0 ; i < 50 ; i++) {
			block = add_row_and_blank(block);
			set_xy(block, block->num_rows-1, cos((i/50.0)*(3.14159265*2.0)), sin((i/50.0)*(3.14159265*2.0)));
		}
	} else if (strcmp(shape, "square")==0) {
		block = add_row_and_blank(block);
		set_xy(block, block->num_rows-1, -1, -1);
		block = add_row_and_blank(block);
		set_xy(block, block->num_rows-1, -1, 1);
		block = add_row_and_blank(block);
		set_xy(block, block->num_rows-1, 1, 1);
		block = add_row_and_blank(block);
		set_xy(block, block->num_rows-1, 1, -1);
	}
	
	write_block(stdout, block);
	free_block(block);
}
