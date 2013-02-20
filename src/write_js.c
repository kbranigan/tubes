
#include "block.h"

int main(int argc, char ** argv)
{
	static char filename[1000] = "";
	static char name_column_name[100] = "";
	
	static int output_header = 1;
	static int output_quotes = 1;
	
	int c;
	while (1)
	{
		static struct option long_options[] = {
			{"filename", required_argument, 0, 'f'},
			{"name", required_argument, 0, 'n'},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "f:n:", long_options, &option_index);
		if (c == -1) break;
		
		switch (c)
		{
			case 0: break;
			case 'f': strncpy(filename, optarg, sizeof(filename)); break;
			case 'n': strncpy(name_column_name, optarg, sizeof(name_column_name)); break;
			default: abort();
		}
	}
	
	if (filename[0] == 0 && argc == 2 && argv[1] != NULL)
		strncpy(filename, argv[1], sizeof(filename));
	
	FILE * fp = (filename[0] != 0) ? fopen(filename, "w") : NULL;
	
	if (fp == NULL || filename[0] == 0)
	{
		fprintf(stderr, "ERROR: Usage: %s --filename=[file_name]\n", argv[0]);
		return -1;
	}
	
	fprintf(fp, "var blocks = {\n");
	fprintf(fp, "  \"blocks\": [\n");
	
	struct Block * block = NULL;
	while ((block = read_block(stdin)))
	{
		fprintf(fp, "    {\n");
		
		fprintf(fp, "      \"attributes\": [");
		int attr_id;
		for (attr_id = 0 ; attr_id < block->num_attributes ; attr_id++) {
			struct Attribute * attr = get_attribute(block, attr_id);
			fprintf(fp, "%s\n        [\"%s\", ", (attr_id == 0 ? "" : ","), attribute_get_name(attr));
			if (attr->type == TYPE_CHAR) {
				fprintf(fp, "\"");
			}
			fprintf_attribute_value(fp, block, attr_id); // suppose to addslashes, but I don't want to write that code right now
			if (attr->type == TYPE_CHAR) {
				fprintf(fp, "\"");
			}
			fprintf(fp, "]");
		}
		fprintf(fp, "\n      ],\n");
		fprintf(fp, "      \"columns\": [");
		int column_id;
		for (column_id = 0 ; column_id < block->num_columns ; column_id++) {
			struct Column * column = get_column(block, column_id);
			fprintf(fp, "%s\n        [\"%s\", \"%s\"]", (column_id == 0 ? "" : ","), column_get_name(column), get_type_name(column->type, column->bsize));
		}
		fprintf(fp, "\n      ],\n");
		
		int as_simple_shapes = 1; // kbfu
		if (as_simple_shapes) {
			fprintf(fp, "      \"shapes\": [");
			int shape_start_id = 0, shape_end_id;
			while ((shape_end_id = get_next_shape_start(block, shape_start_id))) {
				//int shape_row_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
				fprintf(fp, "%s\n        [", (shape_start_id==0) ? "" : ",");
				
				// foreach part of shape
				int part_start_id = shape_start_id, part_end_id;
				while ((part_end_id = get_next_part_start(block, part_start_id))) {
					//int shape_part_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
					fprintf(fp, "%s[", (part_start_id==shape_start_id) ? "" : ",");
					
					int i;
					for (i = part_start_id ; i < part_end_id ; i++) {
						fprintf(fp, "%s[%.6f,%.6f]", (i==part_start_id) ? "" : ",", get_x(block, i), get_y(block, i));
					}
					
					fprintf(fp, "]");
					if (part_end_id == shape_end_id) {
						break; // last part of shape
					}
					part_start_id = part_end_id;
				}
				
				fprintf(fp, "]");
				if (shape_end_id == block->num_rows) {
					break; // last shape
				}
				shape_start_id = shape_end_id;
			}
			fprintf(fp, "\n      ]\n");
		} else {
			fprintf(fp, "      \"data\": [");
			int row_id;
			for (row_id = 0 ; row_id < block->num_rows ; row_id++) {
				fprintf(fp, "%s\n        [", (row_id==0) ? "" : ",");
				for (column_id = 0 ; column_id < block->num_columns ; column_id++) {
					struct Column * column = get_column(block, column_id);
					if (column_id != 0) {
						fprintf(fp, ",");
					}
					if (column->type == TYPE_CHAR) {
						fprintf(fp, "\"");
					}
					fprintf_cell(fp, block, row_id, column_id);
					if (column->type == TYPE_CHAR) {
						fprintf(fp, "\"");
					}
				}
				fprintf(fp, "]");
			}
			fprintf(fp, "\n      ]\n");
		}
		
		/*
		int shape_start_id = 0, shape_end_id;
		while ((shape_end_id = get_next_shape_start(block, shape_start_id)))
		{
			int shape_row_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
			
			if (name_column_id != -1) {
				fprintf(fp, "      \"name\":\"%s\",\n", (char*)get_cell(block, shape_start_id, name_column_id));
			}
			fprintf(fp, "      \"parts\":[");
			
			//fprintf(stderr, "shape = %d to %d (#%d)\n", shape_start_id, shape_end_id, shape_row_id);
			
			int part_start_id = shape_start_id, part_end_id;
			while ((part_end_id = get_next_part_start(block, part_start_id)))
			{
				int shape_part_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
				
				fprintf(fp, "[");
				int i;
				for (i = part_start_id ; i < part_end_id ; i++)
				{
					fprintf(fp, "[%.6f,%.6f]%s", get_x(block, i), get_y(block, i), (i != part_end_id-1) ? "," : "");
					if (i % 10000 == 9999) { fprintf(fp, "\n"); }
				}
				
				fprintf(fp, "]");
				//fprintf(stderr, "	part = %d to %d (#%d)\n", part_start_id, part_end_id, shape_part_id);
				if (part_end_id == shape_end_id) break;
				fprintf(fp, ",");
				part_start_id = part_end_id;
			}
			
			fprintf(fp, "]\n");
			fprintf(fp, "    }%s", (shape_end_id == block->num_rows) ? "\n" : ",");
			
			if (shape_end_id == block->num_rows) break;
			//fprintf(fp, ",");
			shape_start_id = shape_end_id;
		}*/
		fprintf(fp, "    }\n");
		free_block(block);
	}
	fprintf(fp, "  ]\n");
	fprintf(fp, "};\n");
	
}
