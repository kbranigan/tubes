
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
  struct timeval tv;
  int retval;
  
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  
  if (feof(stdin)) return 0;
  
  retval = select(1, &rfds, NULL, NULL, &tv);
  
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
  
  fprintf(stderr, "shape:\n");
  fprintf(stderr, "  unique_set_id: %d\n", shape->unique_set_id);
  fprintf(stderr, "  gl_type: %d\n", shape->gl_type);
  fprintf(stderr, "  num_attributes: %d\n", shape->num_attributes);
  fprintf(stderr, "  num_vertexs: %d\n", shape->num_vertexs);
  fprintf(stderr, "  num_vertex_arrays: %d\n", shape->num_vertex_arrays);
  if (shape->num_vertex_arrays > 0)
  {
    long i;
    for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    {
      fprintf(stderr, "  vertex_arrays:\n");
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
  }
  if (count_zero > 0) fprintf(stderr, "  count_zero: %ld\n", count_zero);
}

int write_shape(FILE * fp, struct Shape * shape)
{
  double inf = 1.0 / 0.0;
  if (fwrite(&inf, sizeof(inf), 1, fp) != 1) return 0;
  if (fwrite(&shape->unique_set_id, sizeof(shape->unique_set_id), 1, fp) != 1) return 0;
  if (fwrite(&shape->num_attributes, sizeof(shape->num_attributes), 1, fp) != 1) return 0;
  
  long i;
  for (i = 0 ; i < shape->num_attributes ; i++)
  {
    
  }
  
  if (fwrite(&shape->gl_type, sizeof(shape->gl_type), 1, fp) != 1) return 0;
  if (fwrite(&shape->num_vertexs, sizeof(shape->num_vertexs), 1, fp) != 1) return 0;
  if (fwrite(&shape->num_vertex_arrays, sizeof(shape->num_vertex_arrays), 1, fp) != 1) return 0;
  
  for (i = 0 ; i < shape->num_vertex_arrays ; i++)
  {
    struct VertexArray * va = &shape->vertex_arrays[i];
    if (fwrite(&va->array_type, sizeof(va->array_type), 1, fp) != 1) return 0;
    if (fwrite(&va->num_dimensions, sizeof(va->num_dimensions), 1, fp) != 1) return 0;
    if (fwrite(va->vertexs, sizeof(double)*va->num_dimensions*shape->num_vertexs, 1, fp) != 1) return 0;
  }
  return 0;
}

struct Shape * read_shape(FILE * fp)
{
  double shape_header;
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
      if (fread(&attribute->key, sizeof(attribute->key), 1, fp) != 1) { fprintf(stderr, "fread attribute %d key error\n", i); return NULL; }
      
      if (fread(&attribute->length, sizeof(attribute->length), 1, fp) != 1) { fprintf(stderr, "fread attribute %d length error\n", i); return NULL; }
      attribute->value = malloc(attribute->length);
      if (fread(&attribute->value, attribute->length, 1, fp) != 1) { fprintf(stderr, "fread attribute %d value error\n", i); return NULL; }
      
      //fprintf(stderr, "%d: %s: %s\n", i, attribute->key, attribute->value);
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
      if (va->num_dimensions != 3) fprintf(stderr, "va->num_dimensions = %d\n", va->num_dimensions);
      
      va->vertexs = (double*)malloc(sizeof(double)*shape->num_vertexs*va->num_dimensions);
      
