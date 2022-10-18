
#ifndef BLOCK_GLIB_HASHTABLE_H
#define BLOCK_GLIB_HASHTABLE_H

struct Block;

#ifdef __cplusplus
extern "C" {
#endif

void * create_hashtable_on_column(struct Block * block, const char * column_name);
int64_t search_hashtable_for_string(void * ht, char * search);
int64_t search_hashtable_for_int64(void * ht, int64_t search);
int64_t get_hashtable_count(void * ht);
void free_hashtable(void * ht);

#ifdef __cplusplus
}
#endif

#endif
