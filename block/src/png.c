
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

#include <png.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "block.h"
#include "SOIL.h"

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

int setup_offscreen_render(float min_x, float max_x, float min_y, float max_y, float min_z, float max_z, int texture_width)
{
  int texture_height = texture_width * ((max_y - min_y) / (max_x - min_x));
  if (texture_height > texture_width * 1.5) texture_height = texture_width * 1.5;
  
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
  
  OSMesaContext ctx = OSMesaCreateContextExt(OSMESA_RGBA, 32, 8, 16, NULL);
  if (!ctx) { fprintf(stderr, "OSMesaCreateContext failed!\n"); return EXIT_FAILURE; }
  void *buffer = malloc(texture_width * texture_height * 4 * sizeof(GLubyte));
  if (buffer == NULL) { fprintf(stderr, "malloc error.\n"); return EXIT_FAILURE; }
  if (!OSMesaMakeCurrent(ctx, buffer, GL_UNSIGNED_BYTE, texture_width, texture_height)) { printf("OSMesaMakeCurrent failed!\n"); return EXIT_FAILURE; }
  
  #endif
  
  // create a texture object
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  
  // create a renderbuffer object to store depth info
  glGenRenderbuffersEXT(1, &rboId);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rboId);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, texture_width, texture_height);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
  
  // create a framebuffer object
  glGenFramebuffersEXT(1, &fboId);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId);
  
  // attach the texture to FBO color attachment point
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, textureId, 0);
  
  // attach the renderbuffer to depth attachment point
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rboId);
  
  // check FBO status
  //GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
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
  
  glViewport(0, 0, texture_width, texture_height);
  glClearColor(1, 1, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //gluPerspective(60.0f, (float)(texture_width)/texture_height, 1.0f, 100.0f);
  //glOrtho(texture_width, -texture_width, texture_height, -texture_height, -100, 100);
  
  float diff_x = max_x - min_x;
  float diff_y = max_y - min_y;
  float diff_z = max_z - min_z;
  
  //glOrtho(min_x-diff_x*0.01, max_x+diff_x*0.01, min_y-diff_y*0.01, max_y+diff_y*0.01, min_z-diff_z*0.01, max_z+diff_z*0.01);
  glOrtho(min_x-diff_x*0.01, max_x+diff_x*0.01, min_y-diff_y*0.01, max_y+diff_y*0.01, 1, -1);
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
  
  png_set_IHDR(png_ptr, info_ptr, bitmap->width, bitmap->height, depth, 
      PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  
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

int main(int argc, char ** argv)
{
  int texture_width = 1200;
  
  char file_name[1000] = "";
  char sprite_file_name[1000] = "";
  unsigned int sprite_point_size = 8;
  float rotation = 0;
  int num_attributes = -1;
  int c;
  while ((c = getopt(argc, argv, "f:w:r:s:p:")) != -1)
  switch (c)
  {
    case 'f': strncpy(file_name, optarg, sizeof(file_name)); break;
    case 's': strncpy(sprite_file_name, optarg, sizeof(sprite_file_name)); break;
    case 'p': sprite_point_size = atoi(optarg); break;
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
  
  struct Block * block = read_block(stdin);
  
  char coord_names[3][20] = { "x", "y", "z" };
  char color_names[4][20] = { "red", "green", "blue", "alpha" };
  
  int block_coord_column_ids[3] = { -1, -1, -1 };
  int block_color_column_ids[4] = { -1, -1, -1, -1 };
  
  int i;
  for (i = 0 ; i < 3 ; i++)
    block_coord_column_ids[i] = find_column_id_by_name(block, coord_names[i]);
  for (i = 0 ; i < 4 ; i++)
    block_color_column_ids[i] = find_column_id_by_name(block, color_names[i]);
  
  int rowc = find_column_id_by_name(block, "row_id");
  int ptc = find_column_id_by_name(block, "part_type");
  
  int row_id;
  for (row_id = 0 ; row_id < block->num_rows ; row_id++)
  {
    for (i = 0 ; i < 3 ; i++)
    {
      if (block_coord_column_ids[i] == -1) continue;
      double coord = get_cell_as_double(block, row_id, block_coord_column_ids[i]);
      if (coord < b[i][0]) b[i][0] = coord;
      if (coord > b[i][1]) b[i][1] = coord;
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
  
  glColor4f(0, 0, 0, 1);
  GLuint tex_2d = 0;
  if (sprite_file_name[0] != 0)
  {
    tex_2d = SOIL_load_OGL_texture(sprite_file_name, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 
                SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT);
    if (tex_2d==0)
      fprintf(stderr, "SOIL load file '%s' error: '%s'\n", sprite_file_name, SOIL_last_result());
    else
    {
      //glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, tex_2d);
      //glEnable(GL_POINT_SPRITE);
      glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
      glPointSize(sprite_point_size);
      //glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColor4f(0, 0, 0, 0.1);
    }
  }
  
  int prev_shapefile_row_id = -1;
  for (row_id = 0 ; row_id < block->num_rows ; row_id++)
  {
    double coord[3] = { 0, 0, 0 };
    for (i = 0 ; i < 3 ; i++)
      if (block_coord_column_ids[i] != -1)
        coord[i] = get_cell_as_double(block, row_id, block_coord_column_ids[i]);
    
    double color[4] = { 0, 0, 0, 1 };
    for (i = 0 ; i < 4 ; i++)
      if (block_color_column_ids[i] != -1)
        color[i] = get_cell_as_double(block, row_id, block_color_column_ids[i]);
    
    int shapefile_row_id = *(int*)get_cell(block, row_id, rowc);
    int part_type = *(int*)get_cell(block, row_id, ptc);
    if (shapefile_row_id != prev_shapefile_row_id)
    {
      if (prev_shapefile_row_id != -1) glEnd();
      if (ptc == -1 || part_type == 1) glBegin(GL_POINTS);
      else if (part_type == 5) glBegin(GL_LINE_STRIP);
      else { fprintf(stderr, "part_type == %d ?\n", part_type); exit(0); }
    }
    
    glVertex3dv(coord);
    glColor4dv(color);
    
    prev_shapefile_row_id = shapefile_row_id;
  }
  glEnd();
  
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

