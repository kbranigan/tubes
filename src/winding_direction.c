
#include "block.h"

static char direction[100] = "";

struct Block * block_level(struct Block * block) {
	//block = add_string_attribute(block, "winding_direction", "that_way");
	return block;
}

struct Block * winding_direction(struct Block * block, uint32_t shape_start_id, uint32_t shape_end_id, uint32_t part_start_id, uint32_t part_end_id) {
	int i;
	for (i = part_start_id ; i < part_end_id ; i++) {
		fprintf(stderr, ".");
	}
	
	// figure out winding direction and change it if needed
	
	return block;
}

int main(int argc, char ** argv) {
	
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	
	struct Params * params = NULL;
	params = add_string_param(params, "direction", 'd', direction);
	
	foreach_block(stdin, stdout, 1, &block_level, NULL, &winding_direction);
}
