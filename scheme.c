
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h> // for select()

#include "scheme.h"

void assert_stdin_is_piped()
{
  fd_set rfds;
  //struct timeval tv;
  int retval;
  
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  //tv.tv_sec = 5;
  //tv.tv_usec = 0;
  
  if (feof(stdin)) { fprintf(stderr, "feof() on stdin\n"); exit(1); }
  
  //retval = select(1, &rfds, NULL, NULL, &tv);
  retval = select(1, &rfds, NULL, NULL, NULL);
  
  if (retval == -1) { fprintf(stderr, "select() on stdin failed.\n"); exit(1); }
  
  if (!(retval != 0))
  {
    fprintf(stderr, "needs a data source. (redirected pipe, using |)\n");
    exit(1);
  }
}
  
void assert_stdout_is_piped()
{
  struct stat outstat = {0};
  struct stat errstat = {0};
  fstat(1, &outstat);
  fstat(2, &errstat);
  
  if (!(memcmp(&outstat, &errstat, sizeof outstat) != 0))
  {
    fprintf(stderr, "needs a data destination, the output is binary and will corrupt your console - try redirecting the output to a file (using [command] > [file])\n");
    exit(1);
  }
}

struct VertexArray * get_or_add_array(struct Shape * shape, int array_type)
{
  if (shape == NULL) shape = new_shape();
  if (shape->vertex_arrays == NULL) return NULL;
  int i = 0;
  for ( ; i < shape->num_vertex_arrays ; i++)
    if (shape->vertex_arrays[i].array_type == array_type)
      return &shape->vertex_arrays[i];
  
  shape->num_vertex_arrays++;
  shape->vertex_arrays = realloc(shape->vertex_arrays, shape->num_vertex_arrays*sizeof(struct VertexArray));
  if (shape->vertex_arrays == NULL) { fprintf(stderr, "malloc failed in read_shape()\n"); exit(1); }
  
  struct VertexArray * va = &shape->vertex_arrays[shape->num_vertex_arrays-1];
  va->shape = shape;
  va->array_type = array_type;
  va->num_dimensions = 2;
  va->vertexs = malloc(sizeof(float)*shape->num_vertexs*va->num_dimensions);
  if (va->vertexs == NULL) { fprintf(stderr, "malloc failed in get_array()\n"); exit(1); }
  
  return va;
}

void _append_vertex_fv(struct VertexArray * va, float * v)
{
  va->vertexs = realloc(va->vertexs, sizeof(float)*(va->shape->num_vertexs+1)*va->num_dimensions);
  if (va->vertexs == NULL) { fprintf(stderr, "realloc failed in append_vertexv()\n"); exit(1); }
  
  int i = 0;
  for (i = 0 ; i < va->num_dimensions ; i++)
    va->vertexs[va->num_dimensions*va->shape->num_vertexs+i] = v[i];
}

void append_vertex(struct Shape * shape, float * v)
{
  if (shape == NULL) { fprintf(stderr, "append_vertex called on a NULL shape\n"); exit(1); }
  if (shape->num_vertex_arrays != 1) { fprintf(stderr, "append_vertex called on a shape with %d arrays (try append_vertex2)\n", shape->num_vertex_arrays); exit(1); }
  
  _append_vertex_fv(&shape->vertex_arrays[0], v);
  shape->num_vertexs++;
}

void append_vertex2(struct Shape * shape, float * v1, float * v2)
{
  if (shape == NULL) { fprintf(stderr, "append_vertex3fv called on a NULL shape\n"); exit(1); }
  if (shape->num_vertex_arrays != 2) { fprintf(stderr, "append_vertex3fv called on a shape with %d arrays (try something like append_vertex3fv_4fv)\n", shape->num_vertex_arrays); exit(1); }
  
  _append_vertex_fv(&shape->vertex_arrays[0], v1);
  _append_vertex_fv(&shape->vertex_arrays[1], v2);
  shape->num_vertexs++;
}

void _set_vertex(struct VertexArray * va, int index, float * v)
{
  int i = 0;
  for (i = 0 ; i < va->num_dimensions ; i++)
    va->vertexs[va->num_dimensions*index+i] = v[i];
}

