
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
  char file_name[1000] = "";
  char layer_name[200] = "";
  int c;
  while ((c = getopt(argc, argv, "f:l:")) != -1)
  switch (c)
  {
    case 'f': strncpy(file_name, optarg, sizeof(file_name)); break;
    case 'l': strncpy(layer_name, optarg, sizeof(layer_name)); break;
    default:  abort();
  }
  
  if (file_name[0] == 0 && argc == 2 && argv[1] != NULL)
    strncpy(file_name, argv[1], sizeof(file_name));
  
  FILE * fp = file_name == NULL ? pipe_in : fopen(file_name, "r");
  
  if (fp == NULL || file_name == NULL)
  {
    fprintf(pipe_err, "ERROR: Usage: %s -f [file_name.dwg]\n", argv[0]);
    return -1;
  }
  
  int counts[10000];
  memset(counts, 0, sizeof(counts));
  
  int i;
  int write_count = 0;
  
  Dwg_Data dwg;
  
  float v[3] = { 0, 0, 0 };
  dwg.num_entities = 0;
  int success = dwg_read_file(file_name, &dwg);
  
  for (i = 0 ; i < dwg.num_entities ; i++)
  {
    if (dwg.object[i].type < 10000)
      counts[dwg.object[i].type]++;
    
    switch (dwg.object[i].type)
    {
      case DWG_TYPE_LINE:
      {
        Dwg_Entity_LINE * line = dwg.object[i].tio.entity->tio.LINE;
        
        if (layer_name[0] != 0)
        {
          Dwg_Object_LAYER * layer = dwg.object[i].tio.entity->layer->obj->tio.object->tio.LAYER;
          if (strcmp(layer->entry_name, layer_name) != 0) break;
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
        
        write_count++;
        write_shape(pipe_out, shape);
        free_shape(shape);
        
        //exit(1);
        break;
      }
      case DWG_TYPE_LWPLINE:
      {
        Dwg_Entity_LWPLINE * lwpLine = dwg.object[i].tio.entity->tio.LWPLINE;
        
        if (layer_name[0] != 0)
        {
          Dwg_Object_LAYER * layer = dwg.object[i].tio.entity->layer->obj->tio.object->tio.LAYER;
          if (strcmp(layer->entry_name, layer_name) != 0) break;
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
        
        write_count++;
        write_shape(pipe_out, shape);
        free_shape(shape);
        
        break;
      }
      case DWG_TYPE_POINT:
      {
        Dwg_Entity_POINT * point = dwg.object[i].tio.entity->tio.POINT;
        
        if (layer_name[0] != 0)
        {
          Dwg_Object_LAYER * layer = dwg.object[i].tio.entity->layer->obj->tio.object->tio.LAYER;
          if (strcmp(layer->entry_name, layer_name) != 0) break;
        }
        
        struct Shape * shape = new_shape();
        
        shape->gl_type = GL_POINTS;
        
        v[0] = point->x;
        v[1] = point->y;
        append_vertex(shape, v);
        
        write_count++;
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
        
        if (layer_name[0] != 0)
        {
          Dwg_Object_LAYER * layer = dwg.object[i].tio.entity->layer->obj->tio.object->tio.LAYER;
          if (strcmp(layer->entry_name, layer_name) != 0) break;
        }
        
        struct Shape * shape = new_shape();
        
        shape->gl_type = GL_POINTS;
        
        v[0] = insert->ins_pt.x;
        v[1] = insert->ins_pt.y;
        append_vertex(shape, v);
        
        write_count++;
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
  
  if (write_count == 0 && layer_name[0] != 0)
  {
    fprintf(stderr, "layer '%s' selection provided but no shapes were found.\n", layer_name);
    
    fprintf(stderr, "The following layers were found:\n");
    for (i = 0 ; i < dwg.num_entities ; i++)
      if (dwg.object[i].type == DWG_TYPE_LAYER)
        fprintf(stderr, "%s\n", dwg.object[i].tio.object->tio.LAYER->entry_name);
  }
  
  if (0)
  for (i = 0 ; i < 10000 ; i++)
  {
    if (counts[i] != 0)
      fprintf(stderr, "%x: %d\n", i, counts[i]);
  }
}
