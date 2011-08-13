
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
int num_points = 0;
struct Shape ** shapes = NULL;

void output(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  mg_printf(conn, "<html>\n<head>\n"
    "<meta http-equiv=\"content-type\" content=\"text/html; charset=ISO-8859-1\">\n"
    "<script type=\"text/javascript\" src=\"glMatrix-0.9.5.min.js\"></script>\n"
    "<script type=\"text/javascript\" src=\"glBasic.js\"></script>\n");
  
  mg_printf(conn,
  "<script id=\"shader-fs\" type=\"x-shader/x-fragment\">\n"
  "  #ifdef GL_ES\n"
  "  precision highp float;\n"
  "  #endif\n"
  "\n"
  "  void main(void) {\n"
  "    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
  "  }\n"
  "</script>\n\n");
  
  mg_printf(conn, 
  "<script id=\"shader-vs\" type=\"x-shader/x-vertex\">\n"
  "  attribute vec3 aVertexPosition;\n"
  "\n"
  "  uniform mat4 uMVMatrix;\n"
  "  uniform mat4 uPMatrix;\n"
  "\n"
  "  void main(void) {\n"
  "    gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);\n"
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
  "<canvas id=\"canvas\" width=\"%d\" height=\"%d\" style='border:1px solid #DDD'></canvas>\n"
  "<script type='text/javascript'>\n"
  "function initBuffers()\n"
  "{\n", (int)width, (int)height);
  
  mg_printf(conn, "  var b = document.getElementsByTagName('body')[0];\n");
  
  int shape_index = 0;
  
  float min_x = 10000, min_y = 10000;
  float max_x = -10000, max_y = -10000;
  
  struct Shape * shape = NULL;
  
  if (num_shapes == 0)
  while ((shape = read_shape(pin)))
  {
    num_shapes++;
    shapes = realloc(shapes, sizeof(struct Shape*)*num_shapes);
    shapes[num_shapes-1] = shape;
    if (shape->num_vertexs == 1) num_points++;
  }
  
  if (num_points > 1)
  {
    mg_printf(conn, "  var buffer = gl.createBuffer();\n");
    mg_printf(conn, "  gl.bindBuffer(gl.ARRAY_BUFFER, buffer);\n");
    mg_printf(conn, "  var vertices = [\n    ");
    int i=0, j;
    for (j = 0 ; j < num_shapes ; j++)
    {
      if (shapes[j]->num_vertexs != 1) continue;
      float * v = get_vertex(shapes[j], 0, 0);
      if (v[0] < min_x) min_x = v[0];
      if (v[0] > max_x) max_x = v[0];
      if (v[1] < min_y) min_y = v[1];
      if (v[1] > max_y) max_y = v[1];
      mg_printf(conn, "%.5f,%.5f,0.0%s", v[0], v[1], (i == num_points-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
      i++;
    }
    mg_printf(conn, "  ];\n");
    mg_printf(conn, "  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);\n");
    mg_printf(conn, "  buffer.gl_type = gl.POINTS;\n");
    mg_printf(conn, "  buffer.itemSize = 3;\n");
    mg_printf(conn, "  buffer.numItems = %d;\n", num_points);
    mg_printf(conn, "  buffers.push(buffer);\n");
  }
  
  int i, j;
  for (j = 0 ; j < num_shapes ; j++)
  {
    if (shapes[j]->num_vertexs == 1) continue;
    shape = shapes[j];
    
    struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
    mg_printf(conn, "  var buffer = gl.createBuffer();\n");
    mg_printf(conn, "  gl.bindBuffer(gl.ARRAY_BUFFER, buffer);\n");
    mg_printf(conn, "  var vertices = [\n    ");
    for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      if (va->num_dimensions < 2) continue;
      float * v = get_vertex(shape, 0, i);
      if (v[0] < min_x) min_x = v[0];
      if (v[0] > max_x) max_x = v[0];
      if (v[1] < min_y) min_y = v[1];
      if (v[1] > max_y) max_y = v[1];
      mg_printf(conn, "%.5f,%.5f,0.0%s", v[0], v[1], (i == shape->num_vertexs-1) ? "\n" : (i%32!=31 ? "," : ",\n    "));
      shape_index ++;
    }
    
    mg_printf(conn, "  ];\n");
    mg_printf(conn, "  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);\n");
    mg_printf(conn, "  buffer.gl_type = gl.LINE_STRIP;\n");
    mg_printf(conn, "  buffer.itemSize = 3;\n");
    mg_printf(conn, "  buffer.numItems = %d;\n", shape->num_vertexs);
    mg_printf(conn, "  buffers.push(buffer);\n");
    //free_shape(shape);
  }
  
  height = width * ((max_y - min_y) / (max_x - min_x));
  if (height > width * 1.5) height = width * 1.5;
  
  mg_printf(conn, "  document.getElementById('canvas').setAttribute('width', %d);\n", (int)width);
  mg_printf(conn, "  document.getElementById('canvas').setAttribute('height', %d);\n", (int)height);
  mg_printf(conn, "  gl.viewportWidth = canvas.width;\n");
  mg_printf(conn, "  gl.viewportHeight = canvas.height;\n");
  mg_printf(conn, "  b.setAttribute('min_x', %f);\n", min_x);
  mg_printf(conn, "  b.setAttribute('min_y', %f);\n", min_y);
  mg_printf(conn, "  b.setAttribute('max_x', %f);\n", max_x);
  mg_printf(conn, "  b.setAttribute('max_y', %f);\n", max_y);
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
