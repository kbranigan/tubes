
#define KDTREE_SIZE_T float
#include <kdtree.hpp>

#include <vector>
#include <limits>
//#include <iostream>
#include <functional>

#include "2dtree.hpp"
#include "scheme.h"

struct duplet 
{
  typedef float value_type;
  
  inline value_type operator[](int const N) const { return d[N]; }
  inline bool operator==(duplet const& other) const { return this->d[0] == other.d[0] && this->d[1] == other.d[1]; }
  inline bool operator!=(duplet const& other) const { return this->d[0] != other.d[0] || this->d[1] != other.d[1]; }
  //friend std::ostream & operator<<(std::ostream & o, duplet const& d) { return o << "(" << d[0] << "," << d[1] << ")"; }
  
  double distance_to(struct duplet const& x) const
  {
    fprintf(stderr, "distance_to\n");
    float dx = d[0] - x.d[0];
    float dy = d[1] - x.d[1];
    double dist = std::sqrt(dx*dx + dy*dy);
    fprintf(stderr, "dist = %f (%f %f)\n", dist, d[0], dy);
    return dist;
  }
  
  value_type d[2];
  int vertex_id;
  struct Shape * shape;
};

typedef KDTree::KDTree<2, duplet, std::pointer_to_binary_function<duplet,int,float> > duplet_tree_type;

inline float return_dup(duplet d, int k) { return d[k]; }

extern "C" void * kdtree_new()
{
  duplet_tree_type * kdtree = new duplet_tree_type(std::ptr_fun(return_dup));
  return (void*)kdtree;
}

/*extern "C" void kdtree_insert(void * v_kdtree, float x, float y)
{
  if (v_kdtree == NULL) return;
  duplet_tree_type * kdtree = (duplet_tree_type *)v_kdtree;
  duplet d = { {x, y}, 0, NULL };
  kdtree->insert(d);
}*/

extern "C" void kdtree_insert_shape(void * v_kdtree, struct Shape * shape)
{
  if (v_kdtree == NULL) return;
  duplet_tree_type * kdtree = (duplet_tree_type *)v_kdtree;
  
  int i = 0;
  for (i = 0 ; i < shape->num_vertexs ; i++)
  {
    float * v = get_vertex(shape, 0, i);
    duplet d = { {v[0], v[1]}, i, shape };
    kdtree->insert(d);
  }
  kdtree_optimise(kdtree);
}

extern "C" void kdtree_optimise(void * v_kdtree)
{
  if (v_kdtree == NULL) return;
  duplet_tree_type * kdtree = (duplet_tree_type *)v_kdtree;
  kdtree->optimise();
}

extern "C" void kdtree_clear(void * v_kdtree)
{
  if (v_kdtree == NULL) return;
  duplet_tree_type * kdtree = (duplet_tree_type *)v_kdtree;
  kdtree->clear();
}

extern "C" void kdtree_find_nearest(void * v_kdtree, float x, float y, struct Shape ** shape, int * vertex_id, float * distance)
{
  if (v_kdtree == NULL) return;
  duplet_tree_type * kdtree = (duplet_tree_type *)v_kdtree;
  duplet d = { {x, y}, 0, NULL };
  
  std::pair<duplet_tree_type::const_iterator, double> p = kdtree->find_nearest(d, std::numeric_limits<double>::max());
  duplet_tree_type::iterator pItr = p.first;
  
  if (shape != NULL) *shape = pItr->shape;
  if (vertex_id != NULL) *vertex_id = pItr->vertex_id;
  if (distance != NULL) *distance = p.second;
}
