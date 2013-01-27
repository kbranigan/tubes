
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ext/hashtable.h"

static unsigned int hash_from_string_fn(void * str)
{
  unsigned char * key = (unsigned char*)str;
  int i, ret = 0, len = strlen((const char*)key);
  for (i = 0 ; i < len ; i++)
    ret += key[i] << (i%16);
  return ret;
}

static int strings_equal_fn(void * key1, void * key2)
{
  return strcmp((char*)key1, (char*)key2)==0;
}

int main(int argc, char ** argv)
{
	MYSQL mysql;
	MYSQL_RES * res;
	MYSQL_ROW row;
	
	int32_t * row_ids = NULL;
	
	struct hashtable * ht = create_hashtable(16, hash_from_string_fn, strings_equal_fn);
	
	if ((mysql_init(&mysql) == NULL)) { fprintf(stderr, "ERROR: mysql_init() error\n"); return 0; }
	if (!mysql_real_connect(&mysql, "localhost", "root", "", "mb", 0, NULL, 0))
	{
		fprintf(stderr, "ERROR: mysql_real_connect error (%s)\n", mysql_error(&mysql));
	}
	
	if (mysql_query(&mysql, "SELECT id, UPPER(TRIM(FULL_ADDRESS)) FROM addresses_oct2012 WHERE FULL_ADDRESS IS NOT NULL")==0)
	{
		res = mysql_store_result(&mysql);
		int num_rows = mysql_num_rows(res);
		
		row_ids = (int32_t*)malloc(sizeof(int32_t)*num_rows);
		//fprintf(stderr, "row_ids = %ld\n", (void*)row_ids);
		int count = 0;
		while ((row = mysql_fetch_row(res)))
		{
			int row_id = atoi(row[0]);
			row_ids[count++] = row_id;
			if (hashtable_search(ht, row[1]) == NULL)
			{
				int value_length = strlen(row[1]);
				char * temp = (char*)malloc(value_length+1);
				strncpy(temp, row[1], value_length);
				temp[value_length] = 0;
				//fprintf(stderr, "%s\n", temp);
				hashtable_insert(ht, temp, &row_ids[count-1]);
			}
		}
		
		mysql_free_result(res);
		
		fprintf(stderr, "%d vs %d\n", num_rows, hashtable_count(ht));
	}
	
	int count = 0;
	if (mysql_query(&mysql, "SELECT COUNT(*) FROM tickets")==0)
	{
		res = mysql_store_result(&mysql);
		row = mysql_fetch_row(res);
		count = atoi(row[0]);
		mysql_free_result(res);
	}
	
	int i;
	for (i = 0 ; i < count ; i+=10000)
	{
		char query[1000];
		sprintf(query, "SELECT id, address_id, location1, TRIM(location2) FROM tickets WHERE id > %d AND id <= %d", i, i+10000);
		fprintf(stderr, ".");
		if (mysql_query(&mysql, query)==0)
		{
			res = mysql_store_result(&mysql);
			int num_rows = mysql_num_rows(res);
			//fprintf(stderr, "num_rows = %d\n", num_rows);
			
			while ((row = mysql_fetch_row(res)))
			{
				if (row[3] == NULL) continue;
				char tempquery[200];
			
				void * ptr = hashtable_search(ht, row[3]);
				if (ptr != NULL)
					fprintf(stdout, "UPDATE tickets SET address_id = %d WHERE id = %s;\n", *(int32_t*)ptr, row[0]);
				else
					fprintf(stdout, "UPDATE tickets SET address_id = NULL WHERE id = %s;\n", row[0]);
			
				//mysql_query(&mysql, tempquery);
			}
		
			mysql_free_result(res);
		}
	}
}





















