
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/time.h> // for select()

#include "scheme.h"

int stdin_is_piped()
{
  //setbuf(stdout, NULL);
  return stdin_has_data();
}

int stdout_is_piped()
{
  struct stat outstat = {0};
  struct stat errstat = {0};
  fstat(1, &outstat);
  fstat(2, &errstat);
  
  //setbuf(stdout, NULL);
  return (memcmp(&outstat, &errstat, sizeof outstat) != 0);
}

int stdin_has_data()
{
  fd_set rfds;
  //struct timeval tv;
  int retval;
  
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  //tv.tv_sec = 5;
  //tv.tv_usec = 0;
  
  if (feof(stdin)) return 0;
  
  //retval = select(1, &rfds, NULL, NULL, &tv);
  retval = select(1, &rfds, NULL, NULL, NULL);
  
  if (retval == -1) { fprintf(stderr, "select() on stdin failed.\n"); exit(1); }
  return (retval != 0);
}

int write_header(FILE * fp, uint32_t file_version)
{
  char error[100] = "";
  
  uint32_t file_header = 42;
  if (fwrite(&file_header, sizeof(file_header), 1, stdout) != 1) sprintf(error, "fwrite error.\n");
  if (file_version != CURRENT_VERSION) sprintf(error, "fwrite file_version != CURRENT_VERSION (%d)\n", CURRENT_VERSION);
  if (fwrite(&file_version, sizeof(file_version), 1, stdout) != 1) sprintf(error, "fwrite error.\n");
  
  if (strlen(error) > 0)
  {
    fprintf(stderr, "%s", error);
    exit(1);
  }
  return 1;
}

int read_header(FILE * fp, uint32_t req_file_version)
{
  char error[100] = "";
  
  uint32_t file_header;
  uint32_t file_version;
  
  if (fread(&file_header, sizeof(file_header), 1, fp) != 1) sprintf(error, "fread file_header error\n");
  if (file_header != 42) sprintf(error, "file_header != 42\n");
  if (fread(&file_version, sizeof(file_version), 1, fp) != 1) sprintf(error, "fread file_version error\n");
  if (req_file_version != 0 && file_version != req_file_version) sprintf(error, "file_version (%d) != req_file_version (%d)\n", file_version, req_file_version);
  if (file_version > CURRENT_VERSION) sprintf(error, "file_version (%d) > CURRENT_VERSION (%d)\n", file_version, CURRENT_VERSION);
  
  if (strlen(error) > 0)
  {
    fprintf(stderr, "%s", error);
    exit(1);
  }
  return 1;
}

void inspect_shape(FILE * fp, struct Shape * shape)
{
  long count_zero = 0;
  long i;
  
  fprintf(stderr, "shape:\n");
  fprintf(stderr, "  unique_set_id: %d\n", shape->unique_set_id);
  fprintf(stderr, "  gl_type: %d\n", shape->gl_type);
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
  float inf = 1.0 / 0.0;
  if (fwrite(&inf, sizeof(inf), 1, fp) != 1) return 0;
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
  float shape_header;
  if (fread(&shape_header, sizeof(shape_header), 1, fp) != 1) return NULL;
  if (shape_header != INFINITY) { fprintf(stderr, "shape header (%f) is not infinity\n", shape_header); return NULL; }
  
  struct Shape * shape = (struct Shape *)malloc(sizeof(struct Shape));
  memset(shape, 0, sizeof(struct Shape));
  
  if (fread(&shape->unique_set_id, sizeof(shape->unique_set_id), 1, fp) != 1) { fprintf(stderr, "fread unique_set_id error\n"); return NULL; }
  if (fread(&shape->num_attributes, sizeof(shape->num_attributes), 1, fp) != 1) { fprintf(stderr, "fread num_attributes error\n"); return NULL; }
  
  if (shape->num_attributes > 0)
  {
    shape->attributes = (struct Attribute *)malloc(sizeof(struct Attribute)*shape->num_attributes);
    
    int i;
    for (i = 0 ; i < shape->num_attributes ; i++)
    {
      struct Attribute * attribute = &shape->attributes[i];
      
      if (fread(&attribute->name, sizeof(attribute->name), 1, fp) != 1) { fprintf(stderr, "fread attribute %d key error\n", i); return NULL; }
      if (fread(&attribute->value_length, sizeof(attribute->value_length), 1, fp) != 1) { fprintf(stderr, "fread attribute %d value length error\n", i); return NULL; }
      attribute->value = malloc(attribute->value_length+1);
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
    
    int i;
    for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    {
      struct VertexArray * va = &shape->vertex_arrays[i];
      va->shape = shape;
      if (fread(&va->array_type, sizeof(va->array_type), 1, fp) != 1) { fprintf(stderr, "fread vertex_array %d array_type error\n", i); return NULL; }
      if (fread(&va->num_dimensions, sizeof(va->num_dimensions), 1, fp) != 1) { fprintf(stderr, "fread vertex_array %d num_dimensions error\n", i); return NULL; }
      //if (va->num_dimensions != 3) fprintf(stderr, "va->num_dimensions = %d\n", va->num_dimensions);
      
      va->vertexs = (float*)malloc(sizeof(float)*shape->num_vertexs*va->num_dimensions);
      
      if (fread(va->vertexs, sizeof(float)*va->num_dimensions*shape->num_vertexs, 1, fp) != 1) { fprintf(stderr, "fread vertex_array %d vertexs error\n", i); return NULL; }
    }
  }
  
  return shape;
}

int free_shape(struct Shape * shape)
{
  free(shape->vertex_arrays);
  free(shape->attributes);
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
