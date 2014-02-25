
#ifndef BLOCK_HASHTABLE_H
#define BLOCK_HASHTABLE_H

struct Block;

#ifdef __cplusplus
extern "C" {
#endif

void * create_hashtable_on_column(struct Block * block, const char * column_name);
int32_t search_hashtable_for_string(void * ht, char * search);
int32_t search_hashtable_for_int(void * ht, int32_t search);
int32_t get_hashtable_count(void * ht);
void free_hashtable(void * ht);

#ifdef __cplusplus
}
#endif

#endif
