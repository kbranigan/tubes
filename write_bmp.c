
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "scheme.h"

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/osmesa.h>
#endif

GLuint textureId;
GLuint rboId;
GLuint fboId;
int TEXTURE_WIDTH = 1024;
int TEXTURE_HEIGHT = 0;

int setup_offscreen_render(float min_x, float max_x, float min_y, float max_y, float min_z, float max_z)
{
  TEXTURE_HEIGHT = TEXTURE_WIDTH * ((max_y - min_y) / (max_x - min_x));
  if (TEXTURE_HEIGHT > TEXTURE_WIDTH * 1.5) TEXTURE_HEIGHT = TEXTURE_WIDTH * 1.5;
  
  #ifdef __APPLE__
  
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
  
  #else
  
  OSMesaContext ctx = OSMesaCreateContextExt(OSMESA_RGBA, 16, 0, 0, NULL);
  if (!ctx) { fprintf(stderr, "OSMesaCreateContext failed!\n"); return EXIT_FAILURE; }
  void *buffer = malloc(TEXTURE_WIDTH * TEXTURE_HEIGHT * 4 * sizeof(GLubyte));
  if (!OSMesaMakeCurrent(ctx, buffer, GL_UNSIGNED_BYTE, TEXTURE_WIDTH, TEXTURE_HEIGHT)) { printf("OSMesaMakeCurrent failed!\n"); return EXIT_FAILURE; }
  
  #endif
  
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
  
  glOrtho(min_x-diff_x*0.1, max_x+diff_x*0.1, min_y-diff_y*0.1, max_y+diff_y*0.1, -100, 100);
  //glOrtho(-1.5, 1.5, 1.5, -1.5, -1.5, 1.5);
  
  //printf("[%f, %f], [%f, %f], [%f, %f]\n", min_x, max_x, min_y, max_y, min_z, max_z);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  //glTranslatef((max_x + min_x)/2.0, (max_y + min_y)/2.0, (max_z + min_z)/2.0);
  //glRotatef(90, 0, 0, 1);
  //glTranslatef(-(max_x + min_x)/2.0, -(max_y + min_y)/2.0, -(max_z + min_z)/2.0);
  
  //glRotatef(-90, 1, 0, 0);
  //glRotatef(min_y + (max_y-min_y)/2.0, 1, 0, 0);
  //glRotatef(min_x + (max_x-min_x)/2.0, 0, -1, 0);
  //glRotatef(90, 0, 0, -1);
  //gluLookAt(0,0,50, 0,0,0, 0,1,0);
  
  return EXIT_SUCCESS;
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

/*
FILE * vertex_file = NULL;
void vertex3dv(const GLdouble * c)
{
  //printf("%f %f %f\n", c[0], c[1], c[2]);
  float x,y,z;
  get_sphere_coords_from_latlng(c[0], c[1], &x, &y, &z);
  fwrite(&x, sizeof(x), 1, vertex_file);
  fwrite(&y, sizeof(y), 1, vertex_file);
  fwrite(&z, sizeof(z), 1, vertex_file);
}

void beginCallback(GLenum which)
{
  //if (which == GL_TRIANGLES) printf("begin triangles\n");
  //else if (which == GL_TRIANGLE_STRIP) printf("begin triangle strip\n");
  //else if (which == GL_TRIANGLE_FAN) printf("begin triangle fan\n");
  //else printf("begin %d\n", which);
  //float t = INFINITY;
  fwrite(&t, sizeof(t), 1, vertex_file);
  fwrite(&which, sizeof(which), 1, vertex_file);
  //glBegin(which);
}

void endCallback(void)
{
  //glEnd();
}

void errorCallback(GLenum errorCode)
{
   const GLubyte *estring;

   estring = gluErrorString(errorCode);
   printf ("Tessellation Error: %s\n", estring);
   exit (0);
}

void combineCallback(GLdouble coords[3], 
                     GLdouble *vertex_data[4],
                     GLfloat weight[4], GLdouble **dataOut )
{
  GLdouble *vertex;
  int i;

  vertex = (GLdouble *) malloc(3 * sizeof(GLdouble));
  vertex[0] = coords[0];
  vertex[1] = coords[1];
  vertex[2] = coords[2];
  //for (i = 3; i < 7; i++)
  //  vertex[i] = weight[0] * vertex_data[0][i] 
  //                + weight[1] * vertex_data[1][i]
  //                + weight[2] * vertex_data[2][i] 
  //                + weight[3] * vertex_data[3][i];
  // *dataOut = vertex;
}
*/

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
  
  double b[3][2] = {
    {10000000, -10000000},
    {10000000, -10000000},
    {10000000, -10000000}
  };
  
  double x,y,z;
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
  
  //write_image(filename, TEXTURE_WIDTH, TEXTURE_HEIGHT);
  //fprintf(stderr, "%s: %dx%d bmp created named '%s'\n", argv[0], TEXTURE_WIDTH, TEXTURE_HEIGHT, filename);
  
  /*GLUtesselator * tobj = NULL;
  if (strcmp(tess, "tess")==0)
  {
    printf("tesselation being used\n");
    tobj = gluNewTess();
    gluTessCallback(tobj, GLU_TESS_VERTEX, (GLvoid (*) ()) &vertex3dv);
    gluTessCallback(tobj, GLU_TESS_BEGIN, (GLvoid (*) ()) &beginCallback);
    gluTessCallback(tobj, GLU_TESS_END, (GLvoid (*) ()) &endCallback);
    gluTessCallback(tobj, GLU_TESS_ERROR, (GLvoid (*) ()) &errorCallback);
    gluTessCallback(tobj, GLU_TESS_COMBINE, (GLvoid (*) ()) &combineCallback);

    gluTessBeginPolygon(tobj, NULL);
    gluTessBeginContour(tobj);
  }
  
  
  
  int count = 0;
  vertex_file = fopen("some_vertexes", "wb");
  sprintf(query, "SELECT sp.x, sp.y, sp.dbf_id, sp.part_id FROM shape_points sp LEFT JOIN DBF USING(dbf_id) %s ", where);
  if (mysql_query(&mysql, query)==0)
  {
    int prev_dbf_id = 0; int prev_part_id = 0;
	  res = mysql_store_result(&mysql);
    int num_rows = mysql_num_rows(res);
    GLdouble * vertexes = (GLdouble*)malloc(sizeof(GLdouble)*3*num_rows);
    while ((row = mysql_fetch_row(res)))
    {
      //float lat = atof(row[0]) / 180.0 * 3.14159265;//(atof(row[0]) - min_x) / (max_x - min_x) * TEXTURE_WIDTH;
      //float lng = atof(row[1]) / 180.0 * 3.14159265;//(atof(row[1]) - min_y) / (max_y - min_y) * TEXTURE_HEIGHT;
      //int pos = y*TEXTURE_WIDTH*3 + x*3;
      //if (pos + 2 >= TEXTURE_WIDTH*TEXTURE_HEIGHT*3) { printf("x,y = [%s,%s] or [%d, %d] is not within min/max (%d > %d)\n", row[0], row[1], x, y, pos, (TEXTURE_WIDTH*TEXTURE_HEIGHT*3)); continue; }
      
      int dbf_id = atoi(row[2]);
      int part_id = atoi(row[3]);
      
      if ((dbf_id != prev_dbf_id && prev_dbf_id != 0) || (part_id != prev_part_id && prev_part_id != 0))
      {
        //printf("count = %d\n", count);
        //break;
        if (tobj != NULL)
        {
          gluTessEndContour(tobj);
          gluTessBeginContour(tobj);
        }
        else
        {
          float t = INFINITY;
          fwrite(&t, sizeof(t), 1, vertex_file);
          GLenum which = GL_LINE_STRIP;
          fwrite(&which, sizeof(which), 1, vertex_file);
        }

        //
        //fprintf(fp, "\n");
        //glEnd();
        //glBegin(GL_LINE_STRIP);
      }
      
      //float from_x_center = 0.5 - ((max_x - x) / (max_x - min_x));
      //float from_y_bottom = 1.0 - ((max_y - y) / (max_y - min_y));
      
      //x = x + (max_x - min_x)*from_x_center*from_y_bottom;
      //glColor4f(from_y_bottom, from_y_bottom, from_y_bottom, 1);
      
      //NxVec3 pos = get_sphere_coords_from_latlng(atof(row[0]), atof(row[1]));
      //glVertex3f(pos[0], pos[1], pos[2]);
      
      vertexes[count*3] = atof(row[1]);
      vertexes[count*3+1] = atof(row[0]);
      vertexes[count*3+2] = 0;
      //printf("%f %f %f\n", c[0], c[1], c[2]);
      if (tobj != NULL)
      {
        gluTessVertex(tobj, &vertexes[count*3], &vertexes[count*3]);
      }
      else
      {
        float x,y,z;
        get_sphere_coords_from_latlng(atof(row[1]), atof(row[0]), &x, &y, &z);
        glVertex3f(x,y,z);
        fwrite(&x, sizeof(x), 1, vertex_file);
        fwrite(&y, sizeof(y), 1, vertex_file);
        fwrite(&z, sizeof(z), 1, vertex_file);
      }
      
      
      prev_dbf_id = dbf_id;
      prev_part_id = part_id;
      count++;
    }
    if (tobj != NULL)
    {
      gluTessEndContour(tobj);
      gluTessEndPolygon(tobj);
    }

    //fputc('\n', vertex_file);
    mysql_free_result(res);
  }
  else printf("mysql error: %s\n", mysql_error(&mysql));
  fclose(vertex_file);
  
  printf("count = %d\n", count);
  //glEnd();
  
  mysql_close(&mysql);
  */
  
}


















