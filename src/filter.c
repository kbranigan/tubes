
#include "block.h"
#include <math.h>

enum OPERATOR {
	OPERATOR_DELETE = 1,
	OPERATOR_PASS = 2
};

char operator_names[3][40] = {
	"unknown", "delete", "pass"
};

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	static char column_name[1000] = "";
	static char value[1000] = "";
	static char operator_as_string[1000] = "PASS";
	static enum OPERATOR operator = OPERATOR_PASS;
	static int debug = 0;
	
	struct Params * params = NULL;
	params = add_string_param(params, "column", 'c', column_name, 1);
	params = add_string_param(params, "value", 'v', value, 0);
	params = add_string_param(params, "operator", 'o', operator_as_string, 0);
	params = add_flag_param(params, "debug", 'd', &debug, 0);
	eval_params(params, argc, argv);
	
	if (strcmp(operator_as_string, "PASS")==0) {
		operator = OPERATOR_PASS;
	} else if (strcmp(operator_as_string, "DELETE")==0) {
		operator = OPERATOR_DELETE;
	} else {
		fprintf(stderr, "Invalid operator '%s', must be PASS or DELETE\n", optarg);
		exit(0);
	}
	
	int * ivalues = NULL;
	int num_ivalues = 0;
	
	int i,j;
	struct Block * block = NULL;
	while ((block = read_block(stdin))) {
		int column_id = get_column_id_by_name_or_exit(block, column_name);
		
		struct Column * column = get_column(block, column_id);
		if (column->type != TYPE_CHAR && 
				(column->type != TYPE_INT || column->bsize != 4) && 
				(column->type != TYPE_FLOAT)) {
			fprintf(stderr, "column '%s' is not a string or int32_t or float or double.\n", column_name);
			write_block(stdout, block);
			free_block(block);
			continue;
		}
		
		struct Block * newblock = new_block();
		newblock = copy_all_attributes(newblock, block);
		
		if (column->type == TYPE_INT) {
			if (ivalues != NULL) {
				free(ivalues);
				num_ivalues = 0;
			}
			char * ptr = strtok(value, ",");
			while (ptr != NULL) {
				num_ivalues++;
				ivalues = (int*)realloc(ivalues, sizeof(int)*num_ivalues);
				ivalues[num_ivalues-1] = atoi(ptr);
				ptr = strtok(NULL, ",");
			}
		}
		double fvalue;
		if (column->type == TYPE_FLOAT) {
			fvalue = atof(value);
		}
		
		for (i = 0 ; i < block->num_rows ; i++) {
			char * cell = (char*)get_cell(block, i, column_id);
			
			if (column->type == TYPE_INT && column->bsize == 4) {
				int j;
				for (j = 0 ; j < num_ivalues ; j++) {
					if ((operator == OPERATOR_DELETE && (*(int32_t*)cell) != ivalues[j]) || 
							(operator == OPERATOR_PASS	 && (*(int32_t*)cell) == ivalues[j]))
					newblock->num_rows++;
				}
			} else if (column->type == TYPE_FLOAT && column->bsize == 4) {
				float temp = (*(float*)cell);
				if ((operator == OPERATOR_DELETE && fabs(temp-fvalue) > 0.000001) || 
						(operator == OPERATOR_PASS	 && fabs(temp-fvalue) < 0.000001))
				newblock->num_rows++;
			} else if (column->type == TYPE_FLOAT && column->bsize == 8) {
				double temp = (*(double*)cell);
				if ((operator == OPERATOR_DELETE && fabs(temp-fvalue) > 0.000001) || 
						(operator == OPERATOR_PASS	 && fabs(temp-fvalue) < 0.000001))
				newblock->num_rows++;
			} else if (column->type == TYPE_CHAR) {
				int comp = strncmp(value, cell, column->bsize);
				//if (newblock->num_rows < 5) fprintf(stderr, "%d: %s vs %s (%d) (%d)\n", i, cell, value, column->bsize, comp);
				if ((operator == OPERATOR_DELETE && comp != 0) ||
						(operator == OPERATOR_PASS	 && comp == 0)) {
					//if (newblock->num_rows < 5) fprintf(stderr, "hi %d: %s vs %s (%d)\n", i, cell, value, column->bsize);
					newblock->num_rows++;
				}
			}
		}
		int new_num_rows = newblock->num_rows;
		newblock->num_rows = 0;
		if (debug) {
			fprintf(stderr, "%d - %d = %d\n", block->num_rows, block->num_rows-new_num_rows, new_num_rows);
		}
		
		char temp[1000];
		sprintf(temp, "'%s' %s '%s' (removing %d rows)", column_name, operator_names[operator], value, block->num_rows - new_num_rows);
		newblock = add_string_attribute(newblock, "filter", temp);
		newblock = copy_all_columns(newblock, block);
		
		newblock->num_rows = new_num_rows;
		newblock = set_num_rows(newblock, newblock->num_rows);
		
		int newblock_row_id = 0;
		for (i = 0 ; i < block->num_rows ; i++) {
			char * cell = (char*)get_cell(block, i, column_id);
			
			if (column->type == TYPE_INT && column->bsize == 4) {
				int j;
				for (j = 0 ; j < num_ivalues ; j++) {
					if ((operator == OPERATOR_DELETE && (*(int32_t*)cell) != ivalues[j]) || 
							(operator == OPERATOR_PASS	 && (*(int32_t*)cell) == ivalues[j])) {
						memcpy(get_row(newblock, newblock_row_id), get_row(block, i), block->row_bsize);
						newblock_row_id++;
					}
				}
			} else if (column->type == TYPE_FLOAT && column->bsize == 4) {
				float temp = (*(float*)cell);
				if ((operator == OPERATOR_DELETE && fabs(temp-fvalue) > 0.000001) || 
						(operator == OPERATOR_PASS	 && fabs(temp-fvalue) < 0.000001)) {
					memcpy(get_row(newblock, newblock_row_id), get_row(block, i), block->row_bsize);
					newblock_row_id++;
				}
			} else if (column->type == TYPE_FLOAT && column->bsize == 8) {
				double temp = (*(double*)cell);
				if ((operator == OPERATOR_DELETE && fabs(temp-fvalue) > 0.000001) || 
						(operator == OPERATOR_PASS	 && fabs(temp-fvalue) < 0.000001)) {
					memcpy(get_row(newblock, newblock_row_id), get_row(block, i), block->row_bsize);
					newblock_row_id++;
				}
			} else if (column->type == TYPE_CHAR) {
				int comp = strncmp(value, cell, column->bsize);
				if ((operator == OPERATOR_DELETE && comp != 0) ||
						(operator == OPERATOR_PASS	 && comp == 0)) {
					memcpy(get_row(newblock, newblock_row_id), get_row(block, i), block->row_bsize);
					newblock_row_id++;
				}
			}
		}
		
		write_block(stdout, newblock);
		free_block(newblock);
		free_block(block);
	}
	free(ivalues);
}
