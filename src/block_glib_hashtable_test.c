
// #include <gmodule.h>
// #include <stdio.h>
// #include <inttypes.h>
#include "block.h"
#include "block_glib_hashtable.h"

int main() {
	// GHashTable * str_ht = g_hash_table_new(g_str_hash, g_str_equal);
	// GHashTable * int64_ht = g_hash_table_new_full(g_int64_hash, g_int64_equal, NULL, NULL);

	// fprintf(stderr, "%d\n", hash_func("asdf"));
	// g_hash_table_insert(str_ht, "asdf", "qwer");

	// int64_t * keys = NULL;
	// int num_keys = 1;
	// keys = realloc(keys, sizeof(int64_t) * num_keys);
	// int64_t key = 1234;
	// int64_t value = 5678;
	// g_hash_table_insert(int64_ht, &key, &value);

	// int64_t hehe = 1234;
	// gpointer p = g_hash_table_lookup(int64_ht, &hehe);

	// if (p != NULL) {
	// 	fprintf(stderr, "%lld\n", *(int64_t*)(p));
	// }

	struct Block * block = new_block();
	block = add_int64_column(block, "fun");
	block = set_num_rows(block, 5);
	for (int row_id = 0 ; row_id < block->num_rows ; row_id++) {
		int64_t data = row_id * 100 + 100;
		set_cell(block, row_id, 0, &data);
	}

	void * ht = create_hashtable_on_column(block, "fun");
	int64_t row_id = search_hashtable_for_int64(ht, 300);
	fprintf(stderr, "row_id: %lld\n", row_id);
	free_hashtable(ht);

	write_block(stdout, block);
}
