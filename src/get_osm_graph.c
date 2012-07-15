
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>   // for getopt_long

extern int get_osm_graph(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err);

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION get_osm_graph
#include "scheme.h"

#include "transit_data.h"

int get_osm_graph(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  fprintf(stderr, "get_osm_graph doesn't work.\n");
  return 0;
  
  float distance = 500; // meters
  char osm_database[100] = "osm";
  //int debug = 0;
  int c;
  while (1)
  {
    static struct option long_options[] = {
      //{"row_id", required_argument, 0, 'r'},
      //{"part_id", required_argument, 0, 'p'},
      {"distance", required_argument, 0, 'd'},
      {"osm", required_argument, 0, 'o'},
      //{"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "o:d:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      //case 'r': row_id = atoi(optarg); break;
      case 'd': distance = atof(optarg); break;
      case 'o': strncpy(osm_database, optarg, sizeof(osm_database)); break;
      default: abort();
    }
  }
  
  float lat_t = -79.397174960512515;
  float lng_t =  43.646916787975790;
  
  int num_ids = 0;
  double range = 0.002;
  int *ids = NULL;
  while (num_ids < 3)
  {
    if (ids != 0) free(ids);
    range *= 2;
    ids = nearest_osm_ids(lat_t, lng_t, range, &num_ids); // ids must be free'd
  }
  fprintf(stderr, "num_ids = %d\n", num_ids);

  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    
    // manipulate data here if you like
    //write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
