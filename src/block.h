
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

#ifndef TUBES_BLOCK_H
#define TUBES_BLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

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

//extern char block_type_names[6][20];

#define TUBE_BLOCK_VERSION 3 // when this changes, ALL cached blocks need to be re-calculated

enum TYPE {
  TYPE_INT   = 1,
  TYPE_UINT  = 2,
  TYPE_FLOAT = 3,
  TYPE_CHAR  = 4
};

/* TODO
union CELL {
  int64_t   i;
  uint64_t ui;
  double    f;
};*/

#define SIZEOF_STRUCT_BLOCK (sizeof(int32_t)*8)

struct Block {
  // sizeof(struct Block) gives the size before the start of the data, total memory footprint is sizeof(struct Block) + all_bsize
  // float header
  // int32_t is_block
  uint32_t version;          // should always be equal to TUBE_BLOCK_VERSION
  uint32_t attributes_bsize; // num_attributes*sizeof(int32_t) + foreach_attribute(sizeof(int32_t)*3 + name_length + value_length)
  uint32_t columns_bsize;
  uint32_t data_bsize;
  uint32_t row_bsize;
  uint32_t num_attributes;
  uint32_t num_columns;
  uint32_t num_rows;
  /// int32_t attribute_offsets[num_attributes];
  /// [attributes]
  /// int32_t column_offsets[num_columns];  // where are the column meta data
  /// int32_t cell_offsets[num_columns];    // where in each row are the cells
  /// [columns]
  /// [rows]
};

struct Attribute {
  enum TYPE type;
  uint32_t name_length;
  uint32_t value_length;
  //char name[name_length];
  //char value[bsize];
  
};

char * attribute_get_name(struct Attribute * attribute);
void * attribute_get_value(struct Attribute * attribute);
void attribute_set_name(struct Attribute * attribute, const char * name);
void attribute_set_value(struct Attribute * attribute, void * value);

struct Column {
  enum TYPE type;
  uint32_t bsize;
  uint32_t name_length;
  //char name[name_length];
};

char * column_get_name(struct Column * column);
void column_set_name(struct Column * column, const char * name);

//////////////////////////////////////////////////////////////////////////////////////////

void setup_segfault_handling(char ** command);

struct Block * new_block();
struct Block * read_block(FILE * fp);
void write_block(FILE * fp, struct Block * block);
void free_block(struct Block * block);
struct Block * realloc_block(struct Block * block);

struct Attribute * get_attribute(struct Block * block, uint32_t attribute_id);
struct Block * _add_attribute(struct Block * block, enum TYPE type, uint32_t bsize, const char * name, void * value);
struct Block * add_int32_attribute(struct Block * block, const char * name, int32_t value);
struct Block * add_int64_attribute(struct Block * block, const char * name, int64_t value);
struct Block * add_float_attribute(struct Block * block, const char * name, float value);
struct Block * add_double_attribute(struct Block * block, const char * name, double value);
struct Block * add_string_attribute(struct Block * block, const char * name, const char * value);
int attribute_is_string(struct Attribute * attribute);

int32_t get_attribute_id_by_name(struct Block * block, const char * attribute_name);
struct Attribute * get_attribute_by_name(struct Block * block, const char * attribute_name);
int32_t get_attribute_value_as_int32(struct Block * block, const char * attribute_name);
double get_attribute_value_as_double(struct Block * block, const char * attribute_name);
const char * get_attribute_value_as_string(struct Block * block, const char * attribute_name);

int32_t * get_attribute_offsets(struct Block * block);
struct Attribute * get_first_attribute(struct Block * block);
struct Attribute * get_next_attribute(struct Block * block, struct Attribute * attribute);

struct Block * copy_all_attributes(struct Block * block, struct Block * src);

void fprintf_attribute_value(FILE * fp, struct Block * block, uint32_t attribute_id);

struct Block * add_xy_columns(struct Block * block);
struct Block * add_xyz_columns(struct Block * block);
struct Block * add_rgb_columns(struct Block * block);
struct Block * add_rgba_columns(struct Block * block);
void blank_column_values(struct Block * block, const char * column_name);

