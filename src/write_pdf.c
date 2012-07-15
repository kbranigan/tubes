
/*

Requires libharu installed, from http://libharu.org/wiki/Downloads

*/

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hpdf.h"

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION write_pdf
#include "scheme.h"

int write_pdf(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * filename = argc > 1 ? argv[1] : "output.pdf";
  if (argc == 1)
  {
    fprintf(stderr, "no filename specified, using '%s'\n", filename);
  }
  
  float b[3][2] = {
    {FLT_MAX, -FLT_MAX},
    {FLT_MAX, -FLT_MAX},
    {FLT_MAX, -FLT_MAX}
  };
  
  HPDF_Doc pdf;
  
  pdf = HPDF_New(NULL, NULL); 
  if (!pdf)
  {
    fprintf(stderr, "ERROR: cannot create pdf object.\n");
    return 1;
  }
  
  HPDF_SetCompressionMode (pdf, HPDF_COMP_ALL);
  HPDF_SetPageMode (pdf, HPDF_PAGE_MODE_USE_OUTLINE);
  
  int num_shapes = 0;
  struct Shape ** shapes = NULL;
  
  HPDF_Page page;
  page = HPDF_AddPage (pdf);
  
  HPDF_Page_SetSize (page, HPDF_PAGE_SIZE_LETTER, HPDF_PAGE_PORTRAIT);
  
  long i = 0;
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    num_shapes++;
    shapes = (struct Shape **)realloc(shapes, sizeof(struct Shape*)*num_shapes);
    if (shapes == NULL) { fprintf(stderr, "realloc for shapes failed :(\n"); exit(0); }
    shapes[num_shapes-1] = shape;
    
    long j;
    for (j = 0 ; j < shape->num_vertexs ; j++)
    {
      float * v = get_vertex(shape, 0, j);
      if (v[0] < b[0][0]) b[0][0] = v[0]; if (v[0] > b[0][1]) b[0][1] = v[0];
      if (v[1] < b[1][0]) b[1][0] = v[1]; if (v[1] > b[1][1]) b[1][1] = v[1];
    }
  }
  
  float dx = b[0][1] - b[0][0];
  float dy = b[1][1] - b[1][0];
  float ratio = dy / dx;
  
  HPDF_Page_SetWidth(page, 2000);
  HPDF_Page_SetHeight(page, 2000.0 * ratio);
  
  for (i = 0 ; i < num_shapes ; i++)
  {
    shape = shapes[i];
    //if (shape->vertex_arrays[0].num_dimensions < 2) { fprintf(stderr, "\n"); continue; }
    
    long j;
    for (j = 0 ; j < shape->num_vertexs ; j++)
    {
      float * v = get_vertex(shape, 0, j);
      if (j == 0)
        HPDF_Page_MoveTo(page, (v[0] - b[0][0]) / dx * 2000.0, (v[1] - b[1][0]) / dy * (2000.0 * ratio));
      else
        HPDF_Page_LineTo(page, (float)(v[0] - b[0][0]) / dx * 2000.0, (float)(v[1] - b[1][0]) / dy * (2000.0 * ratio));
    }
    
    if (shape->gl_type == GL_LINE_LOOP)
    {
      float * v = get_vertex(shape, 0, 0);
      HPDF_Page_LineTo(page, (float)(v[0] - b[0][0]) / dx * 2000.0, (float)(v[1] - b[1][0]) / dy * (2000.0 * ratio));
    }
    
    HPDF_Page_Stroke(page);
    free_shape(shape);
  }
  free(shapes);
  
  HPDF_SaveToFile (pdf, filename);
  
  HPDF_Free(pdf);
}
