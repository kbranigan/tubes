
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#include "ext/cJSON.h"

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_foursquare
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

int read_foursquare(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char url[500] = "https://api.foursquare.com/v2/venues/";
  
  char version[10] = "20110821";
  
  char command[50] = "search";
  
  char latlng[50] = "43.64,-79.39";
  int limit = -1;
  int radius = -1;
  
  char client_id[100] = "4RK2MDG5HONHD1HVXBGGMLUKHV2FM52LJI5LKWZBMLAEOM0Y";
  char client_secret[100] = "EHAMGMV1BINCUFJYELKM4CZ0APQV3NPHLNSNXENOXZQSQVKQ";
  
  int c;
  if (argv != NULL)
  while ((c = getopt(argc, argv, "c:l:n:r:")) != -1)
  switch (c)
  {
    case 'c':
      strncpy(command, optarg, sizeof(command));
      break;
    case 'l':
      strncpy(latlng, optarg, sizeof(latlng));
      break;
    case 'n':
      limit = atol(optarg);
      break;
    case 'r':
      radius = atol(optarg);
      break;
    default:
      abort();
  }
  
  char temp[150] = "";
  
  strcat(url, command);
  strcat(url, "?v="); strcat(url, version);
  strcat(url, "&ll="); strcat(url, latlng);
  if (limit != -1) { strcat(url, "&limit="); sprintf(temp, "%d", limit); strcat(url, temp); }
  if (radius != -1) { strcat(url, "&radius="); sprintf(temp, "%d", radius); strcat(url, temp); }
  strcat(url, "&client_id="); strcat(url, client_id);
  strcat(url, "&client_secret="); strcat(url, client_secret);
  
  CURL *curl = NULL;
  CURLcode res;
  
  curl = curl_easy_init();
  struct MemoryStruct chunk;
  
  chunk.memory = NULL;
  chunk.size = 0;
  
  /*fprintf(pipe_err, "requesting: %s\n", url);*/
  
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  res = curl_easy_perform(curl);
  
  if (chunk.size == 0)
  {
    fprintf(pipe_err, " - received 0 byte response.\n");
    return;
  }
  
  /*fprintf("response size = %d\n", chunk.size);*/
  
  /*FILE * fp = fopen("data/example_foursquare_json.c", "r");
  char * data = NULL;
  int length = 0;
  do {
    length++;
    data = realloc(data, length);
    c = getc(fp);
    if (c != EOF) data[length-1] = c;
    
  } while (c != EOF);*/
  
  cJSON * root = cJSON_Parse(chunk.memory);
  
  cJSON * venues = cJSON_GetObjectItem(cJSON_GetObjectItem(root, "response"), "venues");
  
  cJSON * venue = venues->child;
  while (venue)
  {
    float v[3] = { 0, 0, 0 };
    cJSON * location = cJSON_GetObjectItem(venue, "location");
    v[0] = cJSON_GetObjectItem(location, "lng")->valuedouble;
    v[1] = cJSON_GetObjectItem(location, "lat")->valuedouble;
    
    struct Shape * shape = new_shape();
    set_attribute(shape, "foursquare_id", cJSON_GetObjectItem(venue, "id")->valuestring);
    set_attribute(shape, "name", cJSON_GetObjectItem(venue, "name")->valuestring);
    
    if (cJSON_GetObjectItem(location, "address"))
      set_attribute(shape, "address", cJSON_GetObjectItem(location, "address")->valuestring);
    
    if (cJSON_GetObjectItem(location, "city"))
      set_attribute(shape, "city", cJSON_GetObjectItem(location, "city")->valuestring);
    
    cJSON * stats = cJSON_GetObjectItem(venue, "stats");
    
    if (cJSON_GetObjectItem(stats, "checkinsCount"))
    {
      sprintf(temp, "%d", cJSON_GetObjectItem(stats, "checkinsCount")->valueint);
      set_attribute(shape, "checkins", temp);
    }
    if (cJSON_GetObjectItem(stats, "usersCount"))
    {
      sprintf(temp, "%d", cJSON_GetObjectItem(stats, "usersCount")->valueint);
      set_attribute(shape, "users", temp);
    }
    if (cJSON_GetObjectItem(stats, "tipCount"))
    {
      sprintf(temp, "%d", cJSON_GetObjectItem(stats, "tipCount")->valueint);
      set_attribute(shape, "tips", temp);
    }
    append_vertex(shape, v);
    write_shape(pipe_out, shape);
    free_shape(shape);
    venue = venue->next;
  }
  
  free(chunk.memory);
  curl_easy_cleanup(curl);
  curl = NULL;
}
