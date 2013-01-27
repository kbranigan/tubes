
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h> // for select()

#include "scheme.h"
int ARGC = 0;
char ** ARGV = NULL;
char * COMMAND = NULL;

int num_strings = 0;
char ** strings = NULL;

int stdin_is_piped_t(float timeout)
{
  fd_set rfds;
  struct timeval tv;
  int retval;
  
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  
  tv.tv_sec = floor(timeout);
  tv.tv_usec = (float)floor(timeout / 1000000.0);
  
  if (feof(stdin)) { fprintf(stderr, "feof() on stdin\n"); exit(1); }
  
  if (timeout != 0)
    retval = select(1, &rfds, NULL, NULL, &tv);
  else
    retval = select(1, &rfds, NULL, NULL, NULL);
  
  if (retval == -1) { fprintf(stderr, "select() on stdin failed.\n"); exit(1); }
  
  return (retval != 0);
}
int stdin_is_piped() { return stdin_is_piped_t(0); }

void assert_stdin_is_piped_t(float timeout)
{
  if (!stdin_is_piped_t(timeout))
  {
    fprintf(stderr, "needs a data source. (redirected pipe, using |)\n");
    exit(1);
  }
}
void assert_stdin_is_piped() { assert_stdin_is_piped_t(0); }


int stdout_is_piped()
{
  struct stat outstat = {0};
  struct stat errstat = {0};
  fstat(1, &outstat); // stdout
  fstat(2, &errstat); // stderr
  return (memcmp(&outstat, &errstat, sizeof outstat) != 0); // basically test if stdout == stderr
}

void assert_stdout_is_piped()
{
  if (!stdout_is_piped())
  {
    fprintf(stderr, "needs a data destination, the output is binary and will corrupt your console - try redirecting the output to a file (using [command] > [file])\n");
    exit(1);
  }
}

void assert_stdin_or_out_is_piped()
{
  if (!stdin_is_piped_t(0.2) && !stdout_is_piped())
  {
    fprintf(stderr, "needs a piped source or destination, the output is binary and will corrupt your console - try redirecting the output to a file (using [command] > [file])\n");
    exit(1);
  }
}

struct VertexArray * get_or_add_array(struct Shape * shape, unsigned int array_type)
{
  if (shape == NULL) shape = new_shape();
  if (shape->vertex_arrays == NULL) return NULL;
  unsigned int i = 0;
  for ( ; i < shape->num_vertex_arrays ; i++)
    if (shape->vertex_arrays[i].array_type == array_type)
      return &shape->vertex_arrays[i];
  
  shape->num_vertex_arrays++;
  shape->vertex_arrays = (struct VertexArray*)realloc(shape->vertex_arrays, shape->num_vertex_arrays*sizeof(struct VertexArray));
  if (shape->vertex_arrays == NULL) { fprintf(stderr, "malloc failed in get_or_add_array()\n"); exit(1); }
  
  struct VertexArray * va = &shape->vertex_arrays[shape->num_vertex_arrays-1];
  va->shape = shape;
  va->array_type = array_type;
  if (array_type == GL_COLOR_ARRAY)
    va->num_dimensions = 3;
  else
    va->num_dimensions = 2;
  va->vertexs = (float*)malloc(sizeof(float)*shape->num_vertexs*va->num_dimensions);
  if (va->vertexs == NULL) { fprintf(stderr, "malloc failed in get_or_add_array()\n"); exit(1); }
  
  return va;
}

void set_num_vertexs(struct Shape * shape, unsigned int num_vertexs)
{
  if (shape == NULL) { fprintf(stderr, "set_num_vertexs called on a NULL shape\n"); exit(1); }
  shape->num_vertexs = num_vertexs;
  unsigned int i;
  for (i = 0 ; i < shape->num_vertex_arrays ; i++)
  {
    struct VertexArray * va = &shape->vertex_arrays[i];
    va->vertexs = (float*)realloc(va->vertexs, sizeof(float) * shape->num_vertexs * va->num_dimensions);
    if (va->vertexs == NULL) { fprintf(stderr, "realloc failed generating vertex array for shape (%d points, %d dimensions)\n", shape->num_vertexs, va->num_dimensions); exit(0); }
  }
}

