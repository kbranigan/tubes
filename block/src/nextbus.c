
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/xpath.h>
#include <libxml/xmlreader.h>

#include "block.h"

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

int main(int argc, char ** argv)
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
    fprintf(stderr, "requesting: %s ", url);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    res = curl_easy_perform(curl);
    
    if (chunk.size == 0)
    {
      fprintf(stderr, "- received 0 byte response.\n");
    }
    else
    {
      int count = 0;
      struct Block * block = new_block();
      block = add_long_attribute(block, "last_vehicles_update", last_vehicles_update);
      block = add_string_attribute(block, "source_url", url);
      if (routeTag != NULL) block = add_string_attribute(block, "routeTag", routeTag);
      if (agency != NULL) block = add_string_attribute(block, "agency", agency);
      reader = xmlReaderForMemory(chunk.memory, chunk.size, NULL, NULL, (XML_PARSE_NOBLANKS | XML_PARSE_NOCDATA | XML_PARSE_NOERROR | XML_PARSE_NOWARNING));
      if (reader != NULL)
      {
        int ret = xmlTextReaderRead(reader);
        while (ret == 1)
        {
          const xmlChar * name = xmlTextReaderConstName(reader);
          
          if (strcmp((const char *)name, "vehicle")==0)
          {
            if (block->num_rows == 0)
            while (xmlTextReaderMoveToNextAttribute(reader))
            {
              char * name = xmlTextReaderName(reader);
              char * value = xmlTextReaderValue(reader);
              block = add_string_column_with_length(block, name, value==NULL ? 10 : strlen(value));
              xmlFree(name);
              xmlFree(value);
            }
            if (block->num_rows == 0)
              xmlTextReaderMoveToElement(reader);
            
            block = add_row(block);
            int column_id = 0;
            while (xmlTextReaderMoveToNextAttribute(reader))
            {
              if (column_id > block->num_columns) fprintf(stderr, "bad, extra columns: %d vs %d\n", column_id, block->num_columns);
              char * name = xmlTextReaderName(reader);
              char * value = xmlTextReaderValue(reader);
              struct Column * column = get_column(block, column_id);
              if (column == NULL)
              {
                column = get_column_by_name(block, name);
                if (column == NULL) fprintf(stderr, "bad: new column %s\n", name);
              }
              
              //if (strcmp(&column->name, name) != 0)
              if (strcmp(column_get_name(column), name) != 0)
              {
                column = get_column_by_name(block, name);
                if (strcmp(column_get_name(column), name) != 0) fprintf(stderr, "bad, %s vs %s\n", column_get_name(column), name);
              }
              
              if (strlen(value) >= column->type) fprintf(stderr, "bad, (%d,%d) truncated\n", block->num_rows-1, column_id);
              set_cell(block, block->num_rows-1, column_id, value);
              xmlFree(name);
              xmlFree(value);
              column_id++;
            }
            xmlTextReaderMoveToElement(reader);
          }
          else if (strcmp((const char *)name, "lastTime")==0)
          {
            void * temp = xmlTextReaderGetAttribute(reader, (xmlChar *)"time");
            last_vehicles_update = atoll(temp);
            
            xmlFree(temp);
          }
          
          ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
        if (ret != 0) fprintf(stderr, "xmlReader failed to parse\n");
      }
      free(chunk.memory);
      
      fprintf(stderr, "- %d vehicles received\n", block->num_rows);
      write_block(stdout, block);
      free_block(block);
    }
    
    if (interval != 0 && (num_requests == 0 || request_index < num_requests - 1)) usleep(interval*1000);
  }
  free(routeTag);
  curl_easy_cleanup(curl);
  curl = NULL;
  xmlCleanupParser();
}