void set_vertex(struct Shape * shape, int index, float * v)
{
  if (shape == NULL) { fprintf(stderr, "set_vertex called on a NULL shape\n"); exit(1); }
  if (shape->num_vertex_arrays != 1) { fprintf(stderr, "set_vertex called on a shape with %d arrays (try set_vertex2)\n", shape->num_vertex_arrays); exit(1); }
  if (index < 0 || index >= shape->num_vertexs) { fprintf(stderr, "set_vertex called with an index of %d (there are %d vertexs)\n", index, shape->num_vertexs); exit(1); }
  
  _set_vertex(&shape->vertex_arrays[0], index, v);
}

void set_vertex2(struct Shape * shape, int index, float * v1, float * v2)
{
  if (shape == NULL) { fprintf(stderr, "set_vertex2 called on a NULL shape\n"); exit(1); }
  if (shape->num_vertex_arrays != 2) { fprintf(stderr, "set_vertex2 called on a shape with %d arrays (try set_vertex)\n", shape->num_vertex_arrays); exit(1); }
  if (index < 0 || index >= shape->num_vertexs) { fprintf(stderr, "set_vertex2 called with an index of %d (there are %d vertexs)\n", index, shape->num_vertexs); exit(1); }
  
  _set_vertex(&shape->vertex_arrays[0], index, v1);
  _set_vertex(&shape->vertex_arrays[1], index, v2);
}

float * get_vertex(struct Shape * shape, int index, int va_index)
{
  if (shape == NULL) { fprintf(stderr, "get_vertex called on a NULL shape\n"); exit(1); }
  if (index < 0 || index >= shape->num_vertexs) { fprintf(stderr, "get_vertex called with an index of %d (there are %d vertexs)\n", index, shape->num_vertexs); exit(1); }
  
  struct VertexArray * va = &shape->vertex_arrays[va_index];
  return &va->vertexs[va->num_dimensions*index];
}

struct Shape * new_shape()
{
  struct Shape * shape = (struct Shape*)malloc(sizeof(struct Shape));
  if (shape == NULL) { fprintf(stderr, "malloc failed in new_shape()\n"); exit(1); }
  memset(shape, 0, sizeof(struct Shape));
  shape->unique_set_id = 0;
  shape->version = CURRENT_VERSION;
  shape->num_attributes = 0;
  shape->attributes = NULL;
  shape->gl_type = GL_POINTS;
  shape->num_vertexs = 0;
  shape->num_vertex_arrays = 1;
  shape->vertex_arrays = (struct VertexArray*)malloc(sizeof(struct VertexArray));
  if (shape->vertex_arrays == NULL) { fprintf(stderr, "malloc failed in new_shape()\n"); exit(1); }
  
  struct VertexArray * va = &shape->vertex_arrays[0];
  memset(va, 0, sizeof(struct VertexArray));
  va->shape = shape;
  va->array_type = GL_VERTEX_ARRAY;
  va->num_dimensions = 2;
  va->vertexs = NULL;
  
  return shape;
}

void inspect_shape(FILE * fp, struct Shape * shape)
{
  long count_zero = 0;
  long i;
  
  char gl_types_c[8][20] = {"GL_POINTS", "GL_LINES", "GL_LINE_LOOP", "GL_LINE_STRIP", "GL_TRIANGLES", "GL_TRIANGLE_STRIP", "GL_TRIANGLE_FAN"};
  
  fprintf(stderr, "shape:\n");
  fprintf(stderr, "  unique_set_id: %d\n", shape->unique_set_id);
  fprintf(stderr, "  gl_type: %s\n", (shape->gl_type >= 0 && shape->gl_type < 8) ? gl_types_c[shape->gl_type] : "????");
  fprintf(stderr, "  num_attributes: %d\n", shape->num_attributes);
  if (shape->num_attributes) fprintf(stderr, "  attributes:\n");
  for (i = 0 ; i < shape->num_attributes ; i++)
  {
    struct Attribute * attribute = &shape->attributes[i];
    fprintf(stderr, "    %s(%d): '%s'\n", attribute->name, attribute->value_length, attribute->value);
  }
  fprintf(stderr, "  num_vertexs: %d\n", shape->num_vertexs);
  fprintf(stderr, "  num_vertex_arrays: %d\n", shape->num_vertex_arrays);
  if (shape->num_vertex_arrays) fprintf(stderr, "  vertex_arrays:\n");
  for (i = 0 ; i < shape->num_vertex_arrays ; i++)
  {
    fprintf(stderr, "    array_type: %d\n", shape->vertex_arrays[i].array_type);
    fprintf(stderr, "    num_dimensions: %d\n", shape->vertex_arrays[i].num_dimensions);
    fprintf(stderr, "    vertexs:\n");
    if (shape->num_vertexs > 0 && shape->vertex_arrays[i].num_dimensions > 0)
    {
      long j,k;
      for (k = 0 ; k < shape->num_vertexs ; k++)
      {
        if (k < 4) fprintf(stderr, "      ");
        int is_zero = 1;
        for (j = 0 ; j < shape->vertex_arrays[i].num_dimensions ; j++)
        {
          if (shape->vertex_arrays[i].vertexs[k*shape->vertex_arrays[i].num_dimensions + j] != 0.0) is_zero = 0;
          if (k < 4) fprintf(stderr, "%f ", shape->vertex_arrays[i].vertexs[k*shape->vertex_arrays[i].num_dimensions + j]);
        }
        if (is_zero) count_zero ++;
        if (k < 4) fprintf(stderr, "\n");
        else if (k == 4) fprintf(stderr, "      ...\n");
      }
    }
    if (i == 4) fprintf(stderr, "    [...]\n");
  }
  if (count_zero > 0) fprintf(stderr, "  count_zero: %ld\n", count_zero);
}

