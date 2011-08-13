
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

GLuint textureId;
GLuint rboId;
GLuint fboId;
int TEXTURE_WIDTH = 1600;
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
  
  OSMesaContext ctx = OSMesaCreateContextExt(OSMESA_RGBA, 32, 8, 16, NULL);
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
  
  glViewport(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT);
  glClearColor(1, 1, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  //gluPerspective(60.0f, (float)(TEXTURE_WIDTH)/TEXTURE_HEIGHT, 1.0f, 100.0f);
  //glOrtho(TEXTURE_WIDTH, -TEXTURE_WIDTH, TEXTURE_HEIGHT, -TEXTURE_HEIGHT, -100, 100);
  
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
