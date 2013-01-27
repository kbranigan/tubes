
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mongoose.h"

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION write_webgl
#include "scheme.h"

FILE * pin = NULL;
FILE * pout = NULL;
FILE * perr = NULL;
struct mg_context *ctx;
int done = 0;

int num_shapes = 0;
struct Shape ** shapes = NULL;
struct BBox * bbox = NULL;

#define ADDCOLOR 0

void output(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  mg_printf(conn, "<html>\n<head>\n"
    "<meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n"
    "<script src=\"webgl-debug.js\"></script>\n"
    "<script type=\"text/javascript\" src=\"glMatrix-0.9.5.min.js\"></script>\n"
    "<script type=\"text/javascript\" src=\"glBasic.js\"></script>\n");
  
  mg_printf(conn,
  "<script id=\"shader-fs\" type=\"x-shader/x-fragment\">\n"
  "  #ifdef GL_ES\n"
  //"  precision highp float;\n"
  #if ADDCOLOR
    "    varying lowp vec4 vColor;\n"
  #endif
  "  #endif\n"
  "\n"
  "  void main(void) {\n"
  #if ADDCOLOR
  "    gl_FragColor = vColor;\n"
  #else
  "    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
  #endif
  "  }\n"
  "</script>\n\n");
  
  mg_printf(conn, 
  "<script id=\"shader-vs\" type=\"x-shader/x-vertex\">\n"
  "  attribute vec3 aVertexPosition;\n"
  #if ADDCOLOR
  "  attribute vec4 aVertexColor;\n"
  #endif
  "\n"
  "  uniform mat4 uMVMatrix;\n"
  "  uniform mat4 uPMatrix;\n"
  "\n"
  #if ADDCOLOR
  "  varying lowp vec4 vColor;\n"
  #endif
  "\n"
  "  void main(void) {\n"
  "    gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);\n"
  #if ADDCOLOR
  "    vColor = aVertexColor;\n"
  #endif
  "  }\n"
  "</script>\n\n");
  
  mg_printf(conn, 
  "<script type=\"text/javascript\">\n"
  "</script>\n\n");
  
  float width = 1000;
  float height = 600;
  
  mg_printf(conn, 
  "</head>\n"
  "<body onload=\"start()\">\n"
  "<canvas id=\"canvas\" width=\"%d\" height=\"%d\"></canvas>\n"
  "<script type='text/javascript'>\n"
  "function initBuffers()\n"
  "{\n", (int)width, (int)height);
  
  //mg_printf(conn, "  var b = document.getElementsByTagName('body')[0];\n");
  
  
  if (shapes == NULL)
  {
    shapes = read_all_shapes(stdin, &num_shapes);
    bbox = get_bbox_from_shapes(shapes, num_shapes);
  }
  
  int i,j;
  for (j = 0 ; j < num_shapes ; j++)
  {
    struct Shape * shape = shapes[j];
    
    struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
    mg_printf(conn, "  var va = gl.createBuffer();\n");
    mg_printf(conn, "  gl.bindBuffer(gl.ARRAY_BUFFER, va);\n");
    mg_printf(conn, "  var vertices = [\n    ");
    for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      if (shape->num_vertexs > 4000 && i % 4 == 0) continue;
      if (shape->num_vertexs > 40000 && i % 10 != 0) continue;
      float * v = get_vertex(shape, 0, i);
      if (va->num_dimensions == 1 && shape->num_vertex_arrays == 2)
      {
        float * cv = get_vertex(shape, 1, i);
        float tv = cv[0] + cv[1] + cv[2];
        mg_printf(conn, "%d,0, %d,%.5f, %d,%.5f, %d,%.5f, %d,%.5f, %d,%.5f %s\n",
                        i,
                        i, v[0] * (cv[0] / tv),
                        i, v[0] * (cv[0] / tv),
                        i, v[0] * ((cv[0]+cv[1]) / tv),
                        i, v[0] * ((cv[0]+cv[1]) / tv),
                        i, v[0],
                        ((i == shape->num_vertexs-1) ? "" : ",")
                        );
/*                        i, v[0], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
                        "%d,%.5f %s", i, v[0], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
        mg_printf(conn, "%d,%.5f %s", i, v[0], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
        mg_printf(conn, "%d,%.5f %s", i, v[0], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
        mg_printf(conn, "%d,%.5f %s", i, v[0], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
        mg_printf(conn, "%d,%.5f %s", i, v[0], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));*/
      }
      else if (va->num_dimensions == 2) mg_printf(conn, "%.5f,%.5f %s", v[0], v[1], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
      else mg_printf(conn, "%.5f,%.5f,0.0%s", v[0], v[1], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
    }
    mg_printf(conn, "  ];\n");
    mg_printf(conn, "  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);\n");
    if (shape->num_vertexs == 1)
      mg_printf(conn, "  va.gl_type = gl.POINTS;\n");
    else
      mg_printf(conn, "  va.gl_type = gl.LINES;\n");
    mg_printf(conn, "  va.itemSize = %d;\n", va->num_dimensions);
    mg_printf(conn, "  va.numItems = %d;\n", shape->num_vertexs);
    mg_printf(conn, "  vertexbuffers.push(va);\n");
    
    #if ADDCOLOR
    if (shape->num_dimensions == 2)
    {
      struct VertexArray * cva = get_or_add_array(shape, GL_COLOR_ARRAY);
      mg_printf(conn, "  var cva = gl.createBuffer();\n");
      mg_printf(conn, "  gl.bindBuffer(gl.ARRAY_BUFFER, cva);\n");
      mg_printf(conn, "  var colors = [\n    ");
      for (i = 0 ; i < shape->num_vertexs ; i++)
      {
        if (shape->num_vertexs > 4000 && i % 4 == 0) continue;
        if (shape->num_vertexs > 40000 && i % 10 != 0) continue;
        float * cv = get_vertex(shape, 1, i);
        if (cva->num_dimensions == 3)
        {
          mg_printf(conn, "0,0,%.2f,1, 0,0,%.2f,1, 0,0,%.2f,1, %.2f,0,0,1, %.2f,0,0,1, %.2f,0,0,1 %s",
                          cv[0]*1.2, cv[0]*0.8,
                          cv[0]*0.6, cv[2]*0.6,
                          cv[2]*0.8, cv[2]*1.2,
                          ((i == shape->num_vertexs-1) ? "" : ",")
                          );
          /*mg_printf(conn, "%.2f,%.2f,%.2f,1 %s", cv[0], cv[1], cv[2], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
          mg_printf(conn, "%.2f,%.2f,%.2f,1 %s", cv[0], cv[1], cv[2], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
          mg_printf(conn, "%.2f,%.2f,%.2f,1 %s", cv[0], cv[1], cv[2], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
          mg_printf(conn, "%.2f,%.2f,%.2f,1 %s", cv[0], cv[1], cv[2], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
          mg_printf(conn, "%.2f,%.2f,%.2f,1 %s", cv[0], cv[1], cv[2], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
          mg_printf(conn, "%.2f,%.2f,%.2f,1 %s", cv[0], cv[1], cv[2], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));*/
        }
        //if (cva->num_dimensions == 4) mg_printf(conn, "%.2f,%.2f,%.2f,%.2f %s", cv[0], cv[1], cv[2], cv[3], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
      }
      mg_printf(conn, "  ];\n");
      mg_printf(conn, "  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(colors), gl.STATIC_DRAW);\n");
      mg_printf(conn, "  cva.itemSize = 4;\n");
      mg_printf(conn, "  cva.numItems = %d;\n", shape->num_vertexs * 6);
      mg_printf(conn, "  colorbuffers.push(cva);\n");
    }
    #endif
    mg_printf(conn, "  \n");
  }
  
  float min_x, max_x, min_y, max_y;
  
  if (bbox != NULL && bbox->num_minmax > 0)
  {
    min_y = bbox->minmax[0].min;
    max_y = bbox->minmax[0].max;
    if (bbox->num_minmax == 1)
    {
      min_x = 0;
      max_x = shapes[0]->num_vertexs;
    }
    else
    {
      min_x = bbox->minmax[1].min;
      max_x = bbox->minmax[1].max;
    }
  }
  
  float temp = height;
  height = width * ((max_y - min_y) / (max_x - min_x));
  if (height > width * 1.5)
  {
    height = width * (width / height);
    width = temp;
  }
  fprintf(stderr, "%f %f\n", height, width);
  
  mg_printf(conn, "  document.getElementById('canvas').setAttribute('width', %d);\n", (int)width);
  mg_printf(conn, "  document.getElementById('canvas').setAttribute('height', %d);\n", (int)height);
  mg_printf(conn, "  gl.viewportWidth = canvas.width;\n");
  mg_printf(conn, "  gl.viewportHeight = canvas.height;\n");
  mg_printf(conn, "  document.getElementById('canvas').setAttribute('min_x', %f);\n", min_x);
  mg_printf(conn, "  document.getElementById('canvas').setAttribute('min_y', %f);\n", min_y);
  mg_printf(conn, "  document.getElementById('canvas').setAttribute('max_x', %f);\n", max_x);
  mg_printf(conn, "  document.getElementById('canvas').setAttribute('max_y', %f);\n", max_y);
  //mg_printf(conn, "  document.getElementById('canvas').setAttribute('height', parseFloat(document.getElementById('canvas').getAttribute('width')) * ((%f - %f) / (%f - %f)));\n", max_y, min_y, max_x, min_x);
  mg_printf(conn, "}\n</script>\n</body>\n");
  //done = 1;
}

int write_webgl(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  pin = pipe_in;
  pout = pipe_out;
  perr = pipe_err;
  char port[10] = "2234";
  int c;
  while ((c = getopt(argc, argv, "p:")) != -1)
  switch (c)
  {
    case 'p':
      if (atoi(optarg) > 1000 && atoi(optarg) < 100000) strncpy(port, optarg, sizeof(port));
      break;
    default:
      abort();
  }
  
  fprintf(stderr, "%s on port %s\n", COMMAND, port);
  ctx = mg_start();
  
  mg_set_option(ctx, "ports", port);
  mg_set_uri_callback(ctx, "/", &output, NULL);
  
  for (;;) { usleep(1000); if (done) break; }
  
  mg_stop(ctx);
}