void set_num_dimensions(struct Shape * shape, unsigned int va_index, unsigned int num_dimensions)
{
  if (shape == NULL) { fprintf(stderr, "set_num_dimensions called on a NULL shape\n"); exit(1); }
  if (va_index > shape->num_vertex_arrays) { fprintf(stderr, "set_num_dimensions called with invalid va_index (%d)\n", va_index); return; }
  if (num_dimensions > 100) { fprintf(stderr, "set_num_dimensions called with invalid num_dimensions (%d)\n", num_dimensions); return; }
  
  struct VertexArray * va = &shape->vertex_arrays[va_index];
  float * vertexs = (float*)malloc(sizeof(float)*shape->num_vertexs*num_dimensions);
  if (vertexs == NULL) { fprintf(stderr, "realloc failed in set_num_dimensions()\n"); exit(1); }
  
  unsigned int i, j;
  for (i = 0 ; i < shape->num_vertexs ; i++)
    for (j = 0 ; j < num_dimensions && j < va->num_dimensions ; j++)
      vertexs[num_dimensions*i+j] = va->vertexs[va->num_dimensions*i+j];
  
  free(va->vertexs);
  va->vertexs = vertexs;
  va->num_dimensions = num_dimensions;
}

void _append_vertex_fv(struct VertexArray * va, float * v)
{
  va->vertexs = (float*)realloc(va->vertexs, sizeof(float)*(va->shape->num_vertexs+1)*va->num_dimensions);
  if (va->vertexs == NULL) { fprintf(stderr, "realloc failed in append_vertexv()\n"); exit(1); }
  
  unsigned int i = 0;
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
  if (shape->num_vertex_arrays != 2) { fprintf(stderr, "append_vertex2 called on a shape with %d arrays\n", shape->num_vertex_arrays); exit(1); }
  
  _append_vertex_fv(&shape->vertex_arrays[0], v1);
  _append_vertex_fv(&shape->vertex_arrays[1], v2);
  shape->num_vertexs++;
}

void set_vertex(struct Shape * shape, unsigned int va_index, unsigned int index, float * v)
{
  if (shape == NULL) { fprintf(stderr, "set_vertex called on a NULL shape\n"); exit(1); }
  if (index >= shape->num_vertexs) { fprintf(stderr, "set_vertex called with an index of %d (there are %d vertexs)\n", index, shape->num_vertexs); exit(1); }
  if (va_index >= shape->num_vertex_arrays) { fprintf(stderr, "set_vertex called with an va_index of %d (there are %d vertex_arrays)\n", va_index, shape->num_vertex_arrays); exit(1); }
  
  struct VertexArray * va = &shape->vertex_arrays[va_index];
  
  unsigned int i = 0;
  for (i = 0 ; i < va->num_dimensions ; i++)
  {
    va->vertexs[va->num_dimensions*index+i] = v[i];
  }
}

