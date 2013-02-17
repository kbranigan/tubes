
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
	
	fprintf(fp, "var data = {\n");
	fprintf(fp, "  \"data\": [\n");
	
	struct Block * block = NULL;
	while ((block = read_block(stdin)))
	{
		int shape_row_id_column_id  = get_column_id_by_name(block, "shape_row_id");
		int shape_part_id_column_id = get_column_id_by_name(block, "shape_part_id");
		int red_column_id   = get_column_id_by_name(block, "red");
		int green_column_id = get_column_id_by_name(block, "green");
		int blue_column_id  = get_column_id_by_name(block, "blue");
		int alpha_column_id = get_column_id_by_name(block, "alpha");
		
		int name_column_id  = -1;
		if (name_column_name[0] != 0) {
			name_column_id = get_column_id_by_name(block, name_column_name);
			fprintf(stderr, "name_column_name = %s, name_column_id = %d\n", name_column_name, name_column_id);
		}
		
		int shape_start_id = 0, shape_end_id;
		while ((shape_end_id = get_next_shape_start(block, shape_start_id)))
		{
			int shape_row_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
			
			int red = 0, green = 0, blue = 0, alpha = 1;
			if (red_column_id != -1)   red   = 255 * get_cell_as_double(block, shape_start_id, red_column_id);
			if (green_column_id != -1) green = 255 * get_cell_as_double(block, shape_start_id, green_column_id);
			if (blue_column_id != -1)  blue  = 255 * get_cell_as_double(block, shape_start_id, blue_column_id);
			if (alpha_column_id != -1) alpha = 255 * get_cell_as_double(block, shape_start_id, alpha_column_id) * 0.5;
			
			fprintf(fp, "    {\n");
			if (name_column_id != -1) {
				fprintf(fp, "      \"name\":\"%s\",\n", (char*)get_cell(block, shape_start_id, name_column_id));
			}
			fprintf(fp, "      \"parts\":[\n");
			
			//fprintf(stderr, "shape = %d to %d (#%d)\n", shape_start_id, shape_end_id, shape_row_id);
			
			int part_start_id = shape_start_id, part_end_id;
			while ((part_end_id = get_next_part_start(block, part_start_id)))
			{
				int shape_part_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
				
				fprintf(fp, "        [");
				int i;
				for (i = part_start_id ; i < part_end_id ; i++)
				{
					fprintf(fp, "%.6f,%.6f%s", get_x(block, i), get_y(block, i), (i != part_end_id-1) ? "," : "");
					if (i % 10000 == 9999) { fprintf(fp, "\n"); }
				}
				
				fprintf(fp, "]\n");
				//fprintf(stderr, "	part = %d to %d (#%d)\n", part_start_id, part_end_id, shape_part_id);
				if (part_end_id == shape_end_id) break;
				fprintf(fp, ",");
				part_start_id = part_end_id;
			}
			
			fprintf(fp, "      ]\n");
			fprintf(fp, "    }\n");
			
			if (shape_end_id == block->num_rows) break;
			fprintf(fp, ",");
			shape_start_id = shape_end_id;
		}
	}
	fprintf(fp, "  ]\n");
	fprintf(fp, "};\n");
	
}