int write_shape(FILE * fp, struct Shape * shape)
{
  if (fp == NULL) { fprintf(stderr, "trying to write shape to NULL file pointer\n"); return 0; }
  if (shape == NULL) { fprintf(stderr, "trying to write NULL shape\n"); return 0; }
  if (shape == NULL || shape->version != CURRENT_VERSION) { fprintf(stderr, "trying to write shape with an invalid version (%d), CURRENT_VERSION = %d\n", shape->version, CURRENT_VERSION); return 0; }
  
  float inf = 1.0 / 0.0;
  if (fwrite(&inf, sizeof(inf), 1, fp) != 1) return 0;
  if (fwrite(&shape->version, sizeof(shape->version), 1, fp) != 1) return 0;
  if (fwrite(&shape->unique_set_id, sizeof(shape->unique_set_id), 1, fp) != 1) return 0;
  if (fwrite(&shape->num_attributes, sizeof(shape->num_attributes), 1, fp) != 1) return 0;
  
  long i;
  for (i = 0 ; i < shape->num_attributes ; i++)
  {
    struct Attribute * attribute = &shape->attributes[i];
    fwrite(attribute->name, sizeof(attribute->name), 1, fp);
    fwrite(&attribute->value_length, sizeof(attribute->value_length), 1, fp);
    fwrite(attribute->value, attribute->value_length, 1, fp);
  }
  
  if (fwrite(&shape->gl_type, sizeof(shape->gl_type), 1, fp) != 1) return 0;
  if (fwrite(&shape->num_vertexs, sizeof(shape->num_vertexs), 1, fp) != 1) return 0;
  if (fwrite(&shape->num_vertex_arrays, sizeof(shape->num_vertex_arrays), 1, fp) != 1) return 0;
  
  for (i = 0 ; i < shape->num_vertex_arrays ; i++)
  {
    struct VertexArray * va = &shape->vertex_arrays[i];
    if (fwrite(&va->array_type, sizeof(va->array_type), 1, fp) != 1) return 0;
    if (fwrite(&va->num_dimensions, sizeof(va->num_dimensions), 1, fp) != 1) return 0;
    if (fwrite(va->vertexs, sizeof(float)*va->num_dimensions*shape->num_vertexs, 1, fp) != 1) return 0;
  }
  return 0;
}

struct Shape * read_shape(FILE * fp)
{
  if (fp == NULL) { fprintf(stderr, "trying to read shape from NULL file pointer\n"); return 0; }
  
  float shape_header;
  if (fread(&shape_header, sizeof(shape_header), 1, fp) != 1) return NULL;
  if (shape_header != INFINITY) { fprintf(stderr, "shape header (%f) is not infinity\n", shape_header); return NULL; }
  
  uint32_t shape_version;
  if (fread(&shape_version, sizeof(shape_version), 1, fp) != 1) return NULL;
  if (shape_version != CURRENT_VERSION) { fprintf(stderr, "shape version (%d) is not CURRENT_VERSION (%d)\n", shape_version, CURRENT_VERSION); return NULL; }
  
  struct Shape * shape = (struct Shape *)malloc(sizeof(struct Shape));
  if (shape == NULL) { fprintf(stderr, "malloc failed in read_shape()\n"); exit(1); }
  memset(shape, 0, sizeof(struct Shape));
  shape->version = shape_version;
  
