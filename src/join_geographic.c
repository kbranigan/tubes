
#include "block.h"
#include "vectmath.h"

/*

This script specifically:

  - reads polygons from --filename  (such as neighbourhoods)
  - reads points from stdin         (such as addresses)
  - writes points to stdout with additional fields
  - additional fields include 'number_of_polygon_hits' and all the field values from the first point in the first polygon hit

*/

int point_in_triangle(vec2d A, vec2d B, vec2d C, vec2d P)
{
  vec2d v0; SUBV(v0, C, A);
  vec2d v1; SUBV(v1, B, A);
  vec2d v2; SUBV(v2, P, A);
  
  double dot00; DOTVP(dot00, v0, v0);
  double dot01; DOTVP(dot01, v0, v1);
  double dot02; DOTVP(dot02, v0, v2);
  double dot11; DOTVP(dot11, v1, v1);
  double dot12; DOTVP(dot12, v1, v2);
  
  double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
  double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  double v = (dot00 * dot12 - dot01 * dot02) * invDenom;
  
  return (u > 0.0) && (v > 0.0) && (u + v < 1.0);
}

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	static char filename[1000] = "";
	static int debug = 0;
	
	int c;
	while (1)
	{
		static struct option long_options[] = {
			{"filename", required_argument, 0, 'f'},
			{"debug", no_argument, &debug, 1},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "d:f:", long_options, &option_index);
		if (c == -1) break;
		
		switch (c)
		{
			case 0: break;
			case 'f': strncpy(filename, optarg, sizeof(filename)); break;
			default: abort();
		}
	}
	
	if (filename[0] == 0)
	{
		fprintf(stderr, "Usage: cat points.b | ./join_geographic -f polygons.b | ...");
		return EXIT_FAILURE;
	}
	
	FILE * fp = fopen(filename, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "filename '%s' is invalid.\n", filename);
		return EXIT_FAILURE;
	}
	
	struct Block * polygons = read_block(fp);
	fclose(fp);
	
	if (get_column_id_by_name(polygons, "x") == -1 ||
			get_column_id_by_name(polygons, "y") == -1 ||
			get_column_id_by_name(polygons, "shape_row_id") == -1 ||
			get_column_id_by_name(polygons, "shape_part_id") == -1) {
		fprintf(stderr, "No good, '%s' doesn't have the required fields\n", filename);
	}
	
	int polygon_shape_row_id_column_id = get_column_id_by_name(polygons, "shape_row_id");
	
	const char * shape_type = get_attribute_value_as_string(polygons, "shape_type");
	if (shape_type == NULL || strcmp(shape_type, "triangles") != 0) { fprintf(stderr, "Notice: %s expects '%s' to be 'triangles' (WILL END IN FAILURE)\n", argv[0], filename); }
	
	struct Block * block = NULL;
	while ((block = read_block(stdin)))
	{
		block = add_command(block, argc, argv);
		
		const char * shape_type = get_attribute_value_as_string(block, "shape_type");
		if (shape_type == NULL || strcmp(shape_type, "points") != 0) { fprintf(stderr, "Notice: %s expects piped blocks to be 'points', assuming points\n", argv[0]); }
		
		block = add_int32_column_and_blank(block, "number_of_polygon_hits");
		int number_of_polygon_hits_column_id = block->num_columns - 1;
		
		block = add_int32_column_and_blank(block, "first_hit_shape_row_id");
		int first_hit_shape_row_id_column_id = block->num_columns - 1;
		
		int i;
		for (i = 0 ; i < block->num_rows ; i++)
		{
			double x = get_x(block, i);
			double y = get_y(block, i);
			
			// foreach shape
			int shape_start_id = 0, shape_end_id;
			while ((shape_end_id = get_next_shape_start(polygons, shape_start_id)))
			{
				int polygon_shape_row_id = get_cell_as_int32(polygons, shape_start_id, polygon_shape_row_id_column_id);
				
				// foreach part of shape
				int part_start_id = shape_start_id, part_end_id;
				while ((part_end_id = get_next_part_start(polygons, part_start_id)))
				{
					//int shape_part_id = get_cell_as_int32(polygons, shape_start_id, shape_row_id_column_id);
					
					// this assumes the polygons are triangles.  if this isn't the case, this just won't work AT ALL
					int j;
					for (j = part_start_id ; j < part_end_id ; j += 3)
					{
						vec2d a = { get_x(polygons, j), get_y(polygons, j) };
						vec2d b = { get_x(polygons, j+1), get_y(polygons, j+1) };
						vec2d c = { get_x(polygons, j+2), get_y(polygons, j+2) };
						
						vec2d p = { x, y };
						
						if (point_in_triangle(a, b, c, p))
						{
							int32_t * number_of_polygon_hits = get_cell(block, i, number_of_polygon_hits_column_id);
							(*number_of_polygon_hits)++;
							int32_t * first_hit_shape_row_id = get_cell(block, i, first_hit_shape_row_id_column_id);
							(*first_hit_shape_row_id) = polygon_shape_row_id;
						}
					}
					
					//fprintf(stderr, " part %d to %d of shape %d to %d\n", part_start_id, part_end_id, shape_start_id, shape_end_id);
					if (part_end_id == shape_end_id) break; // last part of shape
					part_start_id = part_end_id;
				}
				
				if (shape_end_id == polygons->num_rows) break; // last shape
				shape_start_id = shape_end_id;
			}
		}
		
		write_block(stdout, block);
		free_block(block);
	}
}

















