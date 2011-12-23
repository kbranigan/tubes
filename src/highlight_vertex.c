
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION highlight_vertex
#include "scheme.h"

void draw_marker(float * ov, float diffx, float diffy)
{
  struct Shape * marker = new_shape();
  marker->gl_type = GL_LINE_LOOP;
  float v[3] = { 0,0,0 };
  
  //fprintf(stderr, "%f vs %f\n", diffx, diffy);
  
  float diff = (diffx > diffy ? diffy : diffx) / 10.0;
  
  float a = 0;
  for (a = 0 ; a < 3.14159265*2.0 ; a+=0.1)
  {
    v[0] = ov[0] + cos(a) * diff;
    v[1] = ov[1] + sin(a) * diff;
    append_vertex(marker, v);
  }
  
  write_shape(stdout, marker);
  free_shape(marker);
}

void draw_line(float * pv, float * v)
{
  struct Shape * marker = new_shape();
  marker->gl_type = GL_LINES;
  append_vertex(marker, pv);
  append_vertex(marker, v);
  write_shape(stdout, marker);
  free_shape(marker);
}

int highlight_vertex(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int v_index = -1;
  int highlight_all = 0;
  int c;
  while ((c = getopt(argc, argv, "i:a")) != -1)
  switch (c)
  {
    case 'i': v_index = atoi(optarg); break;
    case 'a': highlight_all = 1; break;
    default:  abort();
  }
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    float minx = FLT_MAX, maxx = -FLT_MAX;
    float miny = FLT_MAX, maxy = -FLT_MAX;
    int i;
    for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      float * v = get_vertex(shape, 0, i);
      if (v[0] > maxx) maxx = v[0];
      if (v[0] < minx) minx = v[0];
      if (v[1] > maxy) maxy = v[1];
      if (v[1] < miny) miny = v[1];
    }
    
    //fprintf(stderr, "%f => %f, %f => %f\n", minx, maxx, miny, maxy);
    
    if (highlight_all == 1)
      for (i = 0 ; i < shape->num_vertexs ; i++)
        draw_marker(get_vertex(shape, 0, i), maxx-minx, maxy-miny);
    else if ((v_index >= 0 && v_index < shape->num_vertexs))
      draw_marker(get_vertex(shape, 0, v_index), maxx-minx, maxy-miny);
    else if (v_index == -1)
    {
      draw_marker(get_vertex(shape, 0, 0), maxx-minx, maxy-miny);
      draw_marker(get_vertex(shape, 0, shape->num_vertexs-1), maxx-minx, maxy-miny);
    }
    
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
