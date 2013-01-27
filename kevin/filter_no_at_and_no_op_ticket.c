
#include "../src/block.h"

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	struct Block * block = NULL;
	while ((block = read_block(stdin)))
	{
		struct Block * newblock = new_block();
		newblock = copy_all_attributes(newblock, block);
		newblock = copy_all_columns(newblock, block);
		
		int num_AT_meter_tickets_column_id = get_column_id_by_name(newblock, "num_AT_meter_tickets");
		int num_OP_meter_tickets_column_id = get_column_id_by_name(newblock, "num_OP_meter_tickets");
		
		int new_num_rows = 0;
		int block_row_id;
		for (block_row_id = 0 ; block_row_id < block->num_rows ; block_row_id++)
		{
			int32_t num_AT_meter_tickets = get_cell_as_int32(block, block_row_id, num_AT_meter_tickets_column_id);
			int32_t num_OP_meter_tickets = get_cell_as_int32(block, block_row_id, num_OP_meter_tickets_column_id);
			if (num_AT_meter_tickets > 0 || num_OP_meter_tickets > 0)
			{
				new_num_rows++;
			}
		}
		
		fprintf(stderr, "new_num_rows = %d\n", new_num_rows);
		
		newblock = set_num_rows(newblock, new_num_rows);
		
		int newblock_row_id = 0;
		for (block_row_id = 0 ; block_row_id < block->num_rows ; block_row_id++)
		{
			int32_t num_AT_meter_tickets = get_cell_as_int32(block, block_row_id, num_AT_meter_tickets_column_id);
			int32_t num_OP_meter_tickets = get_cell_as_int32(block, block_row_id, num_OP_meter_tickets_column_id);
			if (num_AT_meter_tickets > 0 || num_OP_meter_tickets > 0)
			{
				memcpy(get_row(newblock, newblock_row_id), get_row(block, block_row_id), block->row_bsize);
				newblock_row_id++;
			}
		}
		
		write_block(stdout, newblock);
		free_block(newblock);
		free_block(block);
	}
}







