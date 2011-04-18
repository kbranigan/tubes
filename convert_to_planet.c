
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION convert_to_planet
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

int convert_to_planet(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  /*struct Shape * shape = NULL;
  memset(shape, 0, sizeof(struct Shape));
  shape->gl_which = GL_TRIANGLES;
  
  glBegin(GL_QUAD_STRIP);
  int a, b;
  for (a = -90 ; a <= 90 ; a += 5)
  {
    for (b = -180 ; b <= 180 ; b += 5)
    {
      glColor4f(0.7, 0.7, 0.7 + (180.0 - fabs(a)) / 180.0 * 0.3, 1);
      get_sphere_coords_from_latlng(a, b, &x, &y, &z);
      glVertex3f(x, y, z);
      glColor4f(0.7, 0.7, 0.7 + (180.0 - fabs(a+5)) / 180.0 * 0.3, 1);
      get_sphere_coords_from_latlng(a+5, b, &x, &y, &z);
      glVertex3f(x, y, z);
    }
  }
  glEnd();*/
  
  long i, j, k;
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    for (j = 0 ; j < shape->num_vertex_arrays ; j++)
    {
      struct VertexArray * va = &shape->vertex_arrays[j];
      if (va->array_type != GL_VERTEX_ARRAY) continue;
      if (va->num_dimensions < 2) fprintf(stderr, "vertex_array has %d dimensions (expected at least 2)\n", va->num_dimensions);
      if (va->vertexs == NULL) { fprintf(stderr, "vertex array %ld is NULL\n", j); exit(1); }
      
      float * new_vertexs = (float*)malloc(sizeof(float)*shape->num_vertexs*3);
      
      for (k = 0 ; k < shape->num_vertexs ; k++)
      {
        get_sphere_coords_from_latlng(va->vertexs[k*va->num_dimensions+1], va->vertexs[k*va->num_dimensions+0], &x, &y, &z);
        
        new_vertexs[k*3+0] = x;
        new_vertexs[k*3+1] = y;
        new_vertexs[k*3+2] = z;
      }
      
      free(va->vertexs);
      va->num_dimensions = 3;
      va->vertexs = new_vertexs;
    }
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
