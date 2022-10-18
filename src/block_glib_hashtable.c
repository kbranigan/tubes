
#include "block.h"
#include "block_glib_hashtable.h"
#include <gmodule.h>

struct hashtable_meta {
  GHashTable * ht;
  struct Block * block;
  int64_t * row_ids;
  union {
    // int32_t * values_int32_t;
    int64_t * values_int64_t;
    // chars ?
  };
};

struct hashtable_meta * metas = NULL;
int num_metas = 0;

// static int64_t hash_from_string_fn(void * str)
// {
//   unsigned char * key = (unsigned char*)str;
//   int i, ret = 0, len = strlen((const char*)key);
//   for (i = 0 ; i < len ; i++)
//     ret += key[i] << (i%16);
//   return ret;
// }

// static int strings_equal_fn(void * key1, void * key2)
// {
//   return strcmp((char*)key1, (char*)key2)==0;
// }

// static int64_t hash_from_int_fn(void * k)
// {
//   return *(int64_t*)k;
// }

// static int ints_equal_fn(void * key1, void * key2)
// {
//   return *(int64_t*)key1 == *(int64_t*)key2;
// }

// void * _create_string_hashtable_on_column(struct Block * block, const char * column_name)
// {
//   int row_id, column_id = get_column_id_by_name_or_exit(block, column_name);
//   struct hashtable * ht = create_hashtable(16, hash_from_string_fn, strings_equal_fn);
  
//   num_metas++;
//   metas = (struct hashtable_meta *)realloc(metas, sizeof(struct hashtable_meta)*num_metas);
//   struct hashtable_meta * meta = &metas[num_metas-1];
  
//   meta->ht = ht;
//   meta->block = block;
//   meta->row_ids = (int32_t*)malloc(sizeof(int32_t)*block->num_rows);
  
//   int dups = 0;
//   for (row_id = 0 ; row_id < block->num_rows ; row_id++)
//   {
//     meta->row_ids[row_id] = row_id; // so lame
//     char * value = (char*)get_cell(block, row_id, column_id);
//     int value_length = strlen(value);
//     if (value != NULL && value_length > 0)
//     {
//       if (hashtable_search(ht, value) == NULL)
//       {
//         char * temp = (char*)malloc(value_length+1);
//         strncpy(temp, value, value_length);
//         temp[value_length] = 0;
//         hashtable_insert(ht, temp, &meta->row_ids[row_id]);
//       }
//       else dups++;
//     }
//   }
//   if (dups > 0) fprintf(stderr, "create_hashtable_on_column('%s'), there are %d duplicate rows (ignored) of total: %d\n", column_name, dups, block->num_rows);
//   return (void*)ht;
// }

// void * _create_int_hashtable_on_column(struct Block * block, const char * column_name)
// {
//   int row_id, column_id = get_column_id_by_name_or_exit(block, column_name);
//   // struct hashtable * ht = create_hashtable(16, hash_from_int_fn, ints_equal_fn);
//   struct hashtable * ht = NULL;
  
//   num_metas++;
//   metas = (struct hashtable_meta *)realloc(metas, sizeof(struct hashtable_meta)*num_metas);
//   struct hashtable_meta * meta = &metas[num_metas-1];
  
//   meta->ht = ht;
//   meta->block = block;
//   meta->row_ids = NULL;
  
//   int dups = 0;
//   for (row_id = 0 ; row_id < block->num_rows ; row_id++)
//   {
//     // if (hashtable_search(ht, get_cell(block, row_id, column_id)) == NULL)
//     // {
//     //   hashtable_insert(ht, get_cell(block, row_id, column_id), get_row(block, row_id));
//     // }
//     // else dups++;
//   }
//   if (dups > 0) fprintf(stderr, "%s('%s'), there are %d duplicate rows (ignored) of total: %d\n", __func__, column_name, dups, block->num_rows);
//   return (void*)ht;
// }

void * create_hashtable_on_column(struct Block * block, const char * column_name)
{
  if (block == NULL) { fprintf(stderr, "%s called on NULL block.\n", __func__); return NULL; }
  
  int column_id = get_column_id_by_name_or_exit(block, column_name);
  if (column_id == -1) { fprintf(stderr, "%s couldn't find '%s' column.\n", __func__, column_name); return NULL; }

  if (get_column(block, column_id)->type == TYPE_CHAR) {
    fprintf(stderr, "dang man\n");
    exit(1);
  } else if (get_column(block, column_id)->type == TYPE_INT && get_column(block, column_id)->bsize == sizeof(int64_t)) {
    GHashTable * ht = g_hash_table_new_full(g_int64_hash, g_int64_equal, NULL, NULL);

    num_metas++;
    metas = (struct hashtable_meta *)realloc(metas, sizeof(struct hashtable_meta)*num_metas);
    struct hashtable_meta * meta = &metas[num_metas-1];
    
    meta->ht = ht;
    meta->block = block;
    meta->row_ids = (int64_t*)malloc(sizeof(int64_t)*block->num_rows);
    meta->values_int64_t = (int64_t*)malloc(sizeof(int64_t)*block->num_rows);

    for (int64_t row_id = 0 ; row_id < block->num_rows ; row_id++) {
      meta->values_int64_t[row_id] = get_cell_as_int64(block, row_id, column_id);
      meta->row_ids[row_id] = row_id;
      g_hash_table_insert(ht, &meta->values_int64_t[row_id], &meta->row_ids[row_id]);
    }

    fprintf(stderr, "create_hashtable_on_column created on field '%s' with %d rows\n", column_name, g_hash_table_size(ht));
    return ht;
  }
  else { fprintf(stderr, "%s called on column which is not a string.\n", __func__); return NULL; }
}

// int32_t search_hashtable_for_string(void * ht, char * search)
// {
//   if (ht == NULL) { fprintf(stderr, "search_hashtable_for_string called with NULL hashtable\n"); exit(0); }
//   if (search == NULL) { fprintf(stderr, "search_hashtable_for_string called with NULL search value\n"); exit(0); }
//   int32_t * row_id = (int32_t *)hashtable_search((struct hashtable*)ht, search);
//   if (row_id == NULL) return -1;
//   else return *row_id;
// }

int64_t search_hashtable_for_int64(void * ht, int64_t search)
{
  if (ht == NULL) { fprintf(stderr, "search_hashtable_for_string called with NULL hashtable\n"); exit(0); }
  gpointer p = g_hash_table_lookup(ht, &search);
  if (p == NULL) return -1;
  return *(int64_t*)p;
}

int64_t get_hashtable_count(void * ht)
{
  return g_hash_table_size(ht);
}

void free_hashtable(void * ht)
{
  int i;
  for (i = 0 ; i < num_metas ; i++)
    if (metas[i].ht == ht)
    {
      g_hash_table_destroy(ht);
      free(metas[i].row_ids);
      free(metas[i].values_int64_t);
      metas[i].ht = NULL;     // kbfu
      metas[i].block = NULL;  // kbfu
      break;
    }
  if (num_metas == 0) free(metas);
}
