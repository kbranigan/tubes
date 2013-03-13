
#include "block.h"

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
  
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	static char table[1000] = "";
	static int drop = 0;
	
	struct Params * params = NULL;
	params = add_string_param(params, "table", 't', table, 1);
	params = add_flag_param(params, "drop", 'd', &drop, 0);
	eval_params(params, argc, argv);
	
	if (drop) fprintf(stdout, "DROP TABLE IF EXISTS `%s`;\n", table);
	struct Block * block = NULL;
	while ((block = read_block(stdin)))
	{
		int row_id, column_id;
		fprintf(stdout, "CREATE TABLE IF NOT EXISTS `%s` (id int PRIMARY KEY AUTO_INCREMENT", table);
		for (column_id = 0 ; column_id < block->num_columns ; column_id++)
		{
			struct Column * column = get_column(block, column_id);
			if      (column->type == TYPE_INT)   fprintf(stdout, ", `%s` int", column_get_name(column));
			else if (column->type == TYPE_UINT)  fprintf(stdout, ", `%s` int", column_get_name(column));
			else if (column->type == TYPE_FLOAT) fprintf(stdout, ", `%s` double", column_get_name(column));
			else if (column->type == TYPE_CHAR)  fprintf(stdout, ", `%s` varchar(%d)", column_get_name(column), column->bsize);
		}
		fprintf(stdout, ");\n");
		
		char insert_header[1000] = "";
		sprintf(insert_header, "INSERT INTO `%s` (", table);
		for (column_id = 0 ; column_id < block->num_columns ; column_id++)
		{
			sprintf(&insert_header[strlen(insert_header)], "`%s`", column_get_name(get_column(block, column_id)));
			if (column_id != block->num_columns-1) sprintf(&insert_header[strlen(insert_header)], ",");
		}
		sprintf(&insert_header[strlen(insert_header)], ") VALUES ");
		
		for (row_id = 0 ; row_id < block->num_rows ; row_id++)
		{
			if (row_id%100==0) fprintf(stdout, "%s%s (", (row_id==0?"":";\n"), insert_header);
			else fprintf(stdout, ",(");
			for (column_id = 0 ; column_id < block->num_columns ; column_id++)
			{
				struct Column * column = get_column(block, column_id);
				if (column->type == TYPE_CHAR)
				{
					char * source = (char*)get_cell(block, row_id, column_id);
					char * destination = (char*)malloc(column->bsize*2+1);
					int i = 0, j = 0;
					for (i = 0 ; i < column->bsize ; i++)
					{
						if (source[i] == '"')
							destination[j++] = '\\';
						if (source[i] == '\\')
							destination[j++] = '\\';
						destination[j] = source[i];
						j++;
					}
					destination[column->bsize*2] = 0;
					fprintf(stdout, "\"%s\"", destination);
					free(destination);
				}
				else
				{
					fprintf_cell(stdout, block, row_id, column_id);
				}
				if (column_id != block->num_columns-1) fprintf(stdout, ",");
			}
			fprintf(stdout, ")");
			//break;
		}
		fprintf(stdout, ";\n");
		
		free_block(block);
	}
}
