
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
	char routeTag[20] = "";
	char agency[20] = "ttc";
	long long last_vehicles_update = 0;
	int num_requests = 1; // if num_requests == 0, then it loops forevah
	int interval = 5000; // milliseconds
	
	struct Params * params = NULL;
	params = add_string_param(params, "agency", 'a', agency, 0);
	params = add_string_param(params, "routeTag", 'r', routeTag, 0);
	params = add_longlong_param(params, "t", 't', &last_vehicles_update, 0);
	params = add_int_param(params, "num_requests", 'n', &num_requests, 0);
	params = add_int_param(params, "interval", 'i', &interval, 0);
	eval_params(params, argc, argv);
	
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
    fprintf(stderr, "requesting: %s\n", url);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    res = curl_easy_perform(curl);
    
    struct Block * block = new_block();
    block = add_string_attribute(block, "agency", agency);
    if (routeTag) block = add_string_attribute(block, "routeTag", routeTag);
    block = add_int64_attribute(block, "request_index", request_index);
    block = add_int64_attribute(block, "num_requests", num_requests);
    
    block = add_int32_column(block, "id");
    block = add_int32_column(block, "routeTag");
    block = add_string_column_with_length(block, "dirTag", 25);
    block = add_float_column(block, "x");
    block = add_float_column(block, "y");
    block = add_int32_column(block, "secsSinceReport");
    block = add_string_column_with_length(block, "predictable", 5);
    block = add_int32_column(block, "heading");
    block = add_float_column(block, "speedKmHr");
    block = add_int64_column(block, "last_vehicles_update");
    block = add_int64_column(block, "curr_vehicles_update");
    
    if (chunk.size == 0)
    {
      fprintf(stderr, "- received 0 byte response.\n");
    }
    else
    {
			long long curr_vehicles_update = last_vehicles_update;
			
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
            block = add_row_and_blank(block);
            
            int num_attributes = xmlTextReaderAttributeCount(reader);
            
            int attribute_id;
            for (attribute_id = 0 ; attribute_id < num_attributes ; attribute_id++)
            {
              xmlChar *name, *value;
              xmlTextReaderMoveToAttributeNo(reader, attribute_id);
              name = xmlTextReaderName(reader);
              xmlTextReaderMoveToElement(reader);
              value = xmlTextReaderGetAttributeNo(reader, attribute_id);
              
              int block_column_id = get_column_id_by_name(block, name);
              
              if (strcmp(name, "lat")==0) block_column_id = get_column_id_by_name(block, "y");
              else if (strcmp(name, "lon")==0) block_column_id = get_column_id_by_name(block, "x");
              
              if (block_column_id != -1)
              {
                struct Column * column = get_column(block, block_column_id);
                set_cell_from_string(block, block->num_rows-1, block_column_id, value);
              }
              else
              {
                fprintf(stderr, "field %s not found\n", name);
              }
            }
            
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
        if (ret != 0) fprintf(stderr, "xmlReader failed to parse\n");
      }
      free(chunk.memory);
      
      fprintf(stderr, "- %d vehicles received\n", count);
			
			int last_vehicles_update_column_id = get_column_id_by_name(block, "last_vehicles_update");
			int curr_vehicles_update_column_id = get_column_id_by_name(block, "curr_vehicles_update");
			int row_id;
			for (row_id = 0 ; row_id < block->num_rows ; row_id++) {
				if (last_vehicles_update_column_id != -1) {
					set_cell_from_int32(block, row_id, last_vehicles_update_column_id, last_vehicles_update);
				}
				if (curr_vehicles_update_column_id != -1) {
					set_cell_from_int32(block, row_id, curr_vehicles_update_column_id, curr_vehicles_update);
				}
			}
			
	    write_block(stdout, block);
	    free_block(block);
	    fflush(stdout);
    }
    
    if (interval != 0 && (num_requests == 0 || request_index < num_requests - 1)) usleep(interval*1000);
  }
  curl_easy_cleanup(curl);
  curl = NULL;
  xmlCleanupParser();
}
