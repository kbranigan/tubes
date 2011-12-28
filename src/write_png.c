
#include <png.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION write_png
#include "scheme.h"

#include "setup_opengl.c"
//#include "polygon_offset.c"

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} pixel_t;
  
typedef struct  {
  pixel_t *pixels;
  size_t width;
  size_t height;
} bitmap_t;

static pixel_t * pixel_at(bitmap_t * bitmap, int x, int y)
{
  return bitmap->pixels + bitmap->width * y + x;
}

static int _write_png(bitmap_t *bitmap, const char *path)
{
  FILE * fp;
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  size_t x, y;
  png_byte ** row_pointers = NULL;
  
  int status = -1;
  int pixel_size = 3; // This number is set by trial and error only. I cannot see where it it is documented in the libpng manual.
  int depth = 8;
  
  fp = fopen(path, "wb");
  if(!fp)
    goto fopen_failed;
  
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(png_ptr == NULL)
    goto png_create_write_struct_failed;
  
  info_ptr = png_create_info_struct(png_ptr);
  if(info_ptr == NULL)
    goto png_create_info_struct_failed;
  
  //if(setjmp(png_jmpbuf(png_ptr)))
  //  goto png_failure;
  
  png_set_IHDR(png_ptr, info_ptr, bitmap->width, bitmap->height, depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  
  row_pointers = png_malloc(png_ptr, bitmap->height * sizeof(png_byte *));
  if (row_pointers == NULL) { fprintf(stderr, "png_malloc failed for pointers :(\n"); exit(0); }
  for(y = 0; y < bitmap->height; ++y)
  {
    png_byte *row = png_malloc(png_ptr, sizeof(uint8_t) * bitmap->width * pixel_size);
    if (row_pointers == NULL) { fprintf(stderr, "png_malloc failed for row :(\n"); exit(0); }
    row_pointers[y] = row;
    for(x = 0; x < bitmap->width; ++x)
    {
      pixel_t * pixel = pixel_at(bitmap, x, y);
      *row++ = pixel->red;
      *row++ = pixel->green;
      *row++ = pixel->blue;
    }
  }
  
  png_init_io(png_ptr, fp);
  png_set_rows(png_ptr, info_ptr, row_pointers);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
  
  status = 0;
  
  for(y = 0; y < bitmap->height; y++)
    png_free(png_ptr, row_pointers[y]);
  png_free(png_ptr, row_pointers);
  
  png_failure:
  png_create_info_struct_failed:
  png_destroy_write_struct(&png_ptr, &info_ptr);
  png_create_write_struct_failed:
  fclose(fp);
  fopen_failed:
  return status;
}

int write_png(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int texture_width = 1200;
  
  char file_name[1000] = "";
  float rotation = 0;
  int num_attributes = -1;
  int c;
  while ((c = getopt(argc, argv, "f:w:r:")) != -1)
  switch (c)
  {
    case 'f': strncpy(file_name, optarg, sizeof(file_name)); break;
    case 'w': texture_width = atoi(optarg); break;
    case 'r': rotation = atof(optarg); break;
    default:  abort();
  }
  
  if (file_name[0] == 0 && argc == 2 && argv[1] != NULL)
    strncpy(file_name, argv[1], sizeof(file_name));
  else if (file_name[0] == 0)
  {
    fprintf(stderr, "%s: file name not specified, using 'output.png'\n", argv[0]);
    strcpy(file_name, "output.png");
  }
  
  float b[3][2] = {
    {FLT_MAX, -FLT_MAX},
    {FLT_MAX, -FLT_MAX},
    {FLT_MAX, -FLT_MAX}
  };
  
  float x=0, y=0, z=0;
  int num_shapes = 0;
  struct Shape ** shapes = NULL;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    num_shapes++;
    shapes = (struct Shape **)realloc(shapes, sizeof(struct Shape*)*num_shapes);
    if (shapes == NULL) { fprintf(stderr, "realloc for shapes failed :(\n"); exit(0); }
    shapes[num_shapes-1] = shape;
    
    long j;
    struct VertexArray * va = &shape->vertex_arrays[0];
      
    for (j = 0 ; j < shape->num_vertexs ; j++)
    {
      float * v = get_vertex(shape, 0, j);
      if (va->num_dimensions == 1)
      {
        x = j;
        y = v[0];
      }
      else if (va->num_dimensions >= 2)
      {
        x = v[0];
        y = v[1];
      }
      
      if (va->num_dimensions >= 3)
        z = v[2];
      
      if (x < b[0][0]) b[0][0] = x; if (x > b[0][1]) b[0][1] = x;
      if (y < b[1][0]) b[1][0] = y; if (y > b[1][1]) b[1][1] = y;
      if (z < b[2][0]) b[2][0] = z; if (z > b[2][1]) b[2][1] = z;
    }
  }
  
  int texture_height = texture_width * ((b[1][1] - b[1][0]) / (b[0][1] - b[0][0]));
  if (texture_height > texture_width * 1.5) texture_height = texture_width * 1.5;
  
  if (texture_width <= 1 || texture_height <= 1)
  {
    fprintf(stderr, "%s: ERROR, texture size: %d by %d\n", argv[0], texture_width, texture_height);
    return 0;
  }
  
  if (setup_offscreen_render(b[0][0], b[0][1], b[1][0], b[1][1], b[2][0], b[2][1], texture_width) != EXIT_SUCCESS)
    return EXIT_FAILURE;
  
  glTranslatef((b[0][0]+b[0][1])/2.0, (b[1][0]+b[1][1])/2.0, 0);
  glRotatef(180, 0, 0, 1);
  glRotatef(rotation, 1, 0, 0);
  glScalef(-1, 1, 1);
  glTranslatef((b[0][0]+b[0][1])/-2.0, (b[1][0]+b[1][1])/-2.0, 0);
  
  long i, j, k;
  for (i = 0 ; i < num_shapes ; i++)
  {
    shape = shapes[i];
    if (shape->num_vertexs == 1)
      shape->gl_type = GL_POINTS;
    
    glBegin(shape->gl_type);
    
    if (shape->gl_type == GL_POINTS)
      glColor4f(0, 0, 0, 1);
    else
      glColor4f(0, 0, 0, 0.3);
    
    for (j = 0 ; j < shape->num_vertexs ; j++)
    {
      if (shape->num_vertex_arrays > 1 && shape->vertex_arrays[1].array_type == GL_COLOR_ARRAY)
      {
        struct VertexArray * va = &shape->vertex_arrays[1];
        if (va->num_dimensions == 3)      glColor3fv(get_vertex(shape, 1, j));
        else if (va->num_dimensions == 4) glColor4fv(get_vertex(shape, 1, j));
      }
      struct VertexArray * va = &shape->vertex_arrays[0];
      if (va->num_dimensions == 1)      glVertex2f(j, va->vertexs[j]);
      else if (va->num_dimensions == 2) glVertex2fv(get_vertex(shape, 0, j));
      else if (va->num_dimensions == 3) glVertex3fv(get_vertex(shape, 0, j));
      else if (va->num_dimensions == 4) glVertex4fv(get_vertex(shape, 0, j));
    }
    
    glEnd();
    free_shape(shape);
  }
  free(shapes);
  
  bitmap_t png;
  png.width = texture_width;
  png.height = texture_height;
  
  if (fabs(b[0][0] - b[0][1]) <= 0 || fabs(b[1][0] - b[1][1]) <= 0)
      {
        fprintf(stderr, "content range is: x:(%f => %f) by y:(%f => %f)\n", b[0][0], b[0][1], b[1][0], b[1][1]);
        exit(0);
      }
  
  if (texture_width >= 50000 || texture_height >= 50000)
      {
        fprintf(stderr, "generated image size is definitely wrong (%d x %d)\n", texture_width, texture_height);
        exit(0);
      }
  
  png.pixels = calloc(sizeof(pixel_t), texture_width*texture_height);
  glReadBuffer((GLenum)GL_COLOR_ATTACHMENT0_EXT);
  glReadPixels(0, 0, texture_width, texture_height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)png.pixels);
  
  _write_png(&png, file_name);
  fprintf(stderr, "%s: %dx%d bmp created named '%s'\n", argv[0], texture_width, texture_height, file_name);
}

