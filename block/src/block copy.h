
// header           float (inf)        // appears before every block
// is_block         int32 (1)
// attributes_bsize int32 (byte count)
// columns_bsize    int32 (byte count)
// data_bsize       int32 (row_bsize * num_rows)
// row_bsize        int32 (byte count)
// num_attributes   int32 (count)
// num_fields       int32 (count)      // each row has the values for the fields
// num_rows         int32 (count)
// [ attribute_offsets ]
// [ attributes ]
// [ field_offsets ]
// [ fields ]
// [ rows ]

/*
field type [int32]   // 1 for int, 2 for float, 3 and 4 are reserved, 5 and above are char array
field name length [int32]
field name char(length) - add length to count for a NULL terminator
*/

#ifndef HOOKAH_BLOCK_H
#define HOOKAH_BLOCK_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>    // for FILE
#include <stdlib.h>
#include <string.h>
#include <getopt.h>   // for getopt_long
#include <inttypes.h> // for int32_t
#include <signal.h>   // for segfault handling

extern int stdin_is_piped_t(float timeout);
extern void assert_stdin_is_piped_t(float timeout);
extern void assert_stdin_is_piped();
extern int stdout_is_piped();
extern void assert_stdout_is_piped();
extern void assert_stdin_or_out_is_piped();

extern char block_type_names[6][20];

#define INT_TYPE 1
#define LONG_TYPE 2
#define FLOAT_TYPE 3
#define DOUBLE_TYPE 4
#define STRING_TYPE 5 // only for attributes
// column type for a string is just it's length, so 5 or 10 or whatever is valid, minimum of 5

#define SIZEOF_STRUCT_BLOCK (sizeof(int32_t)*7)

struct Block {
  // sizeof(struct Block) gives the size before the start of the data, total memory footprint is sizeof(struct Block) + all_bsize
  // float header
  // int32_t is_block
  int32_t attributes_bsize; // num_attributes*sizeof(int32_t) + foreach_attribute(sizeof(int32_t)*3 + name_length + value_length)
  int32_t columns_bsize;
  int32_t data_bsize;
  int32_t row_bsize;
  int32_t num_attributes;
  int32_t num_columns;
  int32_t num_rows;
  /// int32_t attribute_offsets[num_attributes];
  /// [attributes]
  /// int32_t column_offsets[num_columns];
  /// int32_t cell_offsets[num_columns];
  /// [columns]
  /// [rows]
};

struct Attribute {
  int32_t type;
  int32_t name_length;
  int32_t value_length;
  //char name[name_length];
  //char value[value_length];
  
};

char * attribute_get_name(struct Attribute * attribute);
char * attribute_get_value(struct Attribute * attribute);
void attribute_set_name(struct Attribute * attribute, char * name);
void attribute_set_value(struct Attribute * attribute, char * value);

struct Column {
  int32_t type;
  int32_t name_length;
  //char name[name_length];
};

char * column_get_name(struct Column * column);
void column_set_name(struct Column * column, char * name);

void setup_segfault_handling(char ** command);

struct Block * new_block();
struct Block * read_block(FILE * fp);
void write_block(FILE * fp, struct Block * block);
void free_block(struct Block * block);

struct Attribute * get_attribute(struct Block * block, int32_t attribute_id);
void _add_attribute(struct Block ** pBlock, int32_t type, char * name, void * value);
void add_int_attribute(struct Block ** pBlock, char * name, int32_t value);
void add_long_attribute(struct Block ** pBlock, char * name, long value);
void add_float_attribute(struct Block ** pBlock, char * name, float value);
void add_double_attribute(struct Block ** pBlock, char * name, double value);
void add_string_attribute(struct Block ** pBlock, char * name, char * value);

struct Column * get_column(struct Block * block, int32_t column_id);
int32_t find_column_id_by_name(struct Block * block, char * column_name);
void _add_column(struct Block ** pBlock, int32_t type, char * name);
void add_int_column(struct Block ** pBlock, char * name);
void add_long_column(struct Block ** pBlock, char * name);
void add_float_column(struct Block ** pBlock, char * name);
void add_double_column(struct Block ** pBlock, char * name);
void add_string_column_with_length(struct Block ** pBlock, char * name, int32_t length);

void set_num_rows(struct Block ** pBlock, int32_t num_rows);
void * get_row(struct Block * block, int32_t row_id);
void add_row(struct Block ** pBlock);
void add_row_with_data(struct Block ** pBlock, int num_columns, ...);

int32_t get_row_bsize_from_columns(struct Block * block);

void * get_cell(struct Block * block, int32_t row_id, int32_t column_id);
void set_cell(struct Block * block, int32_t row_id, int32_t column_id, void * data);

struct Block * add_command(struct Block * block, int argc, char ** argv);

void inspect_block(struct Block * block);

char * get_type_name(int32_t type);

#endif
