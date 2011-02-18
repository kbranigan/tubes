
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

#include "scheme.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

//GLuint textureId;
//GLuint rboId;
//GLuint fboId;
int TEXTURE_WIDTH = 2000;
int TEXTURE_HEIGHT = 0;

//float min_x =  1000000;
//float max_x = -1000000;
//float min_y =  1000000;
//float max_y = -1000000;

/*struct matrix33
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
}*/

/*void get_sphere_coords_from_latlng(float lat, float lng, float *x, float *y, float *z)
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
}*/

struct Shape * out_shape = NULL;
uint32_t unique_set_id = 0;
void beginCallback(GLenum which)
{
  out_shape = (struct Shape*)malloc(sizeof(struct Shape));
  memset(out_shape, 0, sizeof(struct Shape));
  
  out_shape->unique_set_id = unique_set_id;
  out_shape->num_attributes = 0;
  out_shape->gl_type = which;
  out_shape->num_vertexs = 0;
  out_shape->num_vertex_arrays = 1;
  out_shape->vertex_arrays = (struct VertexArray*)malloc(sizeof(struct VertexArray)*out_shape->num_vertex_arrays);
  
  struct VertexArray * va = &out_shape->vertex_arrays[0];
  memset(va, 0, sizeof(struct VertexArray));
  va->num_dimensions = 3;
  va->array_type = GL_VERTEX_ARRAY;
}

void vertex3dv(const GLdouble * c)
{
  out_shape->num_vertexs++;
  struct VertexArray * va = &out_shape->vertex_arrays[0];
  va->vertexs = (double*)realloc(va->vertexs, sizeof(double)*out_shape->num_vertexs*va->num_dimensions);
  va->vertexs[(out_shape->num_vertexs-1)*3] = c[0];
  va->vertexs[(out_shape->num_vertexs-1)*3+1] = c[1];
  va->vertexs[(out_shape->num_vertexs-1)*3+2] = c[2];
}

void endCallback(void)
{
  if (out_shape->gl_type == GL_TRIANGLE_STRIP)
  {
    int new_num_vertexs = ((out_shape->num_vertexs - 3) * 3) + 3;
    double * new_vertexs = (double*)malloc(sizeof(double)*3*(new_num_vertexs));
    
    struct VertexArray * va = &out_shape->vertex_arrays[0];
    long i, j=0;
    for (i = 0 ; i < out_shape->num_vertexs ; i++)
    {
      if (j >= 3)
      {
        new_vertexs[j*3+0] = va->vertexs[(i-2)*3+0]; new_vertexs[j*3+1] = va->vertexs[(i-2)*3+1]; new_vertexs[j*3+2] = va->vertexs[(i-2)*3+2]; j++;
        new_vertexs[j*3+0] = va->vertexs[(i-1)*3+0]; new_vertexs[j*3+1] = va->vertexs[(i-1)*3+1]; new_vertexs[j*3+2] = va->vertexs[(i-1)*3+2]; j++;
      }
      new_vertexs[j*3+0] = va->vertexs[i*3+0]; new_vertexs[j*3+1] = va->vertexs[i*3+1]; new_vertexs[j*3+2] = va->vertexs[i*3+2]; j++;
    }
    
    free(va->vertexs);
    out_shape->num_vertexs = new_num_vertexs;
    va->vertexs = new_vertexs;
    out_shape->gl_type = GL_TRIANGLES;
  }
  else if (out_shape->gl_type == GL_TRIANGLE_FAN)
  {
    int new_num_vertexs = ((out_shape->num_vertexs - 3) * 3) + 3;
    double * new_vertexs = (double*)malloc(sizeof(double)*3*(new_num_vertexs));
    
    struct VertexArray * va = &out_shape->vertex_arrays[0];
    long i, j=0;
    for (i = 0 ; i < out_shape->num_vertexs ; i++)
    {
      if (j >= 3)
      {
        new_vertexs[j*3+0] = va->vertexs[0]; new_vertexs[j*3+1] = va->vertexs[1]; new_vertexs[j*3+2] = va->vertexs[2]; j++;
        new_vertexs[j*3+0] = va->vertexs[(i-1)*3+0]; new_vertexs[j*3+1] = va->vertexs[(i-1)*3+1]; new_vertexs[j*3+2] = va->vertexs[(i-1)*3+2]; j++;
      }
      new_vertexs[j*3+0] = va->vertexs[i*3+0]; new_vertexs[j*3+1] = va->vertexs[i*3+1]; new_vertexs[j*3+2] = va->vertexs[i*3+2]; j++;
    }
    
    free(va->vertexs);
    out_shape->num_vertexs = new_num_vertexs;
    va->vertexs = new_vertexs;
    out_shape->gl_type = GL_TRIANGLES;
  }
  
  write_shape(stdout, out_shape);
  free_shape(out_shape);
  out_shape = NULL;
}

void errorCallback(GLenum errorCode)
{
  const GLubyte *estring;

  estring = gluErrorString(errorCode);
  fprintf(stderr, "Tessellation Error: %s\n", estring);
  exit(1);
}

void combineCallback(GLdouble coords[3], 
                     GLdouble *vertex_data[4],
                     GLfloat weight[4], GLdouble **dataOut)
{
  GLdouble *vertex;
  int i;
  //fprintf(stderr, "combineCallback\n");
  vertex = (GLdouble *) malloc(3 * sizeof(GLdouble));
  vertex[0] = coords[0];
  vertex[1] = coords[1];
  vertex[2] = coords[2];
  //for (i = 3; i < 7; i++)
  //  vertex[i] = weight[0] * vertex_data[0][i] 
  //                + weight[1] * vertex_data[1][i]
  //                + weight[2] * vertex_data[2][i] 
  //                + weight[3] * vertex_data[3][i];
  *dataOut = vertex;
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
  
  GLUtesselator * tobj = NULL;
  tobj = gluNewTess();
  gluTessCallback(tobj, GLU_TESS_VERTEX, (GLvoid (*) ()) &vertex3dv);
  gluTessCallback(tobj, GLU_TESS_BEGIN, (GLvoid (*) ()) &beginCallback);
  gluTessCallback(tobj, GLU_TESS_END, (GLvoid (*) ()) &endCallback);
  gluTessCallback(tobj, GLU_TESS_ERROR, (GLvoid (*) ()) &errorCallback);
  gluTessCallback(tobj, GLU_TESS_COMBINE, (GLvoid (*) ()) &combineCallback);
  
  struct Shape * shape = NULL;
  long i=0, j=0, count=0;
  while ((shape = read_shape(stdin)))
  {
    unique_set_id = shape->unique_set_id;
    if (shape->gl_type != GL_LINE_LOOP) { fprintf(stderr, "providing non line loop to tesselator. NO GOOD\n"); exit(1); }
    gluTessBeginPolygon(tobj, NULL);
    gluTessBeginContour(tobj);
    for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    {
      struct VertexArray * va = &shape->vertex_arrays[i];
      if (va->num_dimensions != 3) continue;
      for (j = 0 ; j < shape->num_vertexs ; j++)
      {
        double *vertex;
        vertex = (double *) malloc(3 * sizeof(double));
        vertex[0] = va->vertexs[j*3];
        vertex[1] = va->vertexs[j*3+1];
        vertex[2] = va->vertexs[j*3+2];
        
        gluTessVertex(tobj, vertex, vertex);
      }
    }
    gluTessEndContour(tobj);
    gluTessEndPolygon(tobj);
    count++;
  }
  gluDeleteTess(tobj);
  
  if (count == 0)
  {
    fprintf(stderr, "There were no line loops to tesselate.\n");
  }
}