      if (fread(va->vertexs, sizeof(double)*va->num_dimensions*shape->num_vertexs, 1, fp) != 1) { fprintf(stderr, "fread vertex_array %d vertexs error\n", i); return NULL; }
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

/////////////////////////////////////////////////////////////////////////////////////////////////

/*int main(int argc, char *argv[])
{
  FILE * fp = fopen("garf", "wb");
  uint32_t temp;
  double temf;
  temp = 42;     assert(fwrite(&temp, sizeof(temp), 1, fp) == 1);
  temp = 2;      assert(fwrite(&temp, sizeof(temp), 1, fp) == 1);
  
  temf = 1/0.0;  assert(fwrite(&temp, sizeof(temp), 1, fp) == 1);
  temp = 0;      assert(fwrite(&temp, sizeof(temp), 1, fp) == 1);
  temp = 1;      assert(fwrite(&temp, sizeof(temp), 1, fp) == 1);
  {
    char temc[20] = "itsakey";
    temp = strlen(temc);
    fwrite(&temc, sizeof(temc), 1, fp);
    sprintf(temc, "kitsvalue");
    temp = strlen(temc) + 1;      assert(fwrite(&temp, sizeof(temp), 1, fp) == 1);
    temp = strlen(temc);
    fwrite(&temc, sizeof(temc), 1, fp);
  }
  
  temp = GL_TRIANGLES; assert(fwrite(&temp, sizeof(temp), 1, fp) == 1);
  temp = 3;      assert(fwrite(&temp, sizeof(temp), 1, fp) == 1);
  temp = 1;      assert(fwrite(&temp, sizeof(temp), 1, fp) == 1);
  
  
  
  fclose(fp);
  
  fp = fopen("garf", "rb");
  if (open_file(fp, 2))
  {
    struct Shape * shape = NULL;
    while ((shape = read_shape(fp)))
    {
      printf(" read shape\n");
      
      //free(shape);
    }
    printf("open succeeded\n");
  }
  printf("done\n");
}*/

/*struct Shapes * load_shapes(FILE * fp)
{
  uint32_t file_header;
  if (fread(&file_header, sizeof(file_header), 1, fp) != 1) { fprintf(stderr, "fread error 1\n"); exit(1); }
  if (file_header != 42) { fprintf(stderr, "file_header != 42\n"); exit(1); }
  
  struct Shapes * s = (struct Shapes *)malloc(sizeof(struct Shapes));
  memset(s, 0, sizeof(struct Shapes));
  
  //if (fread(&s->num_shapes, sizeof(s->num_shapes), 1, fp) != 1) { fprintf(stderr, "fread error 2\n"); exit(1); }
  //s->shapes = (struct Shape *)malloc(sizeof(struct Shape)*s->num_shapes);
  
  //for (i = 0 ; i < s->num_shapes ; i++)
  while (stdin_has_data())
  {
    s->num_shapes++;
    s->shapes = (struct Shape *)realloc(s->shapes, sizeof(struct Shape)*s->num_shapes);
    struct Shape * sh = &s->shapes[s->num_shapes-1];
    
    double shape_start;
    int ret = fread(&shape_start, sizeof(shape_start), 1, stdin);
    if (ret != 1) { s->num_shapes--; break; }
    
    if (shape_start != (1.0 / 0.0)) { fprintf(stderr, "shape_start != inf\n"); exit(1); }
    
    if (fread(&sh->unique_id, sizeof(sh->unique_id), 1, stdin) != 1) { fprintf(stderr, "fread error 4\n"); exit(1); }
    if (fread(&sh->name, sizeof(sh->name), 1, stdin) != 1) { fprintf(stderr, "fread error 5\n"); exit(1); }
    if (fread(&sh->frame_type, sizeof(sh->frame_type), 1, stdin) != 1) { fprintf(stderr, "fread error 6\n"); exit(1); }
    if (fread(&sh->vertex_type, sizeof(sh->vertex_type), 1, stdin) != 1) { fprintf(stderr, "fread error 7\n"); exit(1); }
    if (sh->vertex_type != GL_VERTEX_ARRAY) { fprintf(stderr, "vertex_type != GL_VERTEX_ARRAY\n"); exit(1); }
    if (fread(&sh->number_of_vertexs, sizeof(sh->number_of_vertexs), 1, stdin) != 1) { fprintf(stderr, "fread error 8\n"); exit(1); }
    
    sh->data = (double*)malloc(sizeof(double)*3*sh->number_of_vertexs);
    memset(sh->data, 0, sizeof(double)*3*sh->number_of_vertexs);
    
    long j;
    for (j = 0 ; j < sh->number_of_vertexs ; j++)
    {
      if (fread(sh->data+j*3, sizeof(double), 1, fp) != 1) { fprintf(stderr, "fread data error 9\n"); exit(1); }
      if (fread(sh->data+j*3+1, sizeof(double), 1, fp) != 1) { fprintf(stderr, "fread data error 10\n"); exit(1); }
      if (fread(sh->data+j*3+2, sizeof(double), 1, fp) != 1) { fprintf(stderr, "fread data error 11\n"); exit(1); }
    }
  }
  
  return s;
}

void write_shape(struct Shape * s, FILE * fp)
{
  if (fp == NULL) fp = stdout;
  
  double inf = 1.0 / 0.0;
  if (fwrite(&inf, sizeof(inf), 1, fp) != 1) { fprintf(stderr, "\n"); exit(1); };
  
  if (fwrite(&s->unique_id, sizeof(s->unique_id), 1, fp) != 1) { fprintf(stderr, "\n"); exit(1); };
  if (fwrite(s->name, sizeof(s->name), 1, fp) != 1) { fprintf(stderr, "\n"); exit(1); };
  if (fwrite(&s->frame_type, sizeof(s->frame_type), 1, fp) != 1) { fprintf(stderr, "\n"); exit(1); };
  if (fwrite(&s->vertex_type, sizeof(s->vertex_type), 1, fp) != 1) { fprintf(stderr, "\n"); exit(1); };
  if (fwrite(&s->number_of_vertexs, sizeof(s->number_of_vertexs), 1, fp) != 1) { fprintf(stderr, "\n"); exit(1); };
  
  long j;
  for (j = 0 ; j < s->number_of_vertexs ; j++)
  {
    if (fwrite(s->data+j*3, sizeof(double), 1, fp) != 1) { fprintf(stderr, "writing X coord failed.\n"); exit(1); };
    if (fwrite(s->data+j*3+1, sizeof(double), 1, fp) != 1) { fprintf(stderr, "writing Y coord failed.\n"); exit(1); };
    if (fwrite(s->data+j*3+2, sizeof(double), 1, fp) != 1) { fprintf(stderr, "writing Z coord failed.\n"); exit(1); };
  }
}

void write_shapes(struct Shapes * ss, FILE * fp)
{
  uint32_t file_header = 42;
  if (fwrite(&file_header, sizeof(file_header), 1, fp) != 1) { fprintf(stderr, "writing file header failed.\n"); exit(1); };
  
  long i;
  for (i = 0 ; i < ss->num_shapes ; i++)
  {
    write_shape(&ss->shapes[i], fp);
  }
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