  if (fread(&shape->unique_set_id, sizeof(shape->unique_set_id), 1, fp) != 1) { fprintf(stderr, "fread unique_set_id error\n"); return NULL; }
  if (fread(&shape->num_attributes, sizeof(shape->num_attributes), 1, fp) != 1) { fprintf(stderr, "fread num_attributes error\n"); return NULL; }
  
  if (shape->num_attributes > 0)
  {
    shape->attributes = (struct Attribute *)malloc(sizeof(struct Attribute)*shape->num_attributes);
    if (shape->attributes == NULL) { fprintf(stderr, "malloc failed in read_shape()\n"); exit(1); }
    
    int i;
    for (i = 0 ; i < shape->num_attributes ; i++)
    {
      struct Attribute * attribute = &shape->attributes[i];
      
      if (fread(&attribute->name, sizeof(attribute->name), 1, fp) != 1) { fprintf(stderr, "fread attribute %d key error\n", i); return NULL; }
      if (fread(&attribute->value_length, sizeof(attribute->value_length), 1, fp) != 1) { fprintf(stderr, "fread attribute %d value length error\n", i); return NULL; }
      attribute->value = malloc(attribute->value_length+1);
      if (attribute->value == NULL) { fprintf(stderr, "malloc failed in read_shape()\n"); exit(1); }
      if (fread(attribute->value, attribute->value_length, 1, fp) != 1) { fprintf(stderr, "fread attribute %d value error\n", i); return NULL; }
      attribute->value[attribute->value_length] = 0;
    }
  }
  
  if (fread(&shape->gl_type, sizeof(shape->gl_type), 1, fp) != 1) { fprintf(stderr, "fread gl_type error\n"); return NULL; }
  if (fread(&shape->num_vertexs, sizeof(shape->num_vertexs), 1, fp) != 1) { fprintf(stderr, "fread num_vertexs error\n"); return NULL; }
  if (fread(&shape->num_vertex_arrays, sizeof(shape->num_vertex_arrays), 1, fp) != 1) { fprintf(stderr, "fread num_vertex_arrays error\n"); return NULL; }
  
  if (shape->num_vertex_arrays > 0)
  {
    shape->vertex_arrays = (struct VertexArray *)malloc(sizeof(struct VertexArray)*shape->num_vertex_arrays);
    if (shape->vertex_arrays == NULL) { fprintf(stderr, "malloc failed in read_shape()\n"); exit(1); }
    
    int i;
    for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    {
      struct VertexArray * va = &shape->vertex_arrays[i];
      va->shape = shape;
      if (fread(&va->array_type, sizeof(va->array_type), 1, fp) != 1) { fprintf(stderr, "fread vertex_array %d array_type error\n", i); return NULL; }
      if (fread(&va->num_dimensions, sizeof(va->num_dimensions), 1, fp) != 1) { fprintf(stderr, "fread vertex_array %d num_dimensions error\n", i); return NULL; }
      //if (va->num_dimensions != 3) fprintf(stderr, "va->num_dimensions = %d\n", va->num_dimensions);
      
      va->vertexs = (float*)malloc(sizeof(float)*shape->num_vertexs*va->num_dimensions);
      if (va->vertexs == NULL) { fprintf(stderr, "malloc failed in read_shape()\n"); exit(1); }
      
      if (fread(va->vertexs, sizeof(float)*va->num_dimensions*shape->num_vertexs, 1, fp) != 1) { fprintf(stderr, "fread vertex_array %d vertexs error\n", i); return NULL; }
    }
  }
  
  return shape;
}

int free_shape(struct Shape * shape)
{
  int i = 0;
  for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    free(shape->vertex_arrays[i].vertexs);
    
  for (i = 0 ; i < shape->num_attributes ; i++)
    free(shape->attributes[i].value);
    
  if (shape->num_vertex_arrays) free(shape->vertex_arrays);
  if (shape->num_attributes) free(shape->attributes);
  free(shape);
  return 1;
}

int point_in_triangle(vec2d A, vec2d B, vec2d C, vec2d P)
{
  vec2d v0; SUBV(v0, C, A);
  vec2d v1; SUBV(v1, B, A);
  vec2d v2; SUBV(v2, P, A);
  
  float dot00; DOTVP(dot00, v0, v0);
  float dot01; DOTVP(dot01, v0, v1);
  float dot02; DOTVP(dot02, v0, v2);
  float dot11; DOTVP(dot11, v1, v1);
  float dot12; DOTVP(dot12, v1, v2);
  
  float invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
  float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
  
  return (u > 0.0) && (v > 0.0) && (u + v < 1.0);
}