float * get_vertex(struct Shape * shape, unsigned int va_index, unsigned int index)
{
  if (shape == NULL) { fprintf(stderr, "get_vertex called on a NULL shape\n"); exit(1); }
  if (index >= shape->num_vertexs) { fprintf(stderr, "get_vertex called with an index of %d (there are %d vertexs)\n", index, shape->num_vertexs); exit(1); }
  if (va_index >= shape->num_vertex_arrays) { fprintf(stderr, "get_vertex called with an va_index of %d (there are %d vertex_arrays)\n", va_index, shape->num_vertex_arrays); exit(1); }
  
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
  shape->has_attribute_names = 1;
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

int write_shape(FILE * fp, struct Shape * shape)
{
  if (fp == NULL) { fprintf(stderr, "trying to write shape to NULL file pointer\n"); return 0; }
  if (shape == NULL) { fprintf(stderr, "trying to write NULL shape\n"); return 0; }
  //if (shape == NULL || shape->version != CURRENT_VERSION) { fprintf(stderr, "trying to write shape with an invalid version (%d), CURRENT_VERSION = %d\n", shape->version, CURRENT_VERSION); return 0; }
  
  long i;
  
  float inf = INFINITY;
  if (fwrite(&inf, sizeof(inf), 1, fp) != 1) return 0;
  if (fwrite(&shape->version, sizeof(shape->version), 1, fp) != 1) return 0;
  if (fwrite(&shape->unique_set_id, sizeof(shape->unique_set_id), 1, fp) != 1) return 0;
  if (fwrite(&shape->num_attributes, sizeof(shape->num_attributes), 1, fp) != 1) return 0;
  if (shape->version >= FILE_VERSION_5)
    if (fwrite(&shape->has_attribute_names, sizeof(shape->has_attribute_names), 1, fp) != 1) return 0;
  for (i = 0 ; i < shape->num_attributes ; i++)
  {
    struct Attribute * attribute = &shape->attributes[i];
    if (shape->has_attribute_names)
      fwrite(attribute->name, sizeof(attribute->name), 1, fp);
    fwrite(&attribute->value_length, sizeof(attribute->value_length), 1, fp);
    if (attribute->value_length > 0)
      fwrite(attribute->value, attribute->value_length, 1, fp);
  }
  
  if (fwrite(&shape->gl_type, sizeof(shape->gl_type), 1, fp) != 1) return 0;
  if (fwrite(&shape->num_vertexs, sizeof(shape->num_vertexs), 1, fp) != 1) return 0;
  if (fwrite(&shape->num_vertex_arrays, sizeof(shape->num_vertex_arrays), 1, fp) != 1) return 0;
  
  if (shape->num_vertexs > 0 && shape->num_vertex_arrays > 0)
  for (i = 0 ; i < shape->num_vertex_arrays ; i++)
  {
    struct VertexArray * va = &shape->vertex_arrays[i];
    if (fwrite(&va->array_type, sizeof(va->array_type), 1, fp) != 1) return 0;
    if (fwrite(&va->num_dimensions, sizeof(va->num_dimensions), 1, fp) != 1) return 0;
    if (fwrite(va->vertexs, sizeof(float)*va->num_dimensions*shape->num_vertexs, 1, fp) != 1) return 0;
  }
  
  fflush(fp);
  return 0;
}

struct Shape ** read_all_shapes(FILE * fp, unsigned int * num_shapes_p)
{
  struct Shape ** shapes = NULL;
  int num_shapes = 0;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(fp)))
  {
    num_shapes++;
    shapes = (struct Shape **)realloc(shapes, sizeof(struct Shape*) * num_shapes);
    shapes[num_shapes-1] = shape;
  }
  
  (*num_shapes_p) = num_shapes;
  return shapes;
}

struct Shape * read_shape(FILE * fp)
{
  if (fp == NULL) { fprintf(stderr, "trying to read shape from NULL file pointer\n"); return 0; }
  
