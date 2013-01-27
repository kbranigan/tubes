
#ifndef _2DTREE_H
#define _2DTREE_H

#ifdef __cplusplus
extern "C" {
#endif

void * kdtree_new();
//void kdtree_insert(void * v_kdtree, float x, float y, void * user_ptr);
void kdtree_insert_shape(void * v_kdtree, struct Shape * shape);
void kdtree_optimise(void * v_kdtree);
void kdtree_clear(void * v_kdtree);
void kdtree_find_nearest(void * v_kdtree, float x, float y, struct Shape ** shape, int * vertex_id, float * distance);

#ifdef __cplusplus
}
#endif

#endif
