
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mongoose.h"

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION write_http
#include "scheme.h"

FILE * pin = NULL;
FILE * pout = NULL;
FILE * perr = NULL;

void output(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  struct Shape * shape = NULL;
  while ((shape = read_shape(pin)))
  {
    //write_shape(pout, shape);
    mg_printf(conn, "durr\n");
    free_shape(shape);
  }
}

int write_http(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  pin = pipe_in;
  pout = pipe_out;
  perr = pipe_err;
  char port[10] = "2232";
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
  
  struct mg_context *ctx = mg_start();
  
  mg_set_option(ctx, "ports", port);
  mg_set_uri_callback(ctx, "/", &output, NULL);
  
  for (;;) sleep(10000);
  
  mg_stop(ctx);
}