  float shape_header;
  do
  {
    if (fread(&shape_header, sizeof(shape_header), 1, fp) != 1) return NULL;
    if (isfinite(shape_header)) { fprintf(stderr, "header (%f) is finite (it's suppose to be infinite, either no data or invalid data was read)\n", shape_header); return NULL; }
    
    // is it a shape, or is it a thing?
    if (shape_header < 0)
    {
      uint32_t version;
      if (fread(&version, sizeof(version), 1, fp) != 1) return NULL;
      //if (version != CURRENT_VERSION) { fprintf(stderr, "version (%d) is not CURRENT_VERSION (%d)\n", version, CURRENT_VERSION); return NULL; }
      
      uint32_t thing_type;
      if (fread(&thing_type, sizeof(thing_type), 1, fp) != 1) return NULL;
      //if (thing_type != STRING_TABLE) { fprintf(stderr, "invalid thing found in the data (number: %d)\n", thing_type); return NULL; }
      
      uint32_t num_strings;
      if (fread(&num_strings, sizeof(num_strings), 1, fp) != 1) return NULL;
      if (num_strings > 1000) { fprintf(stderr, "invalid num_strings found in the data (%d)\n", num_strings); return NULL; }
      
      if (strings != NULL) free(strings);
      strings = (char**)malloc(20*num_strings);
      
      unsigned int i;
      for (i = 0 ; i < num_strings ; i++)
      {
        if (fread(&strings[i], 20, 1, fp) != 1) { fprintf(stderr, "fread string %d error\n", i); return NULL; }
      }
      
      ////
      
      if (fread(&shape_header, sizeof(shape_header), 1, fp) != 1) return NULL;
      if (isfinite(shape_header)) { fprintf(stderr, "header (%f) is finite (it's suppose to be infinite, this means your data sucks)\n", shape_header); return NULL; }
    }
  } while (shape_header < 0);
  
  uint32_t shape_version;
  if (fread(&shape_version, sizeof(shape_version), 1, fp) != 1) return NULL;
  //if (shape_version != CURRENT_VERSION) { fprintf(stderr, "version (%d) is not CURRENT_VERSION (%d)\n", shape_version, CURRENT_VERSION); return NULL; }
  
  struct Shape * shape = (struct Shape *)malloc(sizeof(struct Shape));
  if (shape == NULL) { fprintf(stderr, "malloc failed in read_shape()\n"); exit(1); }
  memset(shape, 0, sizeof(struct Shape));
  shape->version = shape_version;
  
  if (fread(&shape->unique_set_id, sizeof(shape->unique_set_id), 1, fp) != 1) { fprintf(stderr, "fread unique_set_id error\n"); return NULL; }
  if (fread(&shape->num_attributes, sizeof(shape->num_attributes), 1, fp) != 1) { fprintf(stderr, "fread num_attributes error\n"); return NULL; }
  if (shape->version >= FILE_VERSION_5)
  {
    if (fread(&shape->has_attribute_names, sizeof(shape->has_attribute_names), 1, fp) != 1) { fprintf(stderr, "fread has_attribute_names error\n"); return NULL; }
  }
  else
    shape->has_attribute_names = 1;
  
