
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/xpath.h>
#include <libxml/xmlreader.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_nextbus
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

int read_nextbus(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * routeTag = NULL;
  char agency[10] = "ttc";
  long long last_vehicles_update = 0;
  int num_requests = 1; // if num_requests == 0, then it loops forevah
  int interval = 5000; // milliseconds
  int c;
  if (argv != NULL)
  while ((c = getopt(argc, argv, "a:t:n:i:r:")) != -1)
  switch (c)
  {
    case 'a': strncpy(agency, optarg, 10); break;
    case 't': last_vehicles_update = atol(optarg); break;
    case 'n': num_requests = atol(optarg); break;
    case 'i': interval = atol(optarg); break;
    case 'r': routeTag = malloc(10); strncpy(routeTag, optarg, 10); break;
    default: abort();
  }
  
  CURL *curl = NULL;
  CURLcode res;
  
  char url[400];
  
  curl = curl_easy_init();
  xmlTextReaderPtr reader;
  struct MemoryStruct chunk;
  
  int request_index = 0;
  while (request_index < num_requests || num_requests == 0)
  {
    request_index++;
    chunk.memory = NULL;
    chunk.size = 0;
    sprintf(url, "http://webservices.nextbus.com/service/publicXMLFeed?command=vehicleLocations%s%s&a=%s&t=%llu", (routeTag!=NULL ? "&r=" : ""), (routeTag!=NULL ? routeTag : ""), agency, last_vehicles_update);
    fprintf(pipe_err, "requesting: %s ", url);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    res = curl_easy_perform(curl);
    
    if (chunk.size == 0)
    {
      fprintf(pipe_err, "- received 0 byte response.\n");
    }
    else
    {
      int count = 0;
      reader = xmlReaderForMemory(chunk.memory, chunk.size, NULL, NULL, (XML_PARSE_NOBLANKS | XML_PARSE_NOCDATA | XML_PARSE_NOERROR | XML_PARSE_NOWARNING));
      if (reader != NULL)
      {
        int ret = xmlTextReaderRead(reader);
        while (ret == 1)
        {
          const xmlChar * name = xmlTextReaderConstName(reader);
          
          if (strcmp((const char *)name, "vehicle")==0)
          {
            struct Shape * shape = new_shape();
            
            void * temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"id");
            shape->unique_set_id = atoi(temp); free(temp);
            
            float v[3];
            
            temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"lon");
            v[0] = atof(temp); free(temp);
            
            temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"lat");
            v[1] = atof(temp); free(temp);
            
            temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"routeTag");
            set_attribute(shape, "routeTag", temp); free(temp);
            
            temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"dirTag");
            set_attribute(shape, "dirTag", temp); free(temp);
            
            temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"secsSinceReport");
            set_attribute(shape, "secsSinceReport", temp); free(temp);
            
            //temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"speedKmHr");
            //set_attribute(shape, "speedKmHr", temp); free(temp);
            
            temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"heading");
            set_attribute(shape, "heading", temp); free(temp);
            
            //temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"predictable");
            //set_attribute(shape, "predictable", temp); free(temp);
            
            append_vertex(shape, v);
            write_shape(pipe_out, shape);
            free_shape(shape);
            fflush(pipe_out);
            count++;
          }
          else if (strcmp((const char *)name, "lastTime")==0)
          {
            void * temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"time");
            last_vehicles_update = atoll(temp); free(temp);
          }
          
          ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
        if (ret != 0) fprintf(pipe_err, "xmlReader failed to parse\n");
      }
      free(chunk.memory);
      
      fprintf(pipe_err, "- %d vehicles received\n", count);
    }
    
    if (interval != 0 && (num_requests == 0 || request_index < num_requests - 1)) usleep(interval*1000);
  }
  free(routeTag);
  curl_easy_cleanup(curl);
  curl = NULL;
  xmlCleanupParser();
}
