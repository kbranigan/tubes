
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "scheme.h"

#include "setup_opengl.c"

int write_image(char * file_name, unsigned int width, unsigned int height)
{
  unsigned char * image_data = (unsigned char *)malloc(TEXTURE_WIDTH*TEXTURE_HEIGHT*sizeof(unsigned char)*3);
  glReadBuffer((GLenum)GL_COLOR_ATTACHMENT0_EXT);
  glReadPixels(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_BGR, GL_UNSIGNED_BYTE, (GLvoid*)image_data);
  
  FILE *fp = fopen(file_name, "wb");
  
  // Reference from : http://en.wikipedia.org/wiki/BMP_file_format#File_Structure
  
  int i = 0; int j = 0;
  int image_data_size_with_row_padding =
    (width*3 + ((((width*3) % 4) != 0) == 0 ? 0 : (4-((width*3) % 4)))) * height;
  
  //printf("width = %d\n", width);
  //printf("height = %d\n", height);
  //printf("image_data_size_with_row_padding = %d\n", image_data_size_with_row_padding);
  
  // BMP Header
  fputc(0x42, fp);  fputc(0x4D, fp);                                      // "BM"                 // Magic Number (unsigned integer 66, 77)
  int file_size = 14 + 40 + image_data_size_with_row_padding;                                     // Size of the BMP file
  fwrite(&file_size, 1, sizeof(int), fp);
  fputc(0x00, fp);  fputc(0x00, fp);  fputc(0x00, fp);  fputc(0x00, fp);  // 0                    // Application Specific
  fputc(0x36, fp);  fputc(0x00, fp);  fputc(0x00, fp);  fputc(0x00, fp);  // 54 bytes             // The offset where the Pixel Array (bitmap data) can be found.
  
  // DIB Header
  fputc(0x28, fp);  fputc(0x00, fp);  fputc(0x00, fp);  fputc(0x00, fp);  // 40 bytes             // The number of bytes in the DIB header (from this point).
  fwrite(&width, 1, sizeof(int), fp);                                                             // The width of the bitmap in pixels
  fwrite(&height, 1, sizeof(int), fp);                                                            // The height of the bitmap in pixels
  fputc(0x01, fp);  fputc(0x00, fp);                                      // 1 plane              // Number of color planes being used.
  fputc(0x18, fp);  fputc(0x00, fp);                                      // 24 bits              // The number of bits per pixel.
  fputc(0x00, fp);  fputc(0x00, fp);  fputc(0x00, fp);  fputc(0x00, fp);  // 0                    // BI_RGB, no Pixel Array compression used
  fwrite(&image_data_size_with_row_padding, 1, sizeof(int), fp);                                  // The size of the raw data in the Pixel Array (incl. padding)
  fputc(0x13, fp);  fputc(0x0B, fp);  fputc(0x00, fp);  fputc(0x00, fp);  // 2,835 pixels/meter   // The horizontal resolution of the image
  fputc(0x13, fp);  fputc(0x0B, fp);  fputc(0x00, fp);  fputc(0x00, fp);  // 2,835 pixels/meter 	// The vertical resolution of the image
  fputc(0x00, fp);  fputc(0x00, fp);  fputc(0x00, fp);  fputc(0x00, fp);  // 0 colors             // Number of colors in the palette
  fputc(0x00, fp);  fputc(0x00, fp);  fputc(0x00, fp);  fputc(0x00, fp);  // 0 important colors   // Means all colors are important
  
  // Start of Pixel Array (bitmap data)
  for (i = 0 ; i < height ; i++)
  {
    for (j = 0 ; j < width ; j++)
    {
      fputc(image_data[i*width*3+j*3+0], fp);
      fputc(image_data[i*width*3+j*3+1], fp);
      fputc(image_data[i*width*3+j*3+2], fp);
    }
    
    if (((width*3) % 4) != 0)
      for (j = 0 ; j < 4-((width*3) % 4) ; j++)
        fputc(0, fp);
  }
  
  fclose(fp);
  free(image_data);
}

int main(int argc, char ** argv)
{
  char * filename = argc > 1 ? argv[1] : "output.bmp";
  int draw_individual_shapes = 0;
  
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
  
  if (!draw_individual_shapes)
    if (setup_offscreen_render(b[0][0], b[0][1], b[1][0], b[1][1], b[2][0], b[2][1]) != EXIT_SUCCESS)
      return EXIT_FAILURE;
  
  long i, j, k;
  for (i = 0 ; i < num_shapes ; i++)
  {
    if (draw_individual_shapes)
      if (setup_offscreen_render(b[0][0], b[0][1], b[1][0], b[1][1], b[2][0], b[2][1]) != EXIT_SUCCESS)
        return EXIT_FAILURE;
    
    shape = shapes[i];
    glBegin(shape->gl_type);
    glColor3f(0,0,0);
    for (j = 0 ; j < shape->num_vertex_arrays ; j++)
    {
      struct VertexArray * va = &shape->vertex_arrays[j];
      if (va->array_type != GL_COLOR_ARRAY) continue;
      if (va->num_dimensions < 3) fprintf(stderr, "vertex_array has %d dimensions (expected at least 3)\n", va->num_dimensions);
      if (va->vertexs == NULL) { fprintf(stderr, "vertex array %ld is NULL\n", j); exit(1); }
      
      for (k = 0 ; k < shape->num_vertexs ; k++)
      {
        if (va->array_type == GL_COLOR_ARRAY && va->num_dimensions == 3) glColor3fv(&va->vertexs[k*va->num_dimensions]);
        if (va->array_type == GL_COLOR_ARRAY && va->num_dimensions == 4) glColor4fv(&va->vertexs[k*va->num_dimensions]);
      }
    }
    
    for (j = 0 ; j < shape->num_vertex_arrays ; j++)
    {
      struct VertexArray * va = &shape->vertex_arrays[j];
      if (va->array_type != GL_VERTEX_ARRAY) continue;
      if (va->num_dimensions < 2) fprintf(stderr, "vertex_array has %d dimensions (expected at least 2)\n", va->num_dimensions);
      if (va->vertexs == NULL) { fprintf(stderr, "vertex array %ld is NULL\n", j); exit(1); }
      
      for (k = 0 ; k < shape->num_vertexs ; k++)
      {
        if (va->array_type == GL_VERTEX_ARRAY && va->num_dimensions == 2) glVertex2fv(&va->vertexs[k*va->num_dimensions]);
        if (va->array_type == GL_VERTEX_ARRAY && va->num_dimensions == 3) glVertex3fv(&va->vertexs[k*va->num_dimensions]);
        if (va->array_type == GL_VERTEX_ARRAY && va->num_dimensions == 4) glVertex4fv(&va->vertexs[k*va->num_dimensions]);
      }
    }
    glEnd();
    free_shape(shape);
    
    if (draw_individual_shapes)
    {
      char f[100];
      sprintf(f, "output/%ld.bmp", i);
      write_image(f, TEXTURE_WIDTH, TEXTURE_HEIGHT);
      fprintf(stderr, "%s: %dx%d bmp created named '%s'\n", argv[0], TEXTURE_WIDTH, TEXTURE_HEIGHT, f);
    }
  }
  
  if (!draw_individual_shapes)
  {
    write_image(filename, TEXTURE_WIDTH, TEXTURE_HEIGHT);
    fprintf(stderr, "%s: %dx%d bmp created named '%s'\n", argv[0], TEXTURE_WIDTH, TEXTURE_HEIGHT, filename);
  }
}


















