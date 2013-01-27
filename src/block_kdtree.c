
#include <limits>
#include "block.h"
#include "block_kdtree.h"
#include "../ext/kdtree.hpp"

struct duplet 
{
  typedef double value_type;
  
  inline value_type operator[](int const N) const { return d[N]; }
  inline bool operator==(duplet const& other) const { return this->d[0] == other.d[0] && this->d[1] == other.d[1]; }
  inline bool operator!=(duplet const& other) const { return this->d[0] != other.d[0] || this->d[1] != other.d[1]; }
  //friend std::ostream & operator<<(std::ostream & o, duplet const& d) { return o << "(" << d[0] << "," << d[1] << ")"; }
  
  double distance_to(struct duplet const& x) const
  {
    fprintf(stderr, "distance_to\n");
    double dx = d[0] - x.d[0];
    double dy = d[1] - x.d[1];
    double dist = std::sqrt(dx*dx + dy*dy);
    fprintf(stderr, "dist = %f (%f %f)\n", dist, d[0], dy);
    return dist;
  }
  
  value_type d[2];
  int32_t row_id;
  //void * data;
};

struct not_me
{
  duplet me;
  not_me(duplet m) : me(m) {}
  
  bool operator()( duplet const& n ) const { return n.row_id != me.row_id; }
};


typedef KDTree::KDTree<2, duplet, std::pointer_to_binary_function<duplet,int,double> > duplet_tree_type;
inline double return_dup(duplet d, int k) { return d[k]; }

extern "C" void * create_kdtree_for_block(struct Block * block)
{
  if (block == NULL) return NULL;
  
  fprintf(stderr, "create_kdtree_for_block ");
  
  duplet_tree_type * kdtree = new duplet_tree_type(std::ptr_fun(return_dup));
  
  int32_t column_x_id = get_column_id_by_name(block, const_cast<char *>("x"));
  int32_t column_y_id = get_column_id_by_name(block, const_cast<char *>("y"));
  
  if (column_x_id == -1 || column_y_id == -1) { fprintf(stderr, "create_kdtree_for_block() called with a block that has no x or y field\n"); return NULL; }
  
  int row_id;
  for (row_id = 0 ; row_id < block->num_rows ; row_id++)
  {
    double x = get_cell_as_double(block, row_id, column_x_id);
    double y = get_cell_as_double(block, row_id, column_y_id);
    duplet d = { {x, y}, row_id };
    kdtree->insert(d);
  }
  
	kdtree->optimize();
  
  fprintf(stderr, " - %ld inserted\n", kdtree->size());
  
  return (void*)kdtree;
}

extern "C" struct kdtree_results search_kdtree_find_within_range(void * kdtree_v, double x, double y, double range)
{
  duplet_tree_type * kdtree = (duplet_tree_type *)kdtree_v;
  
  struct kdtree_results res;
  res.count = 0;
  res.row_ids = NULL;
  duplet d = { {x, y}, NULL };
  std::vector<duplet> v;
  kdtree->find_within_range(d, range/1000, std::back_inserter(v));
  
  if (v.size() > 0)
  {
    res.row_ids = (int32_t*)malloc(sizeof(int32_t)*v.size());
    for (std::vector<duplet>::iterator i = v.begin() ; i != v.end() ; i++)
    {
      res.row_ids[res.count++] = i->row_id;
    }
  }
    
  return res;
}

extern "C" int32_t search_kdtree_find_nearest_not_row_id(void * kdtree_v, double x, double y, int32_t row_id)
{
  duplet_tree_type * kdtree = (duplet_tree_type *)kdtree_v;
  
  duplet d = { {x, y}, row_id };
  
  std::pair<duplet_tree_type::const_iterator, double> p = kdtree->find_nearest_if(d, std::numeric_limits<double>::max(), not_me(d));
  duplet_tree_type::iterator pItr = p.first;
  
  return pItr->row_id;
}

extern "C" int32_t search_kdtree_find_nearest(void * kdtree_v, double x, double y)
{
  duplet_tree_type * kdtree = (duplet_tree_type *)kdtree_v;
  
  duplet d = { {x, y}, 0 };
  
  std::pair<duplet_tree_type::const_iterator, double> p = kdtree->find_nearest(d, std::numeric_limits<double>::max());
  duplet_tree_type::iterator pItr = p.first;
  
  return pItr->row_id;
}

extern "C" void free_kdtree(void * kdtree_v)
{
  duplet_tree_type * kdtree = (duplet_tree_type *)kdtree_v;
}

/*extern "C" void * kdtree_new()
{
  duplet_tree_type * kdtree = new duplet_tree_type(std::ptr_fun(return_dup));
  return (void*)kdtree;
}

extern "C" void kdtree_insert(void * v_kdtree, float x, float y, void * data)
{
  if (v_kdtree == NULL) return;
  duplet_tree_type * kdtree = (duplet_tree_type *)v_kdtree;
  duplet d = { {x, y}, data };
  kdtree->insert(d);
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

extern "C" void * kdtree_find_nearest(void * v_kdtree, float x, float y)
{
  if (v_kdtree == NULL) return NULL;
  duplet_tree_type * kdtree = (duplet_tree_type *)v_kdtree;
  duplet d = { {x, y}, NULL };
  
  std::pair<duplet_tree_type::const_iterator, double> p = kdtree->find_nearest(d, std::numeric_limits<double>::max());
  duplet_tree_type::iterator pItr = p.first;
  
  return pItr->data;
}

extern "C" void ** kdtree_find_within_range(void * v_kdtree, float x, float y, float range, int * num_pointers)
{
  if (v_kdtree == NULL) return NULL;
  duplet_tree_type * kdtree = (duplet_tree_type *)v_kdtree;
  duplet d = { {x, y}, NULL };
  
  std::vector<duplet> v;
  kdtree->find_within_range(d, range/10000, std::back_inserter(v));
  
  void ** ret = NULL;
  if (v.size() > 0)
  {
    ret = (void**)malloc(sizeof(void*)*v.size());
    int j = 0;
    for (std::vector<duplet>::iterator i = v.begin() ; i != v.end() ; i++)
    {
      *(ret+j) = i->data;
      j++;
    }
  }
  *num_pointers = v.size();
  v.clear();
  
  return ret;
}
*/
