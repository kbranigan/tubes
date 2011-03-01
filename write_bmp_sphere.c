
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "scheme.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

GLuint textureId;
GLuint rboId;
GLuint fboId;
int TEXTURE_WIDTH = 1024;
int TEXTURE_HEIGHT = 0;

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

void setup_offscreen_render(float min_x, float max_x, float min_y, float max_y, float min_z, float max_z)
{
  TEXTURE_HEIGHT = TEXTURE_WIDTH * ((max_y - min_y) / (max_x - min_x));
  if (TEXTURE_HEIGHT > TEXTURE_WIDTH * 1.5) TEXTURE_HEIGHT = TEXTURE_WIDTH * 1.5;
  
  CGLContextObj contextObj;
  CGLError err;
  
  CGLPixelFormatAttribute attrs[] =
  {
    kCGLPFADoubleBuffer,
    kCGLPFAAccumSize, (CGLPixelFormatAttribute)32,
    (CGLPixelFormatAttribute)NULL
  };
  
  CGLPixelFormatObj pixFmt;
  GLint numPixelFormats;
  
  err = CGLChoosePixelFormat(attrs, &pixFmt, &numPixelFormats);
  if (err != 0) fprintf(stderr, "CGL error: %d - %s\n", err, CGLErrorString((CGLError)err));
  err = CGLCreateContext(pixFmt, NULL, &contextObj);
  if (err != 0) fprintf(stderr, "CGL error: %d - %s\n", err, CGLErrorString((CGLError)err));
  err = CGLDestroyPixelFormat(pixFmt);
  if (err != 0) fprintf(stderr, "CGL error: %d - %s\n", err, CGLErrorString((CGLError)err));
  err = CGLSetCurrentContext(contextObj);
  if (err != 0) fprintf(stderr, "CGL error: %d - %s\n", err, CGLErrorString((CGLError)err));
  
  // create a texture object
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  
  // create a renderbuffer object to store depth info
  glGenRenderbuffersEXT(1, &rboId);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rboId);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, TEXTURE_WIDTH, TEXTURE_HEIGHT);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
  
  // create a framebuffer object
  glGenFramebuffersEXT(1, &fboId);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId);
  
  // attach the texture to FBO color attachment point
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, textureId, 0);
  
  // attach the renderbuffer to depth attachment point
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rboId);
  
  // check FBO status
  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  //if(status != GL_FRAMEBUFFER_COMPLETE_EXT)
  //  printf("fboUsed = false\n");
  //  fboUsed = false;
  
  glDisable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	glPolygonOffset (1.0f, 1.0f);
	
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_MULTISAMPLE_ARB);
  
  glDisable(GL_TEXTURE_2D);
  
  glViewport(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);
  glClearColor(1, 1, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //gluPerspective(60.0f, (float)(TEXTURE_WIDTH)/TEXTURE_HEIGHT, 1.0f, 100.0f);
  //glOrtho(TEXTURE_WIDTH, -TEXTURE_WIDTH, TEXTURE_HEIGHT, -TEXTURE_HEIGHT, -100, 100);
  
  double diff_x = max_x - min_x;
  double diff_y = max_y - min_y;
  double diff_z = max_z - min_z;
  
  glOrtho(min_x-diff_x*0.01, max_x+diff_x*0.01, max_y+diff_y*0.01, min_y-diff_y*0.01, min_z-diff_z*0.01, max_z+diff_z*0.01);
  //glOrtho(-1.05, 1.05, 1.05, -1.05, -1.05, 1.05);
  
  //printf("[%f, %f], [%f, %f], [%f, %f]\n", min_x, max_x, min_y, max_y, min_z, max_z);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  //glTranslatef((max_x + min_x)/2.0, (max_y + min_y)/2.0, (max_z + min_z)/2.0);
  //glRotatef(45, 0, 0, 1);
  //glTranslatef(-(max_x + min_x)/2.0, -(max_y + min_y)/2.0, -(max_z + min_z)/2.0);
  
  glRotatef(-65, 1, 0, 0);
  glRotatef(180, 0, 0, 1);
  glRotatef(90, 0, 1, 0);
  
  //glRotatef(-90, 1, 0, 0);
  //glRotatef(min_y + (max_y-min_y)/2.0, 1, 0, 0);
  //glRotatef(min_x + (max_x-min_x)/2.0, 0, -1, 0);
  //glRotatef(90, 0, 0, -1);
  //gluLookAt(0,0,50, 0,0,0, 0,1,0);
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

int main(int argc, char ** argv)
{
  char * filename = argc > 1 ? argv[1] : "output_sphere.bmp";
  
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
  
  /*glPushMatrix();
  
  int l, m;
  for (m = -160 ; m <= 180 ; m += 20)
  {
    glRotatef(20, 0, 1, 0);
    
    glBegin(GL_LINE_STRIP);
    for (l = -160 ; l < 180 ; l += 20)
    {
      glColor4f(0, 1, 0, 1); glVertex3f(cos(l/360.0*3.14159265)*0.9999, sin(l/360.0*3.14159265)*0.9999, 0.0);
      glColor4f(1, 0, 0, 1); glVertex3f(cos(l/360.0*3.14159265)*0.9999, sin(l/360.0*3.14159265)*0.9999, 0.0);
      
      glColor4f(0, 1, 0, 1); glVertex3f(cos(l/360.0*3.14159265)*0.9999, sin(l/360.0*3.14159265)*0.9999, 0.0);
      glColor4f(1, 0, 0, 1); glVertex3f(cos(l/360.0*3.14159265)*0.9999, sin(l/360.0*3.14159265)*0.9999, 0.0);
    }
    glEnd();
  }
  glPopMatrix();//*/
  
  glPushMatrix();
  long i, j, k;
  struct Shape * shape = NULL;
  while ((shape = read_shape(stdin)))
  {
    //shape = shapes[i];
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
        if (va->array_type == GL_COLOR_ARRAY && va->num_dimensions == 3) glColor3dv(&va->vertexs[k*va->num_dimensions]);
        if (va->array_type == GL_COLOR_ARRAY && va->num_dimensions == 4) glColor4dv(&va->vertexs[k*va->num_dimensions]);
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
        if (va->array_type == GL_VERTEX_ARRAY && va->num_dimensions == 2) glVertex2dv(&va->vertexs[k*va->num_dimensions]);
        if (va->array_type == GL_VERTEX_ARRAY && va->num_dimensions == 3) glVertex3dv(&va->vertexs[k*va->num_dimensions]);
        if (va->array_type == GL_VERTEX_ARRAY && va->num_dimensions == 4) glVertex4dv(&va->vertexs[k*va->num_dimensions]);
      }
    }
    glEnd();
    free_shape(shape);
  }
  glPopMatrix();
  
  write_image(filename, TEXTURE_WIDTH, TEXTURE_HEIGHT);
  fprintf(stderr, "%s: %dx%d bmp created named '%s'\n", argv[0], TEXTURE_WIDTH, TEXTURE_HEIGHT, filename);
}


















