
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheme.h"


struct matrix33
{
  float _11; float _12; float _13;
  float _21; float _22; float _23;
  float _31; float _32; float _33;
};

void identity33(struct matrix33 *o)
{
  o->_11 = 1; o->_12 = 0; o->_13 = 0;
  o->_21 = 0; o->_22 = 1; o->_23 = 0;
  o->_31 = 0; o->_32 = 0; o->_33 = 1;
}

void multiply_matrixes(struct matrix33 *m1, struct matrix33 *m2, struct matrix33 *o)
{
	o->_11 = m1->_11*m2->_11 + m1->_12*m2->_21 + m1->_13*m2->_31;
	o->_12 = m1->_11*m2->_12 + m1->_12*m2->_22 + m1->_13*m2->_32;
	o->_13 = m1->_11*m2->_13 + m1->_12*m2->_23 + m1->_13*m2->_33;
	o->_21 = m1->_21*m2->_11 + m1->_22*m2->_21 + m1->_23*m2->_31;
	o->_22 = m1->_21*m2->_12 + m1->_22*m2->_22 + m1->_23*m2->_32;
	o->_23 = m1->_21*m2->_13 + m1->_22*m2->_23 + m1->_23*m2->_33;
	o->_31 = m1->_31*m2->_11 + m1->_32*m2->_21 + m1->_33*m2->_31;
	o->_32 = m1->_31*m2->_12 + m1->_32*m2->_22 + m1->_33*m2->_32;
	o->_33 = m1->_31*m2->_13 + m1->_32*m2->_23 + m1->_33*m2->_33;
}

void get_sphere_coords_from_latlng(float lat, float lng, float *x, float *y, float *z)
{
  lat = (-lat+90) / 180.0 * 3.14159265;
  lng = (-lng+180) / 180.0 * 3.14159265;
  
  struct matrix33 x33; identity33(&x33); x33._22 = cosf(lng); x33._23 = -sinf(lng); x33._32 =   sinf(lng); x33._33 = cosf(lng);
  struct matrix33 y33; identity33(&y33); y33._11 = cosf(lat); y33._13 =  sinf(lat);  y33._31 = -sinf(lat); y33._33 = cosf(lat);
  struct matrix33 z33; identity33(&z33); z33._11 = cosf(lat); z33._12 = -sinf(lat); z33._21 =   sinf(lat); z33._22 = cosf(lat);
  struct matrix33 o33; multiply_matrixes(&y33, &x33, &o33);
  
  *x = o33._12;
  *y = o33._11; // switched, i think because of opengl
  *z = o33._13;
}

int main(int argc, char ** argv)
{
  if (!stdin_is_piped())
  {
    fprintf(stderr, "%s needs a data source. (redirected pipe, using |)\n", argv[0]);
    exit(1);
  }
  
  if (!stdout_is_piped())
  {
    fprintf(stderr, "%s outputs binary content. Pipe it to something that can read it.\n", argv[0]);
    exit(1);
  }
  
  if (!read_header(stdin, CURRENT_VERSION)) { fprintf(stderr, "read header failed.\n"); exit(1); }
  if (!write_header(stdout, CURRENT_VERSION)) { fprintf(stderr, "write header failed.\n"); exit(1); }
  
  struct Shape * shape = NULL;
  long i=0, j=0, count=0;
  while ((shape = read_shape(stdin)))
  {
    for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    {
      struct VertexArray * va = &shape->vertex_arrays[i];
      if (va->array_type != GL_VERTEX_ARRAY) continue;
      if (va->num_dimensions != 2) { fprintf(stderr, "./%s was expecting the vertex array to have 2 dimensions (lat/lng) but there are %d\n", argv[0], va->num_dimensions); return; }
      
      int num_new_dimensions = 3;
      float x, y, z;
      double *new_vertex_data = (double*)malloc(sizeof(double)*num_new_dimensions*shape->num_vertexs);
      
      for (j = 0 ; j < shape->num_vertexs ; j++)
      {
        get_sphere_coords_from_latlng(va->vertexs[j*va->num_dimensions+0], va->vertexs[j*va->num_dimensions+1], &x, &y, &z);
        new_vertex_data[j*num_new_dimensions+0] = x;
        new_vertex_data[j*num_new_dimensions+1] = y;
        new_vertex_data[j*num_new_dimensions+2] = z;
      }
      free(va->vertexs);
      va->vertexs = new_vertex_data;
      va->num_dimensions = num_new_dimensions;
    }
    
    write_shape(stdout, shape);
    free_shape(shape);
  }
}