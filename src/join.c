
#include "block.h"

#define LEFT_JOIN 1
#define INNER_JOIN 2
#define RIGHT_JOIN 3

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	static int join = INNER_JOIN;
	static char join_as_string[100] = "";
	static char right_file[1000] = "";
	static char left_column[100] = "";
	static char right_column[100] = "";
	static int debug = 0;
	
	struct Params * params = NULL;
	params = add_string_param(params, "join", 'j', join_as_string, 1);
	params = add_string_param(params, "right_file", 'f', right_file, 1);
	params = add_string_param(params, "left_column", 'l', left_column, 1);
	params = add_string_param(params, "right_column", 'r', right_column, 1);
	eval_params(params, argc, argv);
	
	if (strcmp(join_as_string, "left")==0) {
		join = LEFT_JOIN;
	} else if (strcmp(join_as_string, "inner")==0) {
		join = INNER_JOIN;
	} else if (strcmp(join_as_string, "right")==0) {
		join = RIGHT_JOIN;
	} else {
		fprintf(stderr, "invalid --join option, choices are: 'left', 'inner' or 'right'\n");
	}
	
	if (right_file[0] == 0 || left_column[0] == 0 || right_column[0] == 0)
	{
		fprintf(stderr, "Usage: cat left_table.b | ./join_inner --right_file=right_table.b --left_column=shape_row_id --right_column=first_hit_shape_row_id | ...");
		return EXIT_FAILURE;
	}
	
	FILE * fp = fopen(right_file, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "right_file '%s' is invalid.\n", right_file);
		return EXIT_FAILURE;
	}
	struct Block * left_block = read_block(stdin);
	
	int left_block_join_column_id = get_column_id_by_name(left_block, left_column);
	if (left_block_join_column_id == -1) { fprintf(stderr, "%s: left_block didn't have column '%s'\n", argv[0], left_column); return EXIT_FAILURE; }
	//fprintf(stderr, "left_block_join_column_id = %d\n", left_block_join_column_id);
	
	struct Block * right_block = NULL;
	while ((right_block = read_block(fp)))
	{
		int right_block_join_column_id = get_column_id_by_name(right_block, right_column);
		//fprintf(stderr, "right_block_join_column_id = %d\n", right_block_join_column_id);
		if (right_block_join_column_id == -1) {
			fprintf(stderr, "%s: right_block didn't have column '%s'\n", argv[0], right_column);
			free_block(right_block);
			continue;
		}
		
		struct Column * left_column = get_column(left_block, left_block_join_column_id);
		struct Column * right_column = get_column(right_block, right_block_join_column_id);
		
		if (left_column->type != right_column->type || left_column->bsize != right_column->bsize) {
			fprintf(stderr, "columns found, but they do not match type or bsize\n");
			free_block(right_block);
			continue;
		}
		
		struct Block * join_block = new_block();
		join_block = copy_all_columns(join_block, left_block);
		join_block = copy_all_attributes(join_block, left_block);
		join_block = add_command(join_block, argc, argv);
		
		int num_columns_from_right_block = 0;
		int * column_ids_from_right_block = NULL;
		
		int i, j, k;
		for (i = 0 ; i < right_block->num_columns ; i++) {
			struct Column * column = get_column(right_block, i);
			if (get_column_id_by_name(join_block, column_get_name(column)) == -1) {
				//fprintf(stderr, "add %s\n", column_get_name(column));
				join_block = _add_column(join_block, column->type, column->bsize, column_get_name(column));
				num_columns_from_right_block++;
				column_ids_from_right_block = realloc(column_ids_from_right_block, sizeof(int)*num_columns_from_right_block);
				column_ids_from_right_block[num_columns_from_right_block-1] = i;
			}
		}
		
		for (i = 0 ; i < left_block->num_rows ; i++) {
			int found = 0;
			for (j = 0 ; j < right_block->num_rows ; j++) {
				if (memcmp(get_cell(left_block, i, left_block_join_column_id), get_cell(right_block, j, right_block_join_column_id), left_column->bsize)==0) {
					join_block = add_row_and_blank(join_block);
					
					for (k = 0 ; k < left_block->num_columns ; k++) {
						memcpy(get_cell(join_block, join_block->num_rows-1, k), get_cell(left_block, i, k), get_column(left_block, k)->bsize);
					}
					//memcpy(get_row(join_block, join_block->num_rows-1), get_row(left_block, i), left_block->row_bsize);
					
					for (k = 0 ; k < num_columns_from_right_block ; k++) {
						//fprintf(stderr, "copy cell %d over\n", column_ids_from_right_block[k]);
						memcpy(get_cell(join_block, join_block->num_rows-1, left_block->num_columns + k), get_cell(right_block, j, column_ids_from_right_block[k]), get_column(right_block, column_ids_from_right_block[k])->bsize);
					}
					found++;
				}
			}
			if (found == 0) {
				join_block = add_row_and_blank(join_block);
				for (k = 0 ; k < left_block->num_columns ; k++) {
					memcpy(get_cell(join_block, join_block->num_rows-1, k), get_cell(left_block, i, k), get_column(left_block, k)->bsize);
				}
			}
		}
		
		write_block(stdout, join_block);
		free_block(right_block);
		free_block(join_block);
	}
	free_block(left_block);
	fclose(fp);
}