struct Column * get_column(struct Block * block, uint32_t column_id);
struct Block * _add_column(struct Block * block, enum TYPE type, uint32_t bsize, const char * name);

struct Block * add_int32_column(struct Block * block, const char * name);
struct Block * add_int64_column(struct Block * block, const char * name);
struct Block * add_float_column(struct Block * block, const char * name);
struct Block * add_double_column(struct Block * block, const char * name);
struct Block * add_string_column_with_length(struct Block * block, const char * name, uint32_t length);

struct Block * add_int32_column_and_blank(struct Block * block, const char * name);
struct Block * add_int64_column_and_blank(struct Block * block, const char * name);
struct Block * add_float_column_and_blank(struct Block * block, const char * name);
struct Block * add_double_column_and_blank(struct Block * block, const char * name);
struct Block * add_string_column_with_length_and_blank(struct Block * block, const char * name, uint32_t length);

//int column_is_string(struct Column * column);
int32_t get_column_id_by_name(struct Block * block, const char * column_name);
int32_t get_column_id_by_name_or_exit(struct Block * block, const char * column_name);
struct Column * get_column_by_name(struct Block * block, const char * column_name);

struct Block * column_string_set_length(struct Block * block, uint32_t column_id, int32_t length);

inline int32_t * get_column_offsets(const struct Block * block);
inline int32_t * get_cell_offsets(const struct Block * block);
inline struct Column * get_first_column(const struct Block * block);
inline struct Column * get_next_column(const struct Block * block, const struct Column * column);

struct Block * set_num_rows(struct Block * block, uint32_t num_rows);
void * get_row(struct Block * block, uint32_t row_id);
struct Block * add_row(struct Block * block);
struct Block * add_row_and_blank(struct Block * block);
struct Block * add_row_with_data(struct Block * block, int num_columns, ...);

struct Block * remove_row(struct Block * block, uint32_t row_id);

uint32_t get_row_bsize_from_columns(struct Block * block);

void * get_cell(struct Block * block, uint32_t row_id, uint32_t column_id);
int32_t get_cell_as_int32(struct Block * block, uint32_t row_id, uint32_t column_id);
int64_t get_cell_as_int64(struct Block * block, uint32_t row_id, uint32_t column_id);
float get_cell_as_float(struct Block * block, uint32_t row_id, uint32_t column_id);
double get_cell_as_double(struct Block * block, uint32_t row_id, uint32_t column_id);
char * get_cell_as_char(struct Block * block, uint32_t row_id, uint32_t column_id);

void set_cell(struct Block * block, uint32_t row_id, uint32_t column_id, void * data);
void set_cell_from_int32(struct Block * block, uint32_t row_id, uint32_t column_id, int32_t data);
void set_cell_from_double(struct Block * block, uint32_t row_id, uint32_t column_id, double data);
void set_cell_from_string(struct Block * block, uint32_t row_id, uint32_t column_id, char * data);

int32_t get_next_shape_start(struct Block * block, int32_t shape_start_id);
int32_t get_next_part_start(struct Block * block, int32_t part_start_id);
int32_t get_new_shape_row_id(struct Block * block);

double get_x(struct Block * block, uint32_t row_id);
double get_y(struct Block * block, uint32_t row_id);
double get_z(struct Block * block, uint32_t row_id);

void set_xy(struct Block * block, uint32_t row_id, double x, double y);
void set_xyz(struct Block * block, uint32_t row_id, double x, double y, double z);
void set_rgb(struct Block * block, uint32_t row_id, float r, float g, float b);
void set_rgba(struct Block * block, uint32_t row_id, float r, float g, float b, float a);

struct Block * copy_all_columns(struct Block * block, struct Block * src);
struct Block * append_block(struct Block * block, struct Block * src);

void fprintf_cell(FILE * fp, struct Block * block, uint32_t row_id, uint32_t column_id);

struct Block * add_command(struct Block * block, int argc, char ** argv);

void inspect_block(struct Block * block);

const char * get_type_name(enum TYPE type, uint32_t bsize);

struct Block * sort_block_using_int32_column(struct Block * block, int32_t column_id, char order);

#include "functions/functions.h"

#ifdef __cplusplus
}
#endif

#endif
