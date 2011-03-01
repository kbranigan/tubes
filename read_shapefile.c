
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "scheme.h"
#include "shapefile_src/shapefil.h"

int main(int argc, char *argv[])
{
  char * filename = (argc > 1) ? argv[1] : "prov_ab_p_geo83_e.dbf";
  int id          = (argc > 2) ? atoi(argv[2]) : -1;
  
  FILE * fp = fopen(filename, "r");
  if (!fp)
  {
    fprintf(stderr, "%s: error reading shapefile: %s\n", argv[0], filename);
    exit(1);
  }
  
  if (!stdout_is_piped())
  {
    fprintf(stderr, "%s outputs binary content. Pipe it to something that can read it.\n", argv[0]);
    exit(1);
  }
  
  DBFHandle d = DBFOpen(filename, "rb");
  if (d == NULL) { fprintf(stderr, "DBFOpen error (%s.dbf)\n", filename); exit(1); }
	
	SHPHandle h = SHPOpen(filename, "rb");
  if (h == NULL) { fprintf(stderr, "SHPOpen error (%s.dbf)\n", filename); exit(1); }
	
  long nRecordCount = DBFGetRecordCount(d);
  long nFieldCount = DBFGetFieldCount(d);
  
  if (!write_header(stdout, CURRENT_VERSION)) { fprintf(stderr, "write header failed.\n"); exit(1); }
  
  long t=0;
  long i;
  for (i = 0 ; i < nRecordCount ; i++)
  {
    if (id != -1 && id != i) continue;
    SHPObject	*psShape = SHPReadObject(h, i);
    int j;
    if (psShape->nSHPType == SHPT_POINT)
    {
      struct Shape * shape = (struct Shape*)malloc(sizeof(struct Shape));
      memset(shape, 0, sizeof(struct Shape));
      shape->unique_set_id = t++;
      shape->gl_type = GL_POINTS;
      
      shape->num_vertexs = psShape->nVertices;
      if (shape->num_vertexs <= 0) { fprintf(stderr, "shape->num_vertexs = %d", shape->num_vertexs); return 0; }
      shape->num_attributes = 0;
      
      shape->num_vertex_arrays = 1;
      shape->vertex_arrays = (struct VertexArray*)malloc(sizeof(struct VertexArray)*shape->num_vertex_arrays);
      
      struct VertexArray *va = &shape->vertex_arrays[0];
      va->num_dimensions = 2;
      va->array_type = GL_VERTEX_ARRAY;
      va->vertexs = (double*)malloc(sizeof(double)*shape->num_vertexs*va->num_dimensions);
      //fprintf(stderr, "%ld: %d\n", i, psShape->nSHPType);
      for (j = 0 ; j < shape->num_vertexs ; j++)
      {
        va->vertexs[j*va->num_dimensions+0] = psShape->padfX[j];
        va->vertexs[j*va->num_dimensions+1] = psShape->padfY[j];
      }
      write_shape(stdout, shape);
      free_shape(shape);
    }
    else if (psShape->nSHPType == SHPT_ARC || psShape->nSHPType == SHPT_POLYGON)
    {
      //fprintf(stderr, "shape %d has %d parts and %d vertices\n", psShape->nShapeId, psShape->nParts, psShape->nVertices);
      
      if (psShape->nParts == 0) { fprintf(stderr, "polygon %d has 0 parts (thats bad)\n", psShape->nShapeId); return 0; }
      
      int j, iPart;
      for (iPart = 0 ; iPart < psShape->nParts ; iPart++)
      {
        int start = psShape->panPartStart[iPart];
        int end = psShape->nVertices;
        
        if (iPart != psShape->nParts - 1)
        {
          end = psShape->panPartStart[iPart+1];
        }
        
        struct Shape * shape = (struct Shape*)malloc(sizeof(struct Shape));
        memset(shape, 0, sizeof(struct Shape));
        shape->unique_set_id = t++;
        if (psShape->nSHPType == SHPT_ARC)
          shape->gl_type = GL_LINE_STRIP;
        else if (psShape->nSHPType == SHPT_POLYGON)
          shape->gl_type = GL_LINE_LOOP;
        
        shape->num_vertexs = end - start;
        if (shape->num_vertexs <= 0) { fprintf(stderr, "shape->num_vertexs = %d", shape->num_vertexs); return 0; }
        shape->num_attributes = 0;
        
        //fprintf(stderr, "%d: %d %d (%d)\n", iPart, start, end, shape->num_vertexs);
        
        shape->num_vertex_arrays = 1;
        shape->vertex_arrays = (struct VertexArray*)malloc(sizeof(struct VertexArray)*shape->num_vertex_arrays);
        
        struct VertexArray *va = &shape->vertex_arrays[0];
        va->num_dimensions = 2;
        va->array_type = GL_VERTEX_ARRAY;
        va->vertexs = (double*)malloc(sizeof(double)*shape->num_vertexs*va->num_dimensions);
        //fprintf(stderr, "%ld: %d\n", i, psShape->nSHPType);
        for (j = start ; j < end ; j++)
        {
          va->vertexs[(j-start)*va->num_dimensions+0] = psShape->padfX[j];
          va->vertexs[(j-start)*va->num_dimensions+1] = psShape->padfY[j];
        }
        write_shape(stdout, shape);
        free_shape(shape);
        //if (iPart > 1) break;
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
