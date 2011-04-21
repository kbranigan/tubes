
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gpc/gpc.h"

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION clip
#include "scheme.h"

int clip(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  gpc_op gpc_operation = GPC_INT; // default is intersection
  
  char * clipping_filename = NULL;
  int c;
  while ((c = getopt(argc, argv, "f:o:")) != -1)
  switch (c)
  {
    case 'f':
      clipping_filename = malloc(strlen(optarg)+1);
      strcpy(clipping_filename, optarg);
      break;
    case 'o':
      if (strcmp(optarg, "difference")==0)        gpc_operation = GPC_DIFF;
      else if (strcmp(optarg, "intersection")==0) gpc_operation = GPC_INT;
      else if (strcmp(optarg, "exclusive-or")==0) gpc_operation = GPC_XOR;
      else if (strcmp(optarg, "union")==0)        gpc_operation = GPC_UNION;
      else {
        fprintf(pipe_err, "-o [difference|intersection|exclusive-or|union] ('%s' is invalid)\n", optarg);
        exit(1);
      }
      break;
    default:
      abort();
  }
  
  if (clipping_filename == NULL) { fprintf(pipe_err, "-c [clipping_filename] is required\n"); exit(1); }
  
  FILE * clip_fp = fopen(clipping_filename, "r");
  if (clip_fp == NULL) { fprintf(pipe_err, "-c [clipping_filename] is required ('%s' doesn't exist)\n", clipping_filename); exit(1); }
  
  gpc_polygon * clip_polygon = malloc(sizeof(gpc_polygon));
  clip_polygon->num_contours = 0;
  clip_polygon->hole = NULL;
  clip_polygon->contour = NULL;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(clip_fp)))
  {
    if (shape->gl_type != GL_LINE_LOOP) { fprintf(pipe_err, "Error: Trying to clip with a shape which is not a line loop\n"); exit(1); }
    gpc_vertex_list * contour = malloc(sizeof(gpc_vertex_list));
    contour->num_vertices = shape->num_vertexs;
    contour->vertex = malloc(sizeof(gpc_vertex)*contour->num_vertices);
    
    int i;
    for (i = 0 ; i < contour->num_vertices ; i++)
    {
      float * v = get_vertex(shape, 0, i);
      contour->vertex[i].x = v[0];
      contour->vertex[i].y = v[1];
    }
    
    gpc_add_contour(clip_polygon, contour, 0);
    /* manipulate data here if you like */
    //write_shape(pipe_out, shape);
    free_shape(shape);
  }
  fclose(clip_fp);
  
  while ((shape = read_shape(pipe_in)))
  {
    gpc_polygon * org = malloc(sizeof(gpc_polygon));
    org->num_contours = 0;
    org->hole = NULL;
    org->contour = NULL;
    
    gpc_vertex_list * contour = malloc(sizeof(gpc_vertex_list));
    contour->num_vertices = shape->num_vertexs;
    contour->vertex = malloc(sizeof(gpc_vertex)*contour->num_vertices);
    
    int i;
    for (i = 0 ; i < contour->num_vertices ; i++)
    {
      float * v = get_vertex(shape, 0, i);
      contour->vertex[i].x = v[0];
      contour->vertex[i].y = v[1];
    }
    gpc_add_contour(org, contour, 0);
    
    gpc_polygon * result_polygon = malloc(sizeof(gpc_polygon));
    result_polygon->num_contours = 0;
    result_polygon->contour = NULL;
    result_polygon->hole = NULL;
    
    gpc_polygon_clip(gpc_operation, org, clip_polygon, result_polygon);
    
    int j;
    for (j = 0 ; j < result_polygon->num_contours ; j++)
    {
      struct Shape * res = new_shape();
      res->gl_type = GL_LINE_LOOP;
      for (i = 0 ; i < shape->num_attributes ; i++)
      {
        set_attribute(res, shape->attributes[i].name, shape->attributes[i].value);
      }
      
      for (i = 0 ; i < result_polygon->contour[j].num_vertices ; i++)
      {
        float v[3] = { result_polygon->contour[j].vertex[i].x, result_polygon->contour[j].vertex[i].y, 0.0 };
        append_vertex(res, v);
      }
      write_shape(pipe_out, res);
      free_shape(res);
    }
    
    gpc_free_polygon(org);
    gpc_free_polygon(result_polygon);
    free_shape(shape);
  }
  
  gpc_free_polygon(clip_polygon);
}
