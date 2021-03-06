
#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/osmesa.h>
#endif

#include "ext/SOIL.h"

GLuint textureId;
GLuint rboId;
GLuint fboId;
//int texture_width = 1600;
//int texture_height = 0;

int setup_offscreen_render(float min_x, float max_x, float min_y, float max_y, float min_z, float max_z, int texture_width, int zone_section)
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
  //glClearColor(1, 1, 1, 1);
  glClearColor(0.2, 0.2, 0.2, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //gluPerspective(60.0f, (float)(texture_width)/texture_height, 1.0f, 100.0f);
  //glOrtho(texture_width, -texture_width, texture_height, -texture_height, -100, 100);
  
  float diff_x = max_x - min_x;
  float diff_y = max_y - min_y;
  float diff_z = max_z - min_z;
  
  //glOrtho(min_x-diff_x*0.01, max_x+diff_x*0.01, min_y-diff_y*0.01, max_y+diff_y*0.01, min_z-diff_z*0.01, max_z+diff_z*0.01);
  
       if (zone_section == 1) glOrtho(min_x+diff_x*0.000, min_x+diff_x*0.333, min_y+diff_y*0.000, max_y-diff_y*0.666, 1, -1); // top left
  else if (zone_section == 2) glOrtho(min_x+diff_x*0.333, min_x+diff_x*0.666, min_y+diff_y*0.000, max_y-diff_y*0.666, 1, -1); // top mid
  else if (zone_section == 3) glOrtho(min_x+diff_x*0.666, max_x+diff_x*0.000, min_y+diff_y*0.000, max_y-diff_y*0.666, 1, -1); // top right
  
  else if (zone_section == 4) glOrtho(min_x+diff_x*0.000, min_x+diff_x*0.333, min_y+diff_y*0.333, min_y+diff_y*0.666, 1, -1); // mid left
  else if (zone_section == 5) glOrtho(min_x+diff_x*0.333, min_x+diff_x*0.666, min_y+diff_y*0.333, min_y+diff_y*0.666, 1, -1); // mid mid
  else if (zone_section == 6) glOrtho(min_x+diff_x*0.666, max_x+diff_x*0.000, min_y+diff_y*0.333, min_y+diff_y*0.666, 1, -1); // mid right
  
  else if (zone_section == 7) glOrtho(min_x+diff_x*0.000, min_x+diff_x*0.333, min_y+diff_y*0.666, max_y-diff_y*0.000, 1, -1); // top left
  else if (zone_section == 8) glOrtho(min_x+diff_x*0.333, min_x+diff_x*0.666, min_y+diff_y*0.666, max_y-diff_y*0.000, 1, -1); // top mid
  else if (zone_section == 9) glOrtho(min_x+diff_x*0.666, max_x+diff_x*0.000, min_y+diff_y*0.666, max_y-diff_y*0.000, 1, -1); // top right
  
  else if (zone_section == 10) glOrtho(min_x+diff_x*0.4, min_x+diff_x*0.45, min_y+diff_y*0.3, min_y+diff_y*0.35, 1, -1); // mid mid
  
  else glOrtho(min_x-diff_x*0.01, max_x+diff_x*0.01, min_y-diff_y*0.01, max_y+diff_y*0.01, 1, -1); // no zoom
  
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
