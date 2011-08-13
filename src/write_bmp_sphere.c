
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION write_bmp_sphere
#include "scheme.h"

#include "setup_opengl.c"

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

int write_bmp_sphere(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * filename = argc > 1 ? argv[1] : "output_sphere.bmp";
  
  float x,y,z;
  
  setup_offscreen_render(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  
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
  glEnd();
  
  long i, j, k;
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
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
        get_sphere_coords_from_latlng(va->vertexs[k*va->num_dimensions+1], va->vertexs[k*va->num_dimensions+0], &x, &y, &z);
        glVertex3f(x, y, z);
      }
    }
    glEnd();
    free_shape(shape);
  }
  
  write_image(filename, TEXTURE_WIDTH, TEXTURE_HEIGHT);
  fprintf(stderr, "%s: %dx%d bmp created named '%s'\n", argv[0], TEXTURE_WIDTH, TEXTURE_HEIGHT, filename);
}


















