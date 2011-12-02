
/*

This requires libredwg which can be downloaded from: http://www.gnu.org/s/libredwg/
I have included the lib and the header file in /ext but if that doesn't function on your platform you'll need to
compile it yourself.  I built the included file from: git clone git://git.sv.gnu.org/libredwg.git

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "ext/dwg.h"

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_dwg
#include "scheme.h"

int read_dwg(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * filename = NULL;
  char layername[200] = "";
  int c;
  while ((c = getopt(argc, argv, "f:l:")) != -1)
  switch (c)
  {
    case 'f':
      filename = malloc(strlen(optarg)+1);
      strcpy(filename, optarg);
      break;
    case 'l':
      strncpy(layername, optarg, sizeof(layername));
      break;
  }
  
  if (filename == NULL && argc == 2 && argv[1] != NULL) filename = argv[1];
  
  FILE * fp = filename == NULL ? pipe_in : fopen(filename, "r");
  
  if (fp == NULL || filename == NULL)
  {
    fprintf(pipe_err, "ERROR: Usage: %s -f [filename.dwg]\n", argv[0]);
    return -1;
  }
  
  int counts[10000];
  memset(counts, 0, sizeof(counts));
  
  int i;
  
  Dwg_Data dwg;
  
  float v[3] = { 0, 0, 0 };
  dwg.num_entities = 0;
  int success = dwg_read_file(filename, &dwg);
  
  for (i = 0 ; i < dwg.num_entities ; i++)
  {
    if (dwg.object[i].type < 10000)
      counts[dwg.object[i].type]++;
    
    switch (dwg.object[i].type)
    {
      case DWG_TYPE_LINE:
      {
        Dwg_Entity_LINE * line = dwg.object[i].tio.entity->tio.LINE;
        
        if (layername[0] != 0)
        {
          Dwg_Object_LAYER * layer = dwg.object[i].tio.entity->layer->obj->tio.object->tio.LAYER;
          if (strcmp(layer->entry_name, layername) != 0) break;
        }
        
        struct Shape * shape = new_shape();
        
        shape->gl_type = GL_LINES;
        
        v[0] = line->start.x;
        v[1] = line->start.y;
        append_vertex(shape, v);
        //fprintf(stderr, "%f %f\n", v[0], v[1]);
        
        v[0] = line->end.x;
        v[1] = line->end.y;
        append_vertex(shape, v);
        //fprintf(stderr, "%f %f\n", v[0], v[1]);
        
        write_shape(pipe_out, shape);
        free_shape(shape);
        
        //exit(1);
        break;
      }
      case DWG_TYPE_LWPLINE:
      {
        Dwg_Entity_LWPLINE * lwpLine = dwg.object[i].tio.entity->tio.LWPLINE;
        
        if (layername[0] != 0)
        {
          Dwg_Object_LAYER * layer = dwg.object[i].tio.entity->layer->obj->tio.object->tio.LAYER;
          if (strcmp(layer->entry_name, layername) != 0) break;
        }
        
        struct Shape * shape = new_shape();
        
        shape->gl_type = GL_LINE_STRIP;
        if (lwpLine->flags & 512) shape->gl_type = GL_LINE_LOOP;
        
        int a = 0;
        for (a = 0 ; a < lwpLine->num_points ; a++)
        {
          v[0] = lwpLine->points[a].x;
          v[1] = lwpLine->points[a].y;
          
          if (lwpLine->points[a].x < 600) continue;
          //fprintf(stderr, "%f %f\n", v[0], v[1]);
          //fprintf(stderr, "%f %f\n", v[0], v[1]);
          append_vertex(shape, v);
        }
        
        //if (lwpLine->flags < 10000) counts[lwpLine->flags]++;
        
        //fprintf(stderr, "%d\n", lwpLine->flags);
        
        write_shape(pipe_out, shape);
        free_shape(shape);
        
        break;
      }
      case DWG_TYPE_POINT:
      {
        Dwg_Entity_POINT * point = dwg.object[i].tio.entity->tio.POINT;
        
        if (layername[0] != 0)
        {
          Dwg_Object_LAYER * layer = dwg.object[i].tio.entity->layer->obj->tio.object->tio.LAYER;
          if (strcmp(layer->entry_name, layername) != 0) break;
        }
        
        struct Shape * shape = new_shape();
        
        shape->gl_type = GL_POINTS;
        
        v[0] = point->x;
        v[1] = point->y;
        append_vertex(shape, v);
        
        write_shape(pipe_out, shape);
        free_shape(shape);
        
        break;
      }
      case DWG_TYPE_CIRCLE:
        break;
      case DWG_TYPE_TEXT:
        break;
      case DWG_TYPE_ELLIPSE:
      case DWG_TYPE_ARC:
      case DWG_TYPE_ATTRIB:
      case DWG_TYPE_POLYLINE_3D:
      {
        break;
      }
      case DWG_TYPE_INSERT:
      {
        // stuff, like light poles
        
        Dwg_Entity_INSERT * insert = dwg.object[i].tio.entity->tio.INSERT;
        
        if (layername[0] != 0)
        {
          Dwg_Object_LAYER * layer = dwg.object[i].tio.entity->layer->obj->tio.object->tio.LAYER;
          if (strcmp(layer->entry_name, layername) != 0) break;
        }
        
        struct Shape * shape = new_shape();
        
        shape->gl_type = GL_POINTS;
        
        v[0] = insert->ins_pt.x;
        v[1] = insert->ins_pt.y;
        append_vertex(shape, v);
        
        write_shape(pipe_out, shape);
        free_shape(shape);
        
        break;
      }
      case DWG_TYPE_BLOCK_CONTROL:
      case DWG_TYPE_BLOCK_HEADER:
      case DWG_TYPE_LAYER_CONTROL:
      case DWG_TYPE_SHAPEFILE_CONTROL:
      case DWG_TYPE_LTYPE_CONTROL:
      case DWG_TYPE_VIEW_CONTROL:
      case DWG_TYPE_UCS_CONTROL:
      case DWG_TYPE_VPORT_CONTROL:
      case DWG_TYPE_APPID_CONTROL:
      case DWG_TYPE_DIMSTYLE_CONTROL:
      case DWG_TYPE_VP_ENT_HDR_CONTROL:
      case DWG_TYPE_DIMSTYLE:
      case DWG_TYPE_VPORT:
      case DWG_TYPE_BLOCK:
      case DWG_TYPE_ENDBLK:
      case DWG_TYPE_APPID:
      case DWG_TYPE_MLINESTYLE:
      case DWG_TYPE_LTYPE:
        break;
      case DWG_TYPE_LAYER:
      {
        //Dwg_Object_LAYER * layer = dwg.object[i].tio.object->tio.LAYER;
        //fprintf(stderr, "%s\n", layer->entry_name);
        break;
      }
      case DWG_TYPE_SHAPEFILE:
      {
        // not really sure
        /*Dwg_Object_SHAPEFILE * shapefile = dwg.object[i].tio.object->tio.SHAPEFILE;
        fprintf(stderr, "%s\n", shapefile->entry_name);
        fprintf(stderr, "width_factor=%f\n", shapefile->width_factor);
        fprintf(stderr, "oblique_ang=%f\n", shapefile->oblique_ang);
        fprintf(stderr, "last_height=%f\n", shapefile->last_height);
        
        Dwg_Object_SHAPEFILE_CONTROL * shapefilec = shapefile->shapefile_control;
        
        counts[dwg.object[i].type]++;*/
        break;
      }
      case DWG_TYPE_DICTIONARY:
      {
        //Dwg_Object_DICTIONARY * dict = dwg.object[i].tio.object->tio.DICTIONARY;
        //fprintf(stderr, "%d\n", dict->numitems);
        //counts[dwg.object[i].type]++;
        break;
      }
//        break;
      default:
      {
        //if (dwg.object[i].type < 1000)
        //  counts[dwg.object[i].type]++;
        
        //fprintf(stderr, "%d: %x\n", i, dwg.object[i].type);
        //counts[0]++;
        break;
      }
    }
  }
  
  if (0)
  for (i = 0 ; i < 10000 ; i++)
  {
    if (counts[i] != 0)
      fprintf(stderr, "%x: %d\n", i, counts[i]);
  }
}