  if (shape->num_attributes > 0)
  {
    shape->attributes = (struct Attribute *)malloc(sizeof(struct Attribute)*shape->num_attributes);
    if (shape->attributes == NULL) { fprintf(stderr, "malloc failed in read_shape()\n"); exit(1); }
    
    long i;
    for (i = 0 ; i < shape->num_attributes ; i++)
    {
      struct Attribute * attribute = &shape->attributes[i];
      
      if (shape->has_attribute_names)
        if (fread(&attribute->name, sizeof(attribute->name), 1, fp) != 1) { fprintf(stderr, "fread attribute %ld key error\n", i); return NULL; }
      
      if (fread(&attribute->value_length, sizeof(attribute->value_length), 1, fp) != 1) { fprintf(stderr, "fread attribute %ld value length error\n", i); return NULL; }
      attribute->value = (char*)malloc(attribute->value_length+1);
      if (attribute->value == NULL) { fprintf(stderr, "malloc failed in read_shape()\n"); exit(1); }
      if (attribute->value_length > 0)
        if (fread(attribute->value, attribute->value_length, 1, fp) != 1) { fprintf(stderr, "fread attribute %ld value error\n", i); return NULL; }
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
    
    unsigned int i;
    for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    {
      struct VertexArray * va = &shape->vertex_arrays[i];
      memset(va, 0, sizeof(struct VertexArray));
      va->shape = shape;
      
      if (shape->num_vertexs > 0)
      {
        if (fread(&va->array_type, sizeof(va->array_type), 1, fp) != 1) { fprintf(stderr, "fread vertex_array %d array_type error\n", i); return NULL; }
        if (fread(&va->num_dimensions, sizeof(va->num_dimensions), 1, fp) != 1) { fprintf(stderr, "fread vertex_array %d num_dimensions error\n", i); return NULL; }
        //if (va->num_dimensions != 3) fprintf(stderr, "va->num_dimensions = %d\n", va->num_dimensions);
      
        va->vertexs = (float*)malloc(sizeof(float)*shape->num_vertexs*va->num_dimensions);
        if (va->vertexs == NULL) { fprintf(stderr, "malloc failed in read_shape()\n"); exit(1); }
      
        if (fread(va->vertexs, sizeof(float)*va->num_dimensions*shape->num_vertexs, 1, fp) != 1) { fprintf(stderr, "fread vertex_array %d vertexs error\n", i); return NULL; }
      }
      else
      {
        va->array_type = GL_VERTEX_ARRAY;
        va->num_dimensions = 2;
        va->vertexs = NULL;
      }
    }
  }
  
  return shape;
}

char * get_attribute(struct Shape * shape, char * name)
{
  unsigned int i;
  for (i = 0 ; i < shape->num_attributes ; i++)
    if (strcmp(shape->attributes[i].name, name)==0)
      return shape->attributes[i].value;
  return NULL;
}

void set_attribute(struct Shape * shape, char * name, char * value)
{
  if (shape == NULL || value == NULL) return; // { fprintf(stderr, "set_attribute called with shape or value being NULL"); exit(1); }
  
  unsigned int i;
  if (name != NULL)
  for (i = 0 ; i < shape->num_attributes ; i++)
  {
    if (strcmp(shape->attributes[i].name, name)==0)
    {
      free(shape->attributes[i].value);
      shape->attributes[i].value_length = (int)strlen(value);
      shape->attributes[i].value = (char*)malloc(shape->attributes[i].value_length+1);
      strncpy(shape->attributes[i].value, value, shape->attributes[i].value_length);
      shape->attributes[i].value[shape->attributes[i].value_length] = 0;
      return;
    }
  }
  
  shape->attributes = (struct Attribute*)realloc(shape->attributes, sizeof(struct Attribute)*(shape->num_attributes+1));
  if (name != NULL)
    strncpy(shape->attributes[shape->num_attributes].name, name, 19);
  shape->attributes[shape->num_attributes].value_length = (int)strlen(value);
  shape->attributes[shape->num_attributes].value = (char*)malloc(shape->attributes[shape->num_attributes].value_length+1);
  strncpy(shape->attributes[shape->num_attributes].value, value, shape->attributes[shape->num_attributes].value_length);
  shape->attributes[shape->num_attributes].value[shape->attributes[shape->num_attributes].value_length] = 0;
  shape->num_attributes++;
}

int clamp_int(int v, int min, int max)
{
  if (v > max) return max;
  if (v < min) return min;
  return v;
}

float clamp_float(float v, float min, float max)
{
  if (v > max) return max;
  if (v < min) return min;
  return v;
}

int free_shape(struct Shape * shape)
{
  unsigned int i = 0;
  for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    free(shape->vertex_arrays[i].vertexs);
    
  for (i = 0 ; i < shape->num_attributes ; i++)
    free(shape->attributes[i].value);
    
  if (shape->num_vertex_arrays) free(shape->vertex_arrays);
  if (shape->num_attributes) free(shape->attributes);
  free(shape);
  return 1;
}

void free_all_shapes(struct Shape ** shapes, unsigned int num_shapes)
{
  unsigned int i = 0;
  for (i = 0 ; i < num_shapes ; i++)
    free_shape(shapes[i]);
  free(shapes);
}

struct BBox * get_bbox(struct Shape * shape, struct BBox * bbox)
{
  if (bbox == NULL)
  {
    bbox = (struct BBox *)malloc(sizeof(struct BBox));
    bbox->num_minmax = 0;
    bbox->minmax = NULL;
  }
  
  long j, k;
  struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
  
