
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "block.h"

int debug = 0;

struct Block * current_block = NULL;
struct Block * future_block = NULL;

struct Block * super_temp_block = NULL;
struct Block * temp_block = NULL;

void beginCallback(GLenum which) // this is called several times, switching from GL_TRIANGLE_STRIP to GL_TRIANGLE_FAN
{
	if (debug) fprintf(stderr, "beginCallback %d", which);

	if (debug && which == GL_TRIANGLE_FAN) fprintf(stderr, "  GL_TRIANGLE_FAN\n");
	if (debug && which == GL_TRIANGLE_STRIP) fprintf(stderr, "  GL_TRIANGLE_STRIP\n");
	if (debug && which == GL_TRIANGLES) fprintf(stderr, "  GL_TRIANGLES\n");
	
	if (temp_block != NULL) fprintf(stderr, "beginCallback called with temp_block != NULL\n");
	temp_block = new_block();
	temp_block = add_int32_attribute(temp_block, "gl_type", which);
	temp_block = copy_all_columns(temp_block, super_temp_block);
	//temp_block = add_xy_columns(temp_block);
}

void vertex3dv(const GLdouble * c)
{
	if (debug) fprintf(stderr, "  vertex3dv %f %f\n", c[0], c[1]);
	temp_block = add_row(temp_block);
	set_xy(temp_block, temp_block->num_rows-1, c[0], c[1]);
}

void errorCallback(GLenum errorCode)
{
	const GLubyte * estring;

	estring = gluErrorString(errorCode);
	fprintf(stderr, "Tessellation Error: %s\n", estring);
	exit(1);
}

void combineCallback(GLdouble coords[3], 
										 GLdouble *vertex_data[4],
										 GLfloat weight[4], GLdouble **dataOut)
{
	if (debug) fprintf(stderr, "  combineCallback\n");
	GLdouble *vertex;
	int i;
	vertex = (GLdouble *) malloc(3 * sizeof(GLdouble));
	vertex[0] = coords[0];
	vertex[1] = coords[1];
	vertex[2] = coords[2];
	//for (i = 3; i < 7; i++)
	//	vertex[i] = weight[0] * vertex_data[0][i] 
	//							+ weight[1] * vertex_data[1][i]
	//							+ weight[2] * vertex_data[2][i] 
	//							+ weight[3] * vertex_data[3][i];
	*dataOut = vertex;
}

void endCallback(void)
{
	if (temp_block == NULL) fprintf(stderr, "endCallback called with NULL temp_block\n");
	int32_t gl_type = get_attribute_value_as_int32(temp_block, "gl_type");
	
	int32_t x_column_id = get_column_id_by_name(temp_block, "x");
	int32_t y_column_id = get_column_id_by_name(temp_block, "y");
	
	struct Block * temp_block_as_triangles = new_block();
	temp_block_as_triangles = copy_all_columns(temp_block_as_triangles, temp_block);
	temp_block_as_triangles = add_int32_attribute(temp_block_as_triangles, "gl_type", GL_TRIANGLES);
	
	long i, j = 0;
	
	if (gl_type == GL_TRIANGLE_STRIP)
	{
		temp_block_as_triangles = set_num_rows(temp_block_as_triangles, ((temp_block->num_rows - 3) * 3) + 3);
		for (i = 0 ; i < temp_block->num_rows ; i++)
		{
			if (j >= 3)
			{
				set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, i-2, x_column_id), get_cell_as_double(temp_block, i-2, y_column_id));
				set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, i-1, x_column_id), get_cell_as_double(temp_block, i-1, y_column_id));
			}
			set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, i, x_column_id), get_cell_as_double(temp_block, i, y_column_id));
		}
	}
	else if (gl_type == GL_TRIANGLE_FAN)
	{
		temp_block_as_triangles = set_num_rows(temp_block_as_triangles, ((temp_block->num_rows - 3) * 3) + 3);
		for (i = 0 ; i < temp_block->num_rows ; i++)
		{
			if (j >= 3)
			{
				set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, 0, x_column_id), get_cell_as_double(temp_block, 0, y_column_id));
				set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, i-1, x_column_id), get_cell_as_double(temp_block, i-1, y_column_id));
			}
			set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, i, x_column_id), get_cell_as_double(temp_block, i, y_column_id));
		}
	} else if (gl_type == GL_TRIANGLES) {
		temp_block_as_triangles = set_num_rows(temp_block_as_triangles, 0);
		temp_block_as_triangles = append_block(temp_block_as_triangles, temp_block);
	} else {
		fprintf(stderr, "UNKNOWN gl_type from tesselator (%d)\n", gl_type);
	}
	
	//for (i = 0 ; i < temp_block_as_triangles->num_rows ; i++)
	//{
	//	fprintf(stderr, "%d: %f %f\n", i, get_x(temp_block_as_triangles, i), get_y(temp_block_as_triangles, i));
	//}
	
	if (debug) fprintf(stderr, "  append_block, %d+%d = ", super_temp_block->num_rows, temp_block_as_triangles->num_rows);
	super_temp_block = append_block(super_temp_block, temp_block_as_triangles);
	//fprintf(stderr, "%d\n", super_temp_block->num_rows);
	free_block(temp_block_as_triangles);
	free_block(temp_block);
	temp_block = NULL;
	if (debug) fprintf(stderr, "endCallback\n");
}

