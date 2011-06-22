
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/xmlreader.h>

struct tPos {
  float lat;
  float lng;
  time_t time;    // now - secsSinceReport
  float velocity; // kph
  float heading;  // nextbus reports 0-up cw degrees, this is translated to 0-right ccw radians (for sin and cos functions)
};

struct tVehicle {

  int id;
  
  // nextbus specific
  char routeTag[20];
  char dirTag[20];
  int predictable; // whatever the fuck this is
  
  int slurping_timer;
  struct tPos prev_reported_pos; // for slurping
  struct tPos reported_pos;
  struct tPos draw_pos; // calc from reported_pos + time/heading/velocity or slurp_pos
};
int num_vehicles = 0;
struct tVehicle * vehicles = NULL;

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

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_nextbus
#include "scheme.h"

int read_nextbus(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  CURL *curl;
  CURLcode res;
  
  char * agency = "ttc";
  long long last_vehicles_update = 0;
  
  char url[400];
  sprintf(url, "http://webservices.nextbus.com/service/publicXMLFeed?command=vehicleLocations&a=%s&t=%llu", agency, last_vehicles_update);
  fprintf(pipe_err, "requesting: %s\n", url);
  
  struct MemoryStruct chunk;
  chunk.memory = NULL;
  chunk.size = 0;

  curl = curl_easy_init();
  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }
  
  if (chunk.size == 0)
  {
    fprintf(pipe_err, " - received 0 byte response.\n");
    return;
  }
  if (agency == NULL) return;
  
  fprintf(pipe_err, " -");
  
  int count = 0;
  int count_new = 0;
  xmlTextReaderPtr reader;
  reader = xmlReaderForMemory(chunk.memory, chunk.size, NULL, NULL, (XML_PARSE_NOBLANKS | XML_PARSE_NOCDATA | XML_PARSE_NOERROR | XML_PARSE_NOWARNING));
  if (reader != NULL)
  {
    int ret = xmlTextReaderRead(reader);
    while (ret == 1)
    {
      const xmlChar * name = xmlTextReaderConstName(reader);
      
      if (strcmp((const char *)name, "vehicle")==0)
      {
        int nextbus_id = atoi((char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"id"));
        /*struct tVehicle * vehicle = NULL;
        int i;
        for (i = 0 ; i < num_vehicles ; i++)
        {
          if (vehicles[i].id == nextbus_id)
          {
            vehicle = &vehicles[i];
            break;
          }
        }
        
        if (vehicle == NULL)
        {
          count_new ++;
          num_vehicles++;
          vehicles = realloc(vehicles, num_vehicles*sizeof(struct tVehicle));
          vehicle = &vehicles[num_vehicles-1];
          memset(vehicle, 0, sizeof(struct tVehicle));
        }
        else
        {
          vehicle->prev_reported_pos = vehicle->reported_pos;
        }*/
        
        /*const char * temp;
        strcpy(vehicle->routeTag, (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"routeTag"));
        strcpy(vehicle->dirTag, (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"dirTag"));
        temp = (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"predictable");
        if (strcmp(temp,"true")==0) vehicle->predictable = 1;
        else vehicle->predictable = 0;*/
        
        struct Shape * shape = new_shape();
        shape->unique_set_id = nextbus_id;
        float v[3];
        v[0] = atof((char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"lon"));
        v[1] = atof((char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"lat"));
        
        set_attribute(shape, "routeTag", (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"routeTag"));
        set_attribute(shape, "dirTag", (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"dirTag"));
        set_attribute(shape, "secsSinceReport", (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"secsSinceReport"));
        set_attribute(shape, "velocity", (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"velocity"));
        set_attribute(shape, "speedKmHr", (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"speedKmHr"));
        set_attribute(shape, "heading", (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"heading"));
        set_attribute(shape, "predictable", (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"predictable"));
        append_vertex(shape, v);
        write_shape(pipe_out, shape);
        free_shape(shape);
        /*vehicle->id = nextbus_id;
        vehicle->reported_pos.lat = atof((char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"lat"));
        vehicle->reported_pos.lng = atof((char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"lon"));
        vehicle->reported_pos.velocity = atof((char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"speedKmHr"));
        vehicle->reported_pos.heading = atof((char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"heading"));
        vehicle->reported_pos.time = time(NULL) - atof((char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"secsSinceReport"));
        vehicle->draw_pos = vehicle->reported_pos;*/
        count++;
      }
      else if (strcmp((const char *)name, "lastTime")==0)
      {
        last_vehicles_update = atoll((char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"time"));
        //fprintf(pipe_err, "last_vehicles_update = %llu (%s)\n", last_vehicles_update, (char*)xmlTextReaderGetAttribute(reader, (xmlChar *)"time"));
      }
      
      ret = xmlTextReaderRead(reader);
    }
    xmlFreeTextReader(reader);
    if (ret != 0) fprintf(pipe_err, "xmlReader failed to parse\n");
  }
  
  fprintf(pipe_err, " [%d] vehicles (%d received, %d new)\n", num_vehicles, count, count_new);
  xmlCleanupParser();
}
