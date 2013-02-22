
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../ext/gpc.h"
#include "block.h"

int main(int argc, char ** argv)
{
	gpc_op gpc_operation = GPC_INT; // default is intersection
	char clipping_filename[1000] = "";
	
	int c;
	while (1) {
		static struct option long_options[] = {
			{"filename", required_argument, 0, 'f'},
			{"operation", required_argument, 0, 'o'},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "f:o:", long_options, &option_index);
		if (c == -1) break;
		
		switch (c) {
			case 0: break;
			case 'f': strncpy(clipping_filename, optarg, sizeof(clipping_filename)); break;
			case 'o':
				if (strcmp(optarg, "difference")==0)				gpc_operation = GPC_DIFF;
				else if (strcmp(optarg, "intersection")==0) gpc_operation = GPC_INT;
				else if (strcmp(optarg, "exclusive-or")==0) gpc_operation = GPC_XOR;
				else if (strcmp(optarg, "union")==0)				gpc_operation = GPC_UNION;
				else {
					fprintf(stderr, "-o [difference|intersection|exclusive-or|union] ('%s' is invalid)\n", optarg);
					exit(1);
				}
				break;
			default: abort();
		}
	}
	
	if (clipping_filename == NULL) { fprintf(stderr, "--filename is required\n"); exit(1); }
	
	FILE * clip_fp = fopen(clipping_filename, "r");
	if (clip_fp == NULL) { fprintf(stderr, "--filename is required ('%s' doesn't exist)\n", clipping_filename); exit(1); }
	
	gpc_polygon * clip_polygon = malloc(sizeof(gpc_polygon));
	clip_polygon->num_contours = 0;
	clip_polygon->hole = NULL;
	clip_polygon->contour = NULL;
	
	struct Block * block = NULL;
	while ((block = read_block(clip_fp)))
	{
		/*if (block->gl_type != GL_LINE_LOOP) {
			fprintf(stderr, "Error: Trying to clip with a block which is not a line loop\n");
			exit(1);
		}*/
		
		const char * shape_type = get_attribute_value_as_string(block, "shape_type");
		if (shape_type == NULL || strcmp(shape_type, "line_loop") != 0) {
			fprintf(stderr, "shape_type == NULL || shape_type != 'line_loop'\n");
			exit(1);
		}
		
		gpc_vertex_list * contour = malloc(sizeof(gpc_vertex_list));
		contour->num_vertices = block->num_rows;
		contour->vertex = malloc(sizeof(gpc_vertex)*contour->num_vertices);
		
		int i;
		for (i = 0 ; i < contour->num_vertices ; i++)
		{
			//float * v = get_vertex(block, 0, i);
			contour->vertex[i].x = get_x(block, i);//v[0];
			contour->vertex[i].y = get_y(block, i);//v[1];
		}
		
		gpc_add_contour(clip_polygon, contour, 0);
		/* manipulate data here if you like */
		//write_block(stdout, block);
		free_block(block);
	}
	fclose(clip_fp);
	
	while ((block = read_block(stdin)))
	{
		gpc_polygon * org = malloc(sizeof(gpc_polygon));
		org->num_contours = 0;
		org->hole = NULL;
		org->contour = NULL;
		
		gpc_vertex_list * contour = malloc(sizeof(gpc_vertex_list));
		contour->num_vertices = block->num_rows;
		contour->vertex = malloc(sizeof(gpc_vertex)*contour->num_vertices);
		
		int i;
		for (i = 0 ; i < contour->num_vertices ; i++)
		{
			//float * v = get_vertex(block, 0, i);
			contour->vertex[i].x = get_x(block, i);//v[0];
			contour->vertex[i].y = get_y(block, i);//v[1];
		}
		gpc_add_contour(org, contour, 0);
		
		gpc_polygon * result_polygon = malloc(sizeof(gpc_polygon));
		result_polygon->num_contours = 0;
		result_polygon->contour = NULL;
		result_polygon->hole = NULL;
		
		gpc_polygon_clip(gpc_operation, org, clip_polygon, result_polygon);
		
		int j;
		for (j = 0 ; j < result_polygon->num_contours ; j++)
		{
			struct Block * res = new_block();
			res = copy_all_attributes(res, block);
			//res = add_string_attribute(res, "shape_type", "line_loop");
			//res->gl_type = GL_LINE_LOOP;
			//for (i = 0 ; i < block->num_attributes ; i++) {
			//	set_attribute(res, block->attributes[i].name, block->attributes[i].value);
			//}
			
			for (i = 0 ; i < result_polygon->contour[j].num_vertices ; i++)
			{
				res = add_row_and_blank(res);
				set_xy(res, res->num_rows-1, result_polygon->contour[j].vertex[i].x, result_polygon->contour[j].vertex[i].y);
				//float v[3] = { result_polygon->contour[j].vertex[i].x, result_polygon->contour[j].vertex[i].y, 0.0 };
				//append_vertex(res, v);
			}
			write_block(stdout, res);
			free_block(res);
		}
		
		gpc_free_polygon(org);
		gpc_free_polygon(result_polygon);
		free_block(block);
	}
	
	gpc_free_polygon(clip_polygon);
}
