
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION write_kml
#include "scheme.h"

int write_kml(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * filename = argc > 1 ? argv[1] : "output.kml";
  if (argc == 1)
  {
    fprintf(stderr, "no filename specified, using '%s'\n", filename);
  }
  
  FILE * fp = fopen(filename, "w");
  if (!fp)
  {
    fprintf(stderr, "fopen '%s' error.\n", filename);
    exit(1);
  }
  
  fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(fp, "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n");
  
  fprintf(fp, "  <Document>\n");
  struct Shape * shape = NULL;
  long i=0, j=0, count=0;
  while ((shape = read_shape(pipe_in)))
  {
    char name[200];
    sprintf(name, "shape %d", shape->unique_set_id);
    for (i = 0 ; i < shape->num_attributes ; i++)
      if (strcmp("NAME", shape->attributes[i].name)==0 || 
          strcmp("name", shape->attributes[i].name)==0)
        sprintf(name, "%s", shape->attributes[i].value);
    fprintf(fp, "    <Placemark>\n");
    fprintf(fp, "      <name>%s</name>\n", name);
    fprintf(fp, "      <Polygon>\n");
    fprintf(fp, "        <extrude>1</extrude>\n");
    fprintf(fp, "        <altitudeMode>relativeToGround</altitudeMode>\n");
    fprintf(fp, "        <outerBoundaryIs>\n");
    fprintf(fp, "          <LinearRing>\n");
    fprintf(fp, "            <coordinates>\n");
    
    struct VertexArray * va = &shape->vertex_arrays[0];
    long i;
    for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      fprintf(fp, "%f,%f,100\n", va->vertexs[i*va->num_dimensions], va->vertexs[i*va->num_dimensions+1]);
    }
    fprintf(fp, "            </coordinates>\n");
    fprintf(fp, "          </LinearRing>\n");
    fprintf(fp, "        </outerBoundaryIs>\n");
    fprintf(fp, "      </Polygon>\n");
    fprintf(fp, "    </Placemark>\n");
    free_shape(shape);
  }
  fprintf(fp, "  </Document>\n");
  fprintf(fp, "</kml>\n");
  fclose(fp);
}
