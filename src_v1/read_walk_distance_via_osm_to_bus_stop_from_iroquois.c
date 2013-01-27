
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
#include <curl/curl.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION read_walk_distance_via_osm_to_bus_stop_from_iroquois
#include "scheme.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)data;
  
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL)
  {
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }
  
  memcpy(&(mem->memory[mem->size]), ptr, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
  
  return realsize;
}

int read_walk_distance_via_osm_to_bus_stop_from_iroquois(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  //char filename[300] = "";
  //int num_attributes = -1;
  //int c;
  //while ((c = getopt(argc, argv, "f:a:")) != -1)
  //switch (c)
  //{
  //  case 'f':
  //    strncpy(filename, optarg, 300);
  //    break;
  //  case 'a':
  //    num_attributes = atoi(optarg);
  //    break;
  //  default:
  //    abort();
  //}
  
  
  CURL *curl = NULL;
  CURLcode res;
  struct MemoryStruct chunk;
  
  char url[400];
  
  curl = curl_easy_init();
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in))) // address points
  {
    if (shape->num_vertexs > 1) { fprintf(stderr, "This is likely not the data this script was expecting. (was expecting addres points, single vertex shapes)\n"); break; }
    
    float * v = get_vertex(shape, 0, 0);
    
    chunk.memory = NULL;
    chunk.size = 0;
    
    sprintf(url, "http://localhost:3333/walk_distance_to_a_bus_stop?from=%f,%f", v[1], v[0]);
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    res = curl_easy_perform(curl);
    
    if (chunk.size == 0)
    {
      set_attribute(shape, "route_error", "can not find a bus stop");
    }
    else
    {
      char * walk_distance = strtok(chunk.memory, ",");
      char * myttc_stop_id = strtok(NULL, ",");
      char * myttc_stop_name = strtok(NULL, ",");
      if (walk_distance != NULL) set_attribute(shape, "walk_distance", walk_distance);
      if (myttc_stop_id != NULL) set_attribute(shape, "myttc_stop_id", myttc_stop_id);
      if (myttc_stop_name != NULL) set_attribute(shape, "myttc_stop_name", myttc_stop_name);
    }
    
    free(chunk.memory);
    
    // manipulate data here if you like
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