  if (va->num_dimensions > bbox->num_minmax)
  {
    bbox->minmax = (struct MinMax*)realloc(bbox->minmax, sizeof(struct MinMax)*va->num_dimensions);
    for (j = bbox->num_minmax ; j < va->num_dimensions ; j++)
    {
      bbox->minmax[j].min =  FLT_MAX;
      bbox->minmax[j].max = -FLT_MAX;
    }
    bbox->num_minmax = va->num_dimensions;
  }
  
  for (j = 0 ; j < shape->num_vertexs ; j++)
  {
    for (k = 0 ; k < va->num_dimensions ; k++)
    {
      if (bbox->minmax[k].min > va->vertexs[j*va->num_dimensions+k])
          bbox->minmax[k].min = va->vertexs[j*va->num_dimensions+k];
      if (bbox->minmax[k].max < va->vertexs[j*va->num_dimensions+k])
          bbox->minmax[k].max = va->vertexs[j*va->num_dimensions+k];
    }
  }
  return bbox;
}

struct BBox * get_bbox_from_shapes(struct Shape ** shapes, int num_shapes)
{
  struct BBox * bbox = NULL;
  
  long i;
  for (i = 0 ; i < num_shapes ; i++)
    bbox = get_bbox(shapes[i], bbox);
  
  return bbox;
}

struct Shape * get_shape_from_bbox(struct BBox * bbox)
{
  struct Shape * shape = new_shape();
  struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
  shape->gl_type = GL_LINE_LOOP;
  va->num_dimensions = bbox->num_minmax;
  float v[5] = {0, 0, 0, 0, 0};
  
  long i;
  for (i = 0 ; i < bbox->num_minmax ; i++)
    v[i] = bbox->minmax[i].min;
  
  if (bbox->num_minmax == 2)
  {
    append_vertex(shape, v);
    v[0] = bbox->minmax[0].max;
    append_vertex(shape, v);
    v[0] = bbox->minmax[0].max;
    v[1] = bbox->minmax[1].max;
    append_vertex(shape, v);
    v[0] = bbox->minmax[0].min;
    v[1] = bbox->minmax[1].max;
    append_vertex(shape, v);
  }
  
