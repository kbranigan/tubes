
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

static int write_png(bitmap_t *bitmap, const char *path)
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
  for(y = 0; y < bitmap->height; ++y)
  {
    png_byte *row = png_malloc(png_ptr, sizeof(uint8_t) * bitmap->width * pixel_size);
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

int main(int argc, char ** argv)
{
  char * filename = argc > 1 ? argv[1] : "output.png";
  
  if (!stdin_is_piped())
  {
    fprintf(stderr, "%s needs a data source. (redirected pipe, using |)\n", argv[0]);
    exit(1);
  }
  
  if (!read_header(stdin, CURRENT_VERSION))
  {
    fprintf(stderr, "read header failed.\n");
    exit(1);
  }
  
  float b[3][2] = {
    {10000000, -10000000},
    {10000000, -10000000},
    {10000000, -10000000}
  };
  
  float x,y,z;
  int num_shapes = 0;
  struct Shape ** shapes = NULL;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(stdin)))
  {
    num_shapes++;
    shapes = (struct Shape **)realloc(shapes, sizeof(struct Shape*)*num_shapes);
    shapes[num_shapes-1] = shape;
    
    long i, j;
    for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    {
      struct VertexArray * va = &shape->vertex_arrays[i];
      if (va->array_type != GL_VERTEX_ARRAY) continue;
      if (va->num_dimensions < 2) fprintf(stderr, "vertex_array has %d dimensions (expected at least 2)\n", va->num_dimensions);
      if (va->vertexs == NULL) { fprintf(stderr, "vertex array %ld is NULL\n", i); exit(1); }
      
      for (j = 0 ; j < shape->num_vertexs ; j++)
      {
        x = va->vertexs[j*va->num_dimensions];
        y = va->vertexs[j*va->num_dimensions+1];
        if (va->num_dimensions >= 3) z = va->vertexs[j*va->num_dimensions+2];
        
        if (x < b[0][0]) b[0][0] = x; if (x > b[0][1]) b[0][1] = x;
        if (y < b[1][0]) b[1][0] = y; if (y > b[1][1]) b[1][1] = y;
        if (va->num_dimensions >= 3) { if (z < b[2][0]) b[2][0] = z; if (z > b[2][1]) b[2][1] = z; }
      }
    }
  }
  
  if (setup_offscreen_render(b[0][0], b[0][1], b[1][0], b[1][1], b[2][0], b[2][1]) != EXIT_SUCCESS)
    return EXIT_FAILURE;
  
  glTranslatef((b[0][0]+b[0][1])/2.0, (b[1][0]+b[1][1])/2.0, 0);
  glRotatef(180, 0, 0, 1);
  glScalef(-1, 1, 1);
  glTranslatef((b[0][0]+b[0][1])/-2.0, (b[1][0]+b[1][1])/-2.0, 0);
  
  long i, j, k;
  for (i = 0 ; i < num_shapes ; i++)
  {
    shape = shapes[i];
    if (shape->num_vertexs == 1)
      shape->gl_type = GL_POINTS;
    
    glBegin(shape->gl_type);
    glColor3f(0,0,0);
    for (j = 0 ; j < shape->num_vertex_arrays ; j++)
    {
      struct VertexArray * va = &shape->vertex_arrays[j];
      if (va->array_type != GL_COLOR_ARRAY) continue;
      if (va->num_dimensions < 3) fprintf(stderr, "vertex_array has %d dimensions (expected at least 3)\n", va->num_dimensions);
      if (va->vertexs == NULL) { fprintf(stderr, "vertex array %ld is NULL\n", j); exit(1); }
      
      if (va->array_type == GL_COLOR_ARRAY && va->num_dimensions == 3)
        for (k = 0 ; k < shape->num_vertexs ; k++)
          glColor3fv(&va->vertexs[k*va->num_dimensions]);
      
      else if (va->array_type == GL_COLOR_ARRAY && va->num_dimensions == 4)
        for (k = 0 ; k < shape->num_vertexs ; k++)
          glColor4fv(&va->vertexs[k*va->num_dimensions]);
    }
    
    for (j = 0 ; j < shape->num_vertex_arrays ; j++)
    {
      struct VertexArray * va = &shape->vertex_arrays[j];
      if (va->array_type != GL_VERTEX_ARRAY) continue;
      if (va->num_dimensions < 2) fprintf(stderr, "vertex_array has %d dimensions (expected at least 2)\n", va->num_dimensions);
      if (va->vertexs == NULL) { fprintf(stderr, "vertex array %ld is NULL\n", j); exit(1); }
      
      /*float *x = (float*)malloc(sizeof(float)*shape->num_vertexs);
      float *y = (float*)malloc(sizeof(float)*shape->num_vertexs);
      
      for (k = 0 ; k < shape->num_vertexs ; k++)
      {
        x[k] = va->vertexs[k*va->num_dimensions+0];
        y[k] = va->vertexs[k*va->num_dimensions+1];
      }
      
      insetPolygon(x, y, shape->num_vertexs-1, 0.001);
      printf("\n");//*/
      
      if (va->array_type == GL_VERTEX_ARRAY && va->num_dimensions == 2)
        for (k = 0 ; k < shape->num_vertexs ; k++)
          glVertex2fv(&va->vertexs[k*va->num_dimensions]);
      
      else if (va->array_type == GL_VERTEX_ARRAY && va->num_dimensions == 3)
        for (k = 0 ; k < shape->num_vertexs ; k++)
          glVertex3fv(&va->vertexs[k*va->num_dimensions]);
      
      else if (va->array_type == GL_VERTEX_ARRAY && va->num_dimensions == 4)
        for (k = 0 ; k < shape->num_vertexs ; k++)
          glVertex4fv(&va->vertexs[k*va->num_dimensions]);
      
    }
    glEnd();
    free_shape(shape);
  }
  
  bitmap_t png;
  png.width = TEXTURE_WIDTH;
  png.height = TEXTURE_HEIGHT;
  
  png.pixels = calloc(sizeof(pixel_t), TEXTURE_WIDTH*TEXTURE_HEIGHT);
  glReadBuffer((GLenum)GL_COLOR_ATTACHMENT0_EXT);
  glReadPixels(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)png.pixels);
  
  write_png(&png, filename);
  fprintf(stderr, "%s: %dx%d bmp created named '%s'\n", argv[0], TEXTURE_WIDTH, TEXTURE_HEIGHT, filename);
}

