
#ifndef BLOCK_KDTREE_H
#define BLOCK_KDTREE_H

#ifdef __cplusplus
extern "C" {
#endif

struct kdtree_results {
  int32_t count;
  int32_t * row_ids;
};

void * create_kdtree_for_block(struct Block * block);
struct kdtree_results search_kdtree_find_within_range(void * kdtree_v, double x, double y, double range);
int32_t search_kdtree_find_nearest_not_row_id(void * kdtree_v, double x, double y, int32_t row_id);
int32_t search_kdtree_find_nearest(void * kdtree_v, double x, double y);
void free_kdtree(void * kdtree_v);

#ifdef __cplusplus
}
#endif

#endif
