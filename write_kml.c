
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheme.h"

int main(int argc, char ** argv)
{
  if (!stdin_is_piped())
  {
    fprintf(stderr, "%s needs a data source. (redirected pipe, using |)\n", argv[0]);
    exit(1);
  }
  
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
  
  if (!read_header(stdin, FILE_VERSION_2))
  {
    fprintf(stderr, "read header failed.\n");
    exit(1);
  }
  
  fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(fp, "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n");
  
  fprintf(fp, "  <Document>\n");
  struct Shape * shape = NULL;
  long i=0, j=0, count=0;
  while ((shape = read_shape(stdin)))
  {
    char * name = "lolol";
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
