
#include <string>
#include <map>
#include <math.h>

#include "kstar.hpp"
#include "edgelink.hpp"

//std::vector <Vertex *> nearest;
//float nearest_lat;
//float nearest_lng;

void get_nearest_edge_vertex(float lat_t, float lng_t, Edge **nearest_edge, Vertex **nearest_on_edge)
{
  int num_ids = 0;
  
  double range = 0.002;
  int *ids = NULL;
  while (num_ids < 3)
  {
    range *= 2;
    ids = nearest_osm_ids(lat_t, lng_t, range, &num_ids); // ids must be free'd
    if (num_ids < 3 && num_ids != 0) free(ids);
  }
  
  //nearest.clear();
  //for (int i = 0 ; i < num_ids ; i++)
  //{
  //  nearest.push_back(osm_v[ids[i]]);
  //}
  
  float closest_dist = 10000;
  float closest_angle = 0;
  Vertex *closest_vertex1 = NULL;
  Vertex *closest_vertex2 = NULL;
  Edge *closest_edge = NULL;
  
  Vertex *prev_vertex1 = NULL;

  float lat_diff, lng_diff, dist1_2, dist1_m, dist2_m, angle1_2, angle1_m, angle2_m, angle_m_1_2, angle_1_2_m, dist_g;
  
  for (int i = 0 ; i < num_ids ; i++)
  {
    Vertex *vertex1 = osm_v[ids[i]];
    
    for (int j = 0 ; j < vertex1->num_e_out ; j++)
    {
      Vertex *vertex2 = vertex1->e_out[j]->to;
      if (vertex2 == prev_vertex1 || vertex2 == vertex1) continue;
      lat_diff = vertex2->lat - vertex1->lat;
      lng_diff = vertex2->lng - vertex1->lng;
      dist1_2 = sqrt(lat_diff*lat_diff + lng_diff*lng_diff);
      angle1_2 = atan2(lng_diff, lat_diff);
      
      lat_diff = lat_t - vertex1->lat;
      lng_diff = lng_t - vertex1->lng;
      dist1_m = sqrt(lat_diff*lat_diff + lng_diff*lng_diff);
      angle1_m = atan2(lng_diff, lat_diff);
      
      lat_diff = lat_t - vertex2->lat;
      lng_diff = lng_t - vertex2->lng;
      dist2_m = sqrt(lat_diff*lat_diff + lng_diff*lng_diff);
      angle2_m = atan2(lng_diff, lat_diff);
      
      if (dist2_m < fabs(closest_dist))
      {
        closest_dist = dist2_m;
        closest_angle = angle2_m+3.141592654;
        closest_vertex1 = vertex2;
        closest_vertex2 = vertex2;
        closest_edge = vertex1->e_out[j];
      }
      
      if (dist1_m < fabs(closest_dist))
      {
        closest_dist = dist1_m;
        closest_angle = angle1_m+3.141592654;
        closest_vertex1 = vertex1;
        closest_vertex2 = vertex1;
        closest_edge = vertex1->e_out[j];
      }
      
      angle_m_1_2 = -1 * (angle1_m - angle1_2);
      angle_1_2_m = -1 * (angle1_2 - angle2_m);
      
      if (angle_m_1_2 > 3.141592654) angle_m_1_2 -= 6.283185308;
      if (angle_m_1_2 < -3.141592654) angle_m_1_2 += 6.283185308;
      
      if (angle_1_2_m > 3.141592654) angle_1_2_m -= 6.283185308;
      if (angle_1_2_m < -3.141592654) angle_1_2_m += 6.283185308;
      
      dist_g = sin(angle_m_1_2) * dist1_m; // length of perpendicular (law of sines)
      
      if (angle_m_1_2 > 1.570796327 || angle_m_1_2 < -1.570796327 || (angle_1_2_m < 1.570796327 && angle_1_2_m > -1.570796327)) continue;

      if (fabs(dist_g) < fabs(closest_dist))
      {
        closest_dist = dist_g;
        closest_angle = angle1_2+1.570796327;
        closest_vertex1 = vertex1;
        closest_vertex2 = vertex2;
        closest_edge = vertex1->e_out[j];
      }
    }
    prev_vertex1 = vertex1;
  }
  
  free(ids);
  
  if (closest_vertex1 != NULL)
  {
    *nearest_on_edge = new Vertex(V_NONE, 0, lat_t + cos(closest_angle)*closest_dist, lng_t + sin(closest_angle)*closest_dist, 0);
    *nearest_edge = closest_edge;
  }
}

void connect_vertex_to_edge(Vertex *nearest_on_edge, Edge *nearest_edge)
{
  if (nearest_edge == NULL || nearest_edge->type != E_STREET || nearest_on_edge == NULL) { printf("get_nearest FAILED\n"); exit(1); }

  Street *nearest_street = (Street*)nearest_edge;
  Street *s = NULL;
	
  s = new Street(nearest_street->osm_id, nearest_on_edge, nearest_street->from, nearest_street->name);
  s->length *= 1.1;

  s = new Street(nearest_street->osm_id, nearest_street->from, nearest_on_edge, nearest_street->name);
  s->length *= 1.1;

  s = new Street(nearest_street->osm_id, nearest_on_edge, nearest_street->to, nearest_street->name);
  s->length *= 1.1;

  s = new Street(nearest_street->osm_id, nearest_street->to, nearest_on_edge, nearest_street->name);
  s->length *= 1.1;

  //if (nearest_on_edge->num_e_in != 3) { printf("linking the stop to the osm graph FAILED\n"); exit(1); }
}
