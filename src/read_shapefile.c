
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
  char filename[300] = "";
  long row_id = -1;
  long part_id = -1;
  int num_attributes = -1;
  int c;
  while ((c = getopt(argc, argv, "f:r:p:a:")) != -1)
  switch (c)
  {
    case 'f':
      strncpy(filename, optarg, 300);
      break;
    case 'r':
      row_id = atol(optarg);
      break;
    case 'p':
      part_id = atol(optarg);
      break;
    case 'a':
      num_attributes = atoi(optarg);
      break;
    default:
      abort();
  }
  
  if (strlen(filename) == 0)
  {
    fprintf(stderr, "%s: must specify a shapefile using -f [filename.dbf]\n", argv[0]);
    exit(1);
  }
  
  FILE * fp = fopen(filename, "r");
  if (!fp)
  {
    fprintf(stderr, "%s: error reading shapefile: %s\n", argv[0], filename);
    exit(1);
  }
  
  DBFHandle d = DBFOpen(filename, "rb");
  if (d == NULL) { fprintf(stderr, "DBFOpen error (%s.dbf)\n", filename); exit(1); }
	
	SHPHandle h = SHPOpen(filename, "rb");
  if (h == NULL) { fprintf(stderr, "SHPOpen error (%s.dbf)\n", filename); exit(1); }
	
  long nRecordCount = DBFGetRecordCount(d);
  long nFieldCount = DBFGetFieldCount(d);
  
  long t=0;
  long i;
  
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
    SHPObject	*psShape = SHPReadObject(h, i);
    if (psShape->nSHPType == SHPT_POINT)
    {
      struct Shape * shape = new_shape();
      shape->unique_set_id = t++;
      //shape->has_attribute_names = 0;
      
      int j;
      if (num_attributes)
      for (j = 0 ; j < nFieldCount && (j < num_attributes || num_attributes == -1) ; j++)
      {
        char name[20];
        char value[200];
        int value_length;
        DBFFieldType field_type = DBFGetFieldInfo(d, j, name, &value_length, NULL);
        switch (field_type) {
          case FTString:
            snprintf(value, 200, "%s", (char*)DBFReadStringAttribute(d, i, j));
            break;
          case FTInteger:
            snprintf(value, 200, "%d", DBFReadIntegerAttribute(d, i, j));
            break;
          case FTDouble:
            snprintf(value, 200, "%f", DBFReadDoubleAttribute(d, i, j));
            break;
        }
        set_attribute(shape, name, value);
      }
      
      for (j = 0 ; j < psShape->nVertices ; j++)
      {
        float v[2] = { psShape->padfX[j], psShape->padfY[j] };
        append_vertex(shape, v);
      }
      write_shape(pipe_out, shape);
      free_shape(shape);
    }
    else if (psShape->nSHPType == SHPT_ARC || psShape->nSHPType == SHPT_POLYGON)
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
        
        struct Shape * shape = new_shape();
        shape->unique_set_id = t++;
        
        if (psShape->nSHPType == SHPT_ARC)           shape->gl_type = GL_LINE_STRIP;
        else if (psShape->nSHPType == SHPT_POLYGON)  shape->gl_type = GL_LINE_LOOP;
        
        for (j = 0 ; j < nFieldCount ; j++)
        {
          char name[20];
          char value[200];
          int value_length;
          DBFFieldType field_type = DBFGetFieldInfo(d, j, name, &value_length, NULL);
          switch (field_type) {
            case FTString:
              snprintf(value, 200, "%s", (char*)DBFReadStringAttribute(d, i, j));
              break;
            case FTInteger:
              snprintf(value, 200, "%d", DBFReadIntegerAttribute(d, i, j));
              break;
            case FTDouble:
              snprintf(value, 200, "%f", DBFReadDoubleAttribute(d, i, j));
              break;
          }
          set_attribute(shape, name, value);
        }
        
        for (j = start ; j < end ; j++)
        {
          float v[2] = { psShape->padfX[j], psShape->padfY[j] };
          append_vertex(shape, v);
        }
        write_shape(pipe_out, shape);
        free_shape(shape);
      }
    }
    else
    {
      fprintf(stderr, "doesn't do nSHPType:%d yet.\n", psShape->nSHPType);
      return 0;
    }
    SHPDestroyObject(psShape);
  }
  DBFClose(d);
  SHPClose(h);
}
