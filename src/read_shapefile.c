
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ext/shapefil.h"

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_shapefile
#include "scheme.h"

int read_shapefile(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char file_name[1000] = "";
  long row_id = -1;
  long part_id = -1;
  int num_attributes = -1;
  int c;
  while ((c = getopt(argc, argv, "f:r:p:a:")) != -1)
  switch (c)
  {
    case 'f': strncpy(file_name, optarg, 300); break;
    case 'r': row_id = atol(optarg); break;
    case 'p': part_id = atol(optarg); break;
    case 'a': num_attributes = atoi(optarg); break;
    default: abort();
  }
  
  if (file_name[0] == 0 && argc == 2 && argv[1] != NULL)
    strncpy(file_name, argv[1], sizeof(file_name));
  
  FILE * fp = file_name[0] == 0 ? pipe_in : fopen(file_name, "r");
  
  if (fp == NULL || file_name[0] == 0)
  {
    fprintf(pipe_err, "ERROR: Usage: %s -f [file_name]\n", argv[0]);
    return -1;
  }
  
  DBFHandle dbf = DBFOpen(file_name, "rb");
  //if (dbf == NULL) { fprintf(stderr, "DBFOpen error (%s.dbf)\n", file_name); exit(1); }
  
  SHPHandle shp = SHPOpen(file_name, "rb");
  //if (shp == NULL) { fprintf(stderr, "SHPOpen error (%s.dbf)\n", file_name); exit(1); }
  
  if (dbf == NULL && shp == NULL)
  {
    fprintf(stderr, "%s: Error, neither dbf or shp file found\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  long nRecordCount = DBFGetRecordCount(dbf);
  long nFieldCount = DBFGetFieldCount(dbf);
  
  long t = 0;
  long i, j;
  
  /*int num_strings = nFieldCount;
  char ** strings = malloc(sizeof(char*)*num_strings);
  for (i = 0 ; i < nFieldCount ; i++)
  {
    strings[i] = malloc(20);
    int value_length;
    DBFFieldType field_type = DBFGetFieldInfo(d, i, strings[i], &value_length, NULL);
  }
  write_string_table(pipe_out, num_strings, strings);
  for (i = 0 ; i < nFieldCount ; i++)
    free(strings[i]);
  free(strings);*/
  
  for (i = 0 ; i < nRecordCount ; i++)
  {
    if (row_id != -1 && row_id != i) continue;
    
    struct Shape * shape = new_shape();
    
    if (num_attributes)
    for (j = 0 ; j < nFieldCount && (j < num_attributes || num_attributes == -1) ; j++)
    {
      char name[20];
      char value[200];
      int value_length;
      DBFFieldType field_type = DBFGetFieldInfo(dbf, j, name, &value_length, NULL);
      switch (field_type) {
        case FTString:
          snprintf(value, 200, "%s", (char*)DBFReadStringAttribute(dbf, i, j));
          break;
        case FTInteger:
          snprintf(value, 200, "%d", DBFReadIntegerAttribute(dbf, i, j));
          break;
        case FTDouble:
          snprintf(value, 200, "%f", DBFReadDoubleAttribute(dbf, i, j));
          break;
      }
      set_attribute(shape, name, value);
    }
    
    SHPObject * psShape = NULL;
    if (shp) psShape = SHPReadObject(shp, i);
    
    if (shp && psShape->nSHPType == SHPT_POINT)
    {
      //struct Shape * shape = new_shape();
      shape->unique_set_id = t++;
      //shape->has_attribute_names = 0;
      
      for (j = 0 ; j < psShape->nVertices ; j++)
      {
        float v[2] = { psShape->padfX[j], psShape->padfY[j] };
        append_vertex(shape, v);
      }
      write_shape(pipe_out, shape);
      free_shape(shape);
    }
    else if (shp && (psShape->nSHPType == SHPT_ARC || psShape->nSHPType == SHPT_POLYGON))
    {
      //fprintf(stderr, "shape %d has %d parts and %d vertices\n", psShape->nShapeId, psShape->nParts, psShape->nVertices);
      
      if (psShape->nParts == 0) { fprintf(stderr, "polygon %d has 0 parts (thats bad)\n", psShape->nShapeId); return 0; }
      
      int j, iPart;
      for (iPart = 0 ; iPart < psShape->nParts ; iPart++)
      {
        if (part_id != -1 && part_id != iPart) continue;
        int start = psShape->panPartStart[iPart];
        int end = psShape->nVertices;
        
        if (iPart != psShape->nParts - 1)
        {
          end = psShape->panPartStart[iPart+1];
        }
        
        struct Shape * temp_shape = new_shape();
        temp_shape->unique_set_id = t++;
        
        if (psShape->nSHPType == SHPT_ARC)          temp_shape->gl_type = GL_LINE_STRIP;
        else if (psShape->nSHPType == SHPT_POLYGON) temp_shape->gl_type = GL_LINE_LOOP;
        
        /*for (j = 0 ; j < nFieldCount ; j++)
        {
          char name[20];
          char value[200];
          int value_length;
          DBFFieldType field_type = DBFGetFieldInfo(dbf, j, name, &value_length, NULL);
          switch (field_type) {
            case FTString:
              snprintf(value, 200, "%s", (char*)DBFReadStringAttribute(dbf, i, j));
              break;
            case FTInteger:
              snprintf(value, 200, "%d", DBFReadIntegerAttribute(dbf, i, j));
              break;
            case FTDouble:
              snprintf(value, 200, "%f", DBFReadDoubleAttribute(dbf, i, j));
              break;
          }
          set_attribute(shape, name, value);
        }*/
        
        for (j = 0 ; j < shape->num_attributes ; j++)
          set_attribute(temp_shape, shape->attributes[j].name, shape->attributes[j].value);
        
        for (j = start ; j < end ; j++)
        {
          float v[2] = { psShape->padfX[j], psShape->padfY[j] };
          append_vertex(temp_shape, v);
        }
        write_shape(pipe_out, temp_shape);
        free_shape(temp_shape);
      }
      free_shape(shape);
    }
    else if (shp)
    {
      fprintf(stderr, "doesn't do nSHPType:%d yet.\n", psShape->nSHPType);
      return 0;
    }
    else if (dbf)
    {
      free(shape->vertex_arrays);
      shape->num_vertex_arrays = 0;
      write_shape(pipe_out, shape);
      free_shape(shape);
    }
    if (shp) SHPDestroyObject(psShape);
  }
  if (dbf) DBFClose(dbf);
  if (shp) SHPClose(shp);
}
