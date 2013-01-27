
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION tile
#include "scheme.h"

int tile(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int array_index = 0; // will operate only on a single array, default the first
  
  char file_name[1000] = "";
  int tile_dimension = -1;
  float distance = 0.0;
  int distance_as_percent = 0;
  
  int i;
  while ((i = getopt(argc, argv, "xyzd:t:f:p")) != -1)
  switch (i)
  {
    case 'p': distance_as_percent = 1; break;
    case 'd': distance = atof(optarg); break;
    case 'x': tile_dimension = 0; break;
    case 'y': tile_dimension = 1; break;
    case 'z': tile_dimension = 2; break;
    case 't': tile_dimension = atoi(optarg); break;
    case 'f': strncpy(file_name, optarg, sizeof(file_name)); break;
    default:  abort();
  }
  
  if (file_name[0] == 0 && argc == 2 && argv[1] != NULL)
    strncpy(file_name, argv[1], sizeof(file_name));
  
  FILE * fp = file_name == NULL ? pipe_in : fopen(file_name, "r");
  
  if (fp == NULL || file_name == NULL)
  {
    fprintf(pipe_err, "ERROR: Usage: %s -f [file_name]\n", argv[0]);
    return -1;
  }
  
  int num_shapes1 = 0;
  struct Shape ** shapes1 = read_all_shapes(fp, &num_shapes1);
  struct BBox * bbox1 = get_bbox_from_shapes(shapes1, num_shapes1);
  
  int num_shapes2 = 0;
  struct Shape ** shapes2 = read_all_shapes(pipe_in, &num_shapes2);
  struct BBox * bbox2 = get_bbox_from_shapes(shapes2, num_shapes2);
  
  // intelligent tiling if the dimension is not specified
  if (tile_dimension == -1)
  {
    tile_dimension = 0;
    if (bbox1->num_minmax > 1)
    {
      fprintf(stderr, "intelligent tiling enabled (use -x or -y to specify if specifics are desired)\n");
      if (bbox1->minmax[1].max - bbox1->minmax[1].min < bbox1->minmax[0].max - bbox1->minmax[0].min)
        tile_dimension = 1;
    }
  }
  
  for (i = 0 ; i < num_shapes1 ; i++)
    write_shape(pipe_out, shapes1[i]);
  
  for (i = 0 ; i < num_shapes2 ; i++)
  {
    struct Shape * shape = shapes2[i];
    struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
    float * v;
    long j, k;
    for (j = 0 ; j < shape->num_vertexs ; j++)
    {
      v = get_vertex(shape, 0, j);
      for (k = 0 ; k < va->num_dimensions && k < bbox1->num_minmax && k < bbox2->num_minmax ; k++)
        if (distance_as_percent)
          v[k] = v[k] - bbox2->minmax[k].min + ((k == tile_dimension) ? (bbox1->minmax[k].max + distance * (bbox1->minmax[k].max - bbox1->minmax[k].min)) : bbox1->minmax[k].min);
        else
          v[k] = v[k] - bbox2->minmax[k].min + ((k == tile_dimension) ? (bbox1->minmax[k].max + distance) : bbox1->minmax[k].min);
    }
    write_shape(pipe_out, shapes2[i]);
  }
  
  free_bbox(bbox1);
  free_bbox(bbox2);
  
  free_all_shapes(shapes1, num_shapes1);
  free_all_shapes(shapes2, num_shapes2);
}
