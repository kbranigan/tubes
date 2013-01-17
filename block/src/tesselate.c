
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
struct Block * temp_block = NULL;

void beginCallback(GLenum which) // this is called several times, switching from GL_TRIANGLE_STRIP to GL_TRIANGLE_FAN
{
	if (debug) fprintf(stderr, "beginCallback %d\n", which);
	if (temp_block != NULL) fprintf(stderr, "beginCallback called with temp_block != NULL\n");
	temp_block = new_block();
	temp_block = add_int32_attribute(temp_block, "gl_type", which);
	temp_block = add_xy_columns(temp_block);
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
	
	if (gl_type == GL_TRIANGLE_STRIP)
	{
		if (debug) fprintf(stderr, "  GL_TRIANGLE_STRIP\n");
		struct Block * temp_block_as_triangles = new_block();
		temp_block_as_triangles = add_int32_attribute(temp_block_as_triangles, "gl_type", GL_TRIANGLES);
		temp_block_as_triangles = add_xy_columns(temp_block_as_triangles);
		temp_block_as_triangles = set_num_rows(temp_block_as_triangles, ((temp_block->num_rows - 3) * 3) + 3);
		
		long i, j=0;
		for (i = 0 ; i < temp_block->num_rows ; i++)
		{
			if (j >= 3)
			{
				set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, i-2, x_column_id), get_cell_as_double(temp_block, i-2, y_column_id));
				set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, i-1, x_column_id), get_cell_as_double(temp_block, i-1, y_column_id));
				//set_xy(temp_block_as_triangles, j++, get_x(temp_block, i-2), get_y(temp_block, i-2));
				//set_xy(temp_block_as_triangles, j++, get_x(temp_block, i-1), get_y(temp_block, i-1));
			}
			set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, i, x_column_id), get_cell_as_double(temp_block, i, y_column_id));
			//set_xy(temp_block_as_triangles, j++, get_x(temp_block, i), get_y(temp_block, i));
		}
		
		free_block(temp_block);
		temp_block = temp_block_as_triangles;
		
	}
	else if (gl_type == GL_TRIANGLE_FAN)
	{
		if (debug) fprintf(stderr, "  GL_TRIANGLE_FAN\n");
		struct Block * temp_block_as_triangles = new_block();
		temp_block_as_triangles = add_int32_attribute(temp_block_as_triangles, "gl_type", GL_TRIANGLES);
		temp_block_as_triangles = add_xy_columns(temp_block_as_triangles);
		temp_block_as_triangles = set_num_rows(temp_block_as_triangles, ((temp_block->num_rows - 3) * 3) + 3);
		
		long i, j=0;
		for (i = 0 ; i < temp_block->num_rows ; i++)
		{
			if (j >= 3)
			{
				set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, 0, x_column_id), get_cell_as_double(temp_block, 0, y_column_id));
				set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, i-1, x_column_id), get_cell_as_double(temp_block, i-1, y_column_id));
				//set_xy(temp_block_as_triangles, j++, get_x(temp_block, 0), get_y(temp_block, 0));
				//set_xy(temp_block_as_triangles, j++, get_x(temp_block, i-1), get_y(temp_block, i-1));
			}
			set_xy(temp_block_as_triangles, j++, get_cell_as_double(temp_block, i, x_column_id), get_cell_as_double(temp_block, i, y_column_id));
			//set_xy(temp_block_as_triangles, j++, get_x(temp_block, i), get_y(temp_block, i));
		}
		
		free_block(temp_block);
		temp_block = temp_block_as_triangles;
	}
	else if (gl_type == GL_TRIANGLES)
	{
		if (debug) fprintf(stderr, "  GL_TRIANGLES\n");
	}
	else
	{
		fprintf(stderr, "UNKNOWN gl_type from tesselator\n");
	}
	
	//fprintf(stderr, "temp_block->num_rows = %d\n", temp_block->num_rows);
	
	int i;
	for (i = 0 ; i < temp_block->num_rows ; i++)
	{
		future_block = add_row(future_block);
		double x = get_cell_as_double(temp_block, i, x_column_id);
		double y = get_cell_as_double(temp_block, i, y_column_id);
		set_xy(future_block, future_block->num_rows-1, x, y);
	}
	
	free_block(temp_block);
	temp_block = NULL;
}

int main(int argc, char ** argv)
{
	GLUtesselator * tobj = NULL;
	tobj = gluNewTess();
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
		//future_block = add_xy_columns(future_block);
		
		gluTessBeginPolygon(tobj, NULL);
		gluTessBeginContour(tobj);
		
		int x_column_id = get_column_id_by_name(block, "x");
		int y_column_id = get_column_id_by_name(block, "y");
		int shape_part_type_column_id = get_column_id_by_name(block, "shape_part_type");
		
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
		
		write_block(stdout, future_block);
		free_block(block);
	}
}











