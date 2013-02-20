
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
			{"header", no_argument, &output_header, 1},
			{"no-header", no_argument, &output_header, 0},
			{"quotes", no_argument, &output_quotes, 1},
			{"no-quotes", no_argument, &output_quotes, 0},
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
	
	fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(fp, "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n");
	fprintf(fp, "<Document>\n");
	
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
			
			int red = 255, green = 255, blue = 255, alpha = 100;
			if (red_column_id != -1)   red   = 255 * get_cell_as_double(block, shape_start_id, red_column_id);
			if (green_column_id != -1) green = 255 * get_cell_as_double(block, shape_start_id, green_column_id);
			if (blue_column_id != -1)  blue  = 255 * get_cell_as_double(block, shape_start_id, blue_column_id);
			if (alpha_column_id != -1) alpha = 255 * get_cell_as_double(block, shape_start_id, alpha_column_id) * 0.5;
			
			fprintf(fp, "<Placemark>\n");
			if (name_column_id != -1) {
				fprintf(fp, "<name>%s</name>\n", (char*)get_cell(block, shape_start_id, name_column_id));
			}
			fprintf(fp, "<Style>\n");
			fprintf(fp, "<PolyStyle>\n");
			fprintf(fp, "<color>%02x%02x%02x%02x</color>\n", alpha, blue, green, red);
			fprintf(fp, "<colorMode>normal</colorMode>\n");
			fprintf(fp, "<outline>1</outline>\n");
			fprintf(fp, "</PolyStyle>\n");
			fprintf(fp, "</Style>\n");
			//fprintf(fp, "<MultiGeometry>\n");
			fprintf(fp, "<Polygon>\n");
			fprintf(fp, "<extrude>1</extrude>\n");
			fprintf(fp, "<altitudeMode>relativeToGround</altitudeMode>\n");
			//fprintf(stderr, "shape = %d to %d (#%d)\n", shape_start_id, shape_end_id, shape_row_id);
			
			int part_start_id = shape_start_id, part_end_id;
			while ((part_end_id = get_next_part_start(block, part_start_id)))
			{
				int shape_part_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
				
				if (part_start_id == shape_start_id)
					fprintf(fp, "<outerBoundaryIs>\n");
				else
					fprintf(fp, "<innerBoundaryIs>\n");
				
				fprintf(fp, "<LinearRing>\n");
				fprintf(fp, "<coordinates>\n");
				
				int i;
				for (i = part_start_id ; i < part_end_id ; i++)
				{
					fprintf(fp, "%.6f,%.6f,50\n", get_x(block, i), get_y(block, i));
				}
				
				fprintf(fp, "</coordinates>\n");
				fprintf(fp, "</LinearRing>\n");
				if (part_start_id == shape_start_id)
					fprintf(fp, "</outerBoundaryIs>\n");
				else
					fprintf(fp, "</innerBoundaryIs>\n");
				
				//fprintf(stderr, "	part = %d to %d (#%d)\n", part_start_id, part_end_id, shape_part_id);
				if (part_end_id == shape_end_id) break;
				part_start_id = part_end_id;
				//break;
			}
			
			fprintf(fp, "</Polygon>\n");
			//fprintf(fp, "</MultiGeometry>\n");
			fprintf(fp, "</Placemark>\n");
			if (shape_end_id == block->num_rows) break;
			shape_start_id = shape_end_id;
		}
	}
	fprintf(fp, "</Document>\n");
	fprintf(fp, "</kml>\n");
	
}