  return shape;
}

void free_bbox(struct BBox * bbox)
{
  free(bbox->minmax);
  free(bbox);
}

/*void write_command_string(FILE * fp)
{
  //if (has_wrote_command_string) return;
  //has_wrote_command_string = 1;
  float ninf = -INFINITY;
  if (fwrite(&ninf, sizeof(ninf), 1, fp) != 1) return;
  uint32_t thing_version = CURRENT_VERSION;
  if (fwrite(&thing_version, sizeof(thing_version), 1, fp) != 1) return;
  uint32_t thing_type = COMMAND_STRING;
  if (fwrite(&thing_type, sizeof(thing_type), 1, fp) != 1) return;
  uint32_t length = (int)strlen(COMMAND);
  if (fwrite(&length, sizeof(length), 1, fp) != 1) return;
  if (fwrite(&COMMAND, length, 1, fp) != 1) return;
}

int write_string_table(FILE * fp, int num, char ** strings)
{
  if (strings == NULL || num == 0) return num;
  float ninf = -INFINITY;
  if (fwrite(&ninf, sizeof(ninf), 1, fp) != 1) return 0;
  uint32_t thing_version = CURRENT_VERSION;
  if (fwrite(&thing_version, sizeof(thing_version), 1, fp) != 1) return 0;
  uint32_t thing_type = STRING_TABLE;
  if (fwrite(&thing_type, sizeof(thing_type), 1, fp) != 1) return 0;
  uint32_t num_strings = num;
  if (fwrite(&num_strings, sizeof(num_strings), 1, fp) != 1) return 0;
  unsigned int i;
  for (i = 0 ; i < num_strings ; i++)
    fwrite(strings[i], 20, 1, fp);
  return num;
}*/

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

const char * array_type_names[] = {
  "GL_VERTEX_ARRAY",
  "GL_COLOR_ARRAY",
  "GL_NORMAL_ARRAY",
  "GL_INDEX_ARRAY",
  "GL_TEXTURE_COORD_ARRAY",
  "GL_EDGE_FLAG_ARRAY",
  "GL_FOG_COORD_ARRAY",
  "GL_SECONDARY_COLOR_ARRAY"
};

const char * get_array_type_name(int array_type)
{
  if (array_type == GL_VERTEX_ARRAY) return array_type_names[0];
  if (array_type == GL_COLOR_ARRAY) return array_type_names[1];
  if (array_type == GL_NORMAL_ARRAY) return array_type_names[2];
  if (array_type == GL_INDEX_ARRAY) return array_type_names[3];
  if (array_type == GL_TEXTURE_COORD_ARRAY) return array_type_names[4];
  if (array_type == GL_EDGE_FLAG_ARRAY) return array_type_names[5];
  if (array_type == GL_FOG_COORD_ARRAY) return array_type_names[6];
  if (array_type == GL_SECONDARY_COLOR_ARRAY) return array_type_names[7];
  return NULL;
}

int get_array_type(char * array_type_name)
{
  if (strcmp(array_type_name, "GL_VERTEX_ARRAY")==0) return GL_VERTEX_ARRAY;
  if (strcmp(array_type_name, "GL_COLOR_ARRAY")==0) return GL_COLOR_ARRAY;
  if (strcmp(array_type_name, "GL_NORMAL_ARRAY")==0) return GL_NORMAL_ARRAY;
  if (strcmp(array_type_name, "GL_INDEX_ARRAY")==0) return GL_INDEX_ARRAY;
  if (strcmp(array_type_name, "GL_TEXTURE_COORD_ARRAY")==0) return GL_TEXTURE_COORD_ARRAY;
  if (strcmp(array_type_name, "GL_EDGE_FLAG_ARRAY")==0) return GL_EDGE_FLAG_ARRAY;
  if (strcmp(array_type_name, "GL_FOG_COORD_ARRAY")==0) return GL_FOG_COORD_ARRAY;
  if (strcmp(array_type_name, "GL_SECONDARY_COLOR_ARRAY")==0) return GL_SECONDARY_COLOR_ARRAY;
  return 0;
}

const char * gl_type_names[] = {
  "GL_POINTS",
  "GL_LINES",
  "GL_LINE_LOOP",
  "GL_LINE_STRIP",
  "GL_TRIANGLES",
  "GL_TRIANGLE_STRIP",
  "GL_TRIANGLE_FAN"
};

const char * get_gl_type_name(int gl_type)
{
  if (gl_type == GL_POINTS) return gl_type_names[0];
  if (gl_type == GL_LINES) return gl_type_names[1];
  if (gl_type == GL_LINE_LOOP) return gl_type_names[2];
  if (gl_type == GL_LINE_STRIP) return gl_type_names[3];
  if (gl_type == GL_TRIANGLES) return gl_type_names[4];
  if (gl_type == GL_TRIANGLE_STRIP) return gl_type_names[5];
  if (gl_type == GL_TRIANGLE_FAN) return gl_type_names[6];
  return NULL;
}

int get_gl_type(char * gl_type_name)
{
  if (strcmp(gl_type_name, "GL_POINTS")==0) return GL_POINTS;
  if (strcmp(gl_type_name, "GL_LINES")==0) return GL_LINES;
  if (strcmp(gl_type_name, "GL_LINE_LOOP")==0) return GL_LINE_LOOP;
  if (strcmp(gl_type_name, "GL_LINE_STRIP")==0) return GL_LINE_STRIP;
  if (strcmp(gl_type_name, "GL_TRIANGLES")==0) return GL_TRIANGLES;
  if (strcmp(gl_type_name, "GL_TRIANGLE_STRIP")==0) return GL_TRIANGLE_STRIP;
  if (strcmp(gl_type_name, "GL_TRIANGLE_FAN")==0) return GL_TRIANGLE_FAN;
  return 0;
}