int main(int argc, char ** argv)
{
	GLUtesselator * tobj = NULL;
	tobj = gluNewTess();
	gluTessProperty(tobj, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
	
	gluTessCallback(tobj, GLU_TESS_VERTEX, (GLvoid (*) ()) &vertex3dv);
	gluTessCallback(tobj, GLU_TESS_BEGIN, (GLvoid (*) ()) &beginCallback);
	gluTessCallback(tobj, GLU_TESS_END, (GLvoid (*) ()) &endCallback);
	gluTessCallback(tobj, GLU_TESS_ERROR, (GLvoid (*) ()) &errorCallback);
	gluTessCallback(tobj, GLU_TESS_COMBINE, (GLvoid (*) ()) &combineCallback);
	
	struct Block * block = NULL;
	while ((block = read_block(stdin)))
	{
		future_block = new_block();
		future_block = copy_all_attributes(future_block, block);
		future_block = copy_all_columns(future_block, block);
		future_block = add_int32_attribute(future_block, "gl_type", (int32_t)GL_TRIANGLES);
		future_block = add_command(future_block, argc, argv);
		
		super_temp_block = new_block();
		super_temp_block = copy_all_columns(super_temp_block, future_block);
		
		int x_column_id = get_column_id_by_name(block, "x");
		int y_column_id = get_column_id_by_name(block, "y");
		int shape_row_id_column_id = get_column_id_by_name(block, "shape_row_id");
		int shape_part_id_column_id = get_column_id_by_name(block, "shape_part_id");
		int shape_part_type_column_id = get_column_id_by_name(block, "shape_part_type");
		
		int i, block_start = 0;
		int prev_shape_row_id = -1;
		int prev_shape_part_id = -1;
		
		if (debug) fprintf(stderr, "start\n");
		gluTessBeginPolygon(tobj, NULL);
		gluTessBeginContour(tobj);
		for (i = 0 ; i < block->num_rows ; i++)
		{
			int32_t shape_row_id  = get_cell_as_int32(block, i, shape_row_id_column_id);
			int32_t shape_part_id = get_cell_as_int32(block, i, shape_part_id_column_id);
			
			if (i == 0)
			{
				prev_shape_row_id = shape_row_id;
				prev_shape_part_id = shape_part_id;
			}
			
			if (prev_shape_row_id != shape_row_id || prev_shape_part_id != shape_part_id)
			{
				if (debug) fprintf(stderr, "stop (%dvs%d) (%dvs%d)\n", prev_shape_row_id, shape_row_id, prev_shape_part_id, shape_part_id);
				if (prev_shape_row_id != shape_row_id || prev_shape_part_id != shape_part_id) gluTessEndContour(tobj);
				if (prev_shape_row_id != shape_row_id) gluTessEndPolygon(tobj);
				
				int j = 0, k = 0;
				for (j = 0 ; j < super_temp_block->num_columns ; j++)
				{
					if (j == x_column_id || j == y_column_id) continue;
					for (k = 0 ; k < super_temp_block->num_rows ; k++)
					{
						if (j == shape_part_type_column_id)
							set_cell_from_int32(super_temp_block, k, j, 4); // 4 is for GL_TRIANGLES I guess (gonna change this to attributes)
						else
							set_cell(super_temp_block, k, j, get_cell(block, block_start, j));
					}
				}
				
				if (debug) fprintf(stderr, "append %d rows (src=(%d->%d))\n", super_temp_block->num_rows, block_start, i);
				future_block = append_block(future_block, super_temp_block);
				
				free_block(super_temp_block);
				super_temp_block = NULL;
				
				super_temp_block = new_block();
				super_temp_block = copy_all_columns(super_temp_block, future_block);
				
				block_start = i;
				
				if (prev_shape_row_id != shape_row_id) gluTessBeginPolygon(tobj, NULL);
				if (prev_shape_row_id != shape_row_id || prev_shape_part_id != shape_part_id) gluTessBeginContour(tobj);
				if (debug) fprintf(stderr, "start\n");
			}
			
			GLdouble * vertex = (GLdouble*)malloc(3*sizeof(GLdouble));
			vertex[0] = get_cell_as_double(block, i, x_column_id);
			vertex[1] = get_cell_as_double(block, i, y_column_id);
			vertex[2] = 0;
			gluTessVertex(tobj, vertex, vertex);
			
			prev_shape_row_id = shape_row_id;
			prev_shape_part_id = shape_part_id;
		}
		
		if (debug) fprintf(stderr, "stop (last)\n");
		gluTessEndContour(tobj);
		gluTessEndPolygon(tobj); // calls all the callbacks
		
		int j = 0, k = 0;
		for (j = 0 ; j < super_temp_block->num_columns ; j++)
		{
			if (j == x_column_id || j == y_column_id) continue;
			for (k = 0 ; k < super_temp_block->num_rows ; k++)
			{
				if (j == shape_part_type_column_id)
					set_cell_from_int32(super_temp_block, k, j, 4); // 4 is for GL_TRIANGLES I guess (gonna change this to attributes)
				else
					set_cell(super_temp_block, k, j, get_cell(block, block_start, j));
			}
		}
		
		//fprintf(stderr, "append %d rows (src=(%d->%d))\n", super_temp_block->num_rows, block_start, i-1);
		future_block = append_block(future_block, super_temp_block);
		
		free_block(super_temp_block);
		super_temp_block = NULL;
		
		write_block(stdout, future_block);
		free_block(block);
		
		free_block(future_block);
		future_block = NULL;
		
		/*
		gluTessBeginPolygon(tobj, NULL);
		gluTessBeginContour(tobj);
		
		int i, j;
		for (j = 0 ; j < block->num_rows ; j++)
		{
			GLdouble * vertex = (GLdouble*)malloc(3*sizeof(GLdouble));
			vertex[0] = get_cell_as_double(block, j, x_column_id);
			vertex[1] = get_cell_as_double(block, j, y_column_id);
			vertex[2] = 0;
			//fprintf(stderr, "input: %f %f\n", vertex[0], vertex[1]);
			gluTessVertex(tobj, vertex, vertex);
		}

		gluTessEndContour(tobj);
		gluTessEndPolygon(tobj);
		
		for (i = 0 ; i < future_block->num_columns ; i++)
		{
			if (i == x_column_id || i == y_column_id) continue;
			for (j = 0 ; j < future_block->num_rows ; j++)
			{
				if (i == shape_part_type_column_id)
					set_cell_from_int32(future_block, j, i, 4);
				else
					set_cell(future_block, j, i, get_cell(block, 0, i));
			}
		}
		*/
	}
}











