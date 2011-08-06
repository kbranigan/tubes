
/*

This requires sndfile which can be downloaded from: http://libdwg.sourceforge.net/en/
I have included the lib and the header file in /ext but if that doesn't function on your platform you'll need to
compile it yourself.  I built the included file from: http://downloads.sourceforge.net/libdwg/libdwg-0.3.tar.bz2

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <dvg.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_dwg
#include "scheme.h"

int read_dwg(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * filename = NULL;
  int c;
  while ((c = getopt(argc, argv, "f:")) != -1)
  switch (c)
  {
    case 'f':
      filename = malloc(strlen(optarg)+1);
      strcpy(filename, optarg);
      break;
  }
  
  FILE * fp = filename == NULL ? pipe_in : fopen(filename, "r");
  
  if (fp == NULL)
  {
    fprintf(pipe_err, "ERROR: Usage: %s -f [filename.dwg]\n", argv[0]);
    return -1;
  }
  
  int i,l=0,e=0,t=0,s=0,r=0,p=0,a=0,d=0;
  
  Dvg_Strukturo dwg;
  
  float v[3] = { 0, 0, 0 };
  dwg.objekto_kiom = 0;
  int success = dvg_legi_dosiero (filename, &dwg);
  for (i = 0; i < dwg.objekto_kiom; i++)
  {
    switch (dvg_objekto_tipo (&dwg, i))
    {
      case DVG_OT_LINE:
      {
        /*v[0] = dvg_linio_x0 (&dwg, i);
        v[1] = dvg_linio_y0 (&dwg, i);
        append_vertex(shape, v);
        
        v[0] = dvg_linio_x1 (&dwg, i);
        v[1] = dvg_linio_y1 (&dwg, i);
        append_vertex(shape, v);*/
        
        l++;
        break;
      }
      case DVG_OT_CIRCLE:
        e++;
        break;
      case DVG_OT_TEXT:
        t++;
        break;
      case DVG_OT_ELLIPSE:
      case DVG_OT_ARC:
      case DVG_OT_POINT:
      case DVG_OT_ATTRIB:
      case DVG_OT_LWPOLYLINE:
        if (dvg_liniaro_rondigo_kiom(&dwg, i) == 0)
        {
          struct Shape * shape = new_shape();
          shape->gl_type = (dvg_liniaro_cxu_fermita(&dwg, i) != 0) ? GL_LINE_LOOP : GL_LINE_STRIP;
          
          struct VertexArray *va = get_or_add_array(shape, GL_VERTEX_ARRAY);
          va->num_dimensions = 2;
          
          v[0] = dvg_liniaro_xi(&dwg, i, 0);
          v[1] = dvg_liniaro_yi(&dwg, i, 0);
          append_vertex(shape, v);
          
          int j;
          for (j = 1 ; j < dvg_liniaro_vertico_kiom(&dwg, i) ; j++)
          {
            v[0] = dvg_liniaro_xi(&dwg, i, j);
            v[1] = dvg_liniaro_yi(&dwg, i, j);
            append_vertex(shape, v);
          }
          
          write_shape(pipe_out, shape);
          free_shape(shape);
        }
        else if (dvg_liniaro_rondigo_kiom (&dwg, i) == dvg_liniaro_vertico_kiom (&dwg, i))
          r++;

        break;
      default:
        d++;
        break;
    }
  }
  
  fprintf(stderr, "lines = %d\ncircles = %d\ntext = %d\n", l, e, t);
  fprintf(stderr, "%d %d %d %d %d\n", s, r, p, a, d);
  
  fprintf(stderr, "other = %d\n", d);
  
  // 
  // struct Shape * shape = new_shape();
  // shape->gl_type = GL_LINE_LOOP;
  // 
  // struct VertexArray *va = get_or_add_array(shape, GL_VERTEX_ARRAY);
  // va->num_dimensions = num_dimensions;
  // 
  // float v0[3] = { x-width/2.0, y-height/2.0, 0.0 };
  // append_vertex(shape, v0);
  // float v1[3] = { x-width/2.0, y+height/2.0, 0.0 };
  // append_vertex(shape, v1);
  // float v2[3] = { x+width/2.0, y+height/2.0, 0.0 };
  // append_vertex(shape, v2);
  // float v3[3] = { x+width/2.0, y-height/2.0, 0.0 };
  // append_vertex(shape, v3);
  // 
  // write_shape(pipe_out, shape);
  // free_shape(shape);
}
