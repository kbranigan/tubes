
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <signal.h>
#include <execinfo.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>

#include "block.h"
#include "std_helpers.h" // includes C source actually

static char ** running_command = NULL;
void block_segfault_handler(int sig)
{
  fprintf(stderr, "\nSIGSEGV (segfault) in : ");
  if (running_command != NULL)
  {
    int i = 0;
    while (running_command[i] != NULL)
      fprintf(stderr, "%s ", running_command[i++]);
  }
  fprintf(stderr, "\n");
  
  void * array[32];
  size_t size = backtrace(array, 32);
  
  char ** strings = backtrace_symbols(array, size);
  
  int i;
  for (i = 0 ; i < size ; i++) fprintf(stderr, "%s\n", strings[i]);
  
  fprintf(stderr, "\n\n");
  exit(EXIT_FAILURE);
}

void setup_segfault_handling(char ** command)
{
  running_command = command;
  signal(SIGSEGV, block_segfault_handler);
}

char block_type_names[6][20] = {
  "unknown", "int", "long", "float", "double", "string"
};

size_t block_header_size()
{
  return sizeof(int32_t)*7;
}

struct Block * new_block()
{
  if (sizeof(struct Block) != block_header_size()) { fprintf(stderr, "sizeof(struct Block) is the wrong size (%ld) - should be %ld, perhaps padding or memory alignment works differently for your machine?\n", sizeof(struct Block), block_header_size()); exit(1); }
  
  struct Block * block = (struct Block*)malloc(sizeof(struct Block));
  memset(block, 0, sizeof(struct Block));
  return block;
}

struct Block * realloc_block(struct Block * block)
{
  return (struct Block *)realloc(block, sizeof(struct Block) + block->attributes_bsize + block->columns_bsize + block->data_bsize);
}

struct Block * set_num_rows(struct Block * block, int32_t num_rows)
{
  block->num_rows = num_rows;
  block->data_bsize = block->row_bsize * block->num_rows;
  block = realloc_block(block);
  return block;
}

struct Block * read_block(FILE * fp)
{
  if (fp == NULL) { fprintf(stderr, "trying to read from NULL file pointer\n"); return 0; }
  
  float header;
  if (fread(&header, sizeof(header), 1, fp) != 1) return NULL;
  if (isfinite(header)) { fprintf(stderr, "header (%f) is finite (it's suppose to be infinite, either no data or invalid data was read)\n", header); return NULL; }
  
  int32_t is_block;
  if (fread(&is_block, sizeof(is_block), 1, fp) != 1) { fprintf(stderr, "fread block is_block failed.\n"); return NULL; }
  if (is_block != 1) { fprintf(stderr, "is_block != 1\n"); return NULL; }
  
  struct Block * block = (struct Block*)malloc(sizeof(struct Block));
  if (fread(block, sizeof(struct Block), 1, fp) != 1) { fprintf(stderr, "fread block failed (error 1)\n"); return NULL; }
  block = realloc_block(block);
  
  if (fread(&block[1], block->attributes_bsize + block->columns_bsize + block->data_bsize, 1, fp) != 1) { fprintf(stderr, "fread block failed (error 2)\n"); return NULL; }
  
  return block;
}

void write_block(FILE * fp, struct Block * block)
{
  if (block == NULL) { fprintf(stderr, "write_block passed null block.\n"); return; }
  
  float inf = INFINITY;
  if (fwrite(&inf, sizeof(inf), 1, fp) != 1) { fprintf(stderr, "fwrite block header failed.\n"); return; }
  
  int32_t is_block = 1;
  if (fwrite(&is_block, sizeof(is_block), 1, fp) != 1) { fprintf(stderr, "fwrite block is_block failed.\n"); return; }
  if (fwrite(block, sizeof(struct Block) + block->attributes_bsize + block->columns_bsize + block->data_bsize, 1, fp) != 1) { fprintf(stderr, "fwrite block failed.\n"); return; }
}

void free_block(struct Block * block)
{
  free(block);
}

struct Block * add_command(struct Block * block, int argc, char ** argv)
{
  if (block == NULL || argc == 0 || argv == NULL) return block;
  
  int length = 100;
  char * full_command = malloc(length);
  sprintf(full_command, "");
  int i = 0;
  for (i = 0 ; i < argc ; i++)
  {
    if (strlen(full_command) + strlen(argv[i]) + 2 > length) {
      length += 100;
      full_command = realloc(full_command, length);
    }
    strcat(full_command, argv[i]);
    if (i < argc-1) strcat(full_command, " ");
  }
  block = add_string_attribute(block, "command", full_command);
  free(full_command);
  return block;
}

int32_t memory_pad(int32_t i)
{
  return ceil(i / 4.0) * 4.0;
}

int32_t get_type_size(int32_t type)
{
  if (type == INT_TYPE)         return sizeof(int32_t);
  else if (type == LONG_TYPE)   return sizeof(long);
  else if (type == FLOAT_TYPE)  return sizeof(float);
  else if (type == DOUBLE_TYPE) return sizeof(double);
  else if (type >= 5)           return memory_pad(type);
  else fprintf(stderr, "column_get_cell_size invalid type '%d'\n", type);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t * get_attribute_offsets(struct Block * block) {
  return (int32_t*)((char*)block + sizeof(struct Block));
}

struct Attribute * get_first_attribute(struct Block * block) {
  return (block == NULL) ? NULL : (struct Attribute *)((char*)block + sizeof(struct Block) + sizeof(int32_t)*block->num_attributes);
}

struct Attribute * get_next_attribute(struct Block * block, struct Attribute * attribute) {
  return (block == NULL || attribute == NULL) ? NULL : (struct Attribute *)((char*)attribute + sizeof(struct Attribute) + attribute->name_length + attribute->value_length);
}

struct Attribute * get_attribute(struct Block * block, int32_t attribute_id)
{
  if (block == NULL || attribute_id < 0 || attribute_id > block->num_attributes)
  {
    fprintf(stderr, "get_attribute(%ld, %d) FAILED\n", (long int)block, attribute_id);
    return NULL;
  }
  
  int32_t * attribute_offsets = get_attribute_offsets(block);
  return (struct Attribute*)((char*)block + sizeof(struct Block) + attribute_offsets[attribute_id]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline int32_t * get_column_offsets(const struct Block * block) {
  return (int32_t*)((char*)block + sizeof(struct Block) + block->attributes_bsize);
}

inline int32_t * get_cell_offsets(const struct Block * block) {
  return (int32_t*)((char*)block + sizeof(struct Block) + block->attributes_bsize + sizeof(int32_t)*block->num_columns);
}

inline struct Column * get_first_column(const struct Block * block) {
  return (block == NULL) ? NULL : (struct Column *)((char*)block + sizeof(struct Block) + block->attributes_bsize + (sizeof(int32_t)+sizeof(int32_t))*block->num_columns);
}

inline struct Column * get_next_column(const struct Block * block, const struct Column * column) {
  return (block == NULL || column == NULL) ? NULL : (struct Column *)((char*)column + sizeof(struct Column) + column->name_length);
}

struct Column * get_column(struct Block * block, int32_t column_id)
{
  if (block == NULL || column_id < 0 || column_id > block->num_columns)
  {
    fprintf(stderr, "get_column(%ld, %d) FAILED\n", (long int)block, column_id);
    return NULL;
  }
  
  int32_t * column_offsets = get_column_offsets(block);
  return (struct Column*)((char*)block + sizeof(struct Block) + block->attributes_bsize + column_offsets[column_id]);
  
  int i = 0;
  struct Column * column = get_first_column(block);
  for (i = 0 ; i != column_id ; i++)
    column = get_next_column(block, column);
  
  return column;
}

int column_is_string(struct Column * column)
{
  return (column->type >= 5);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void * get_row(struct Block * block, int32_t row_id)
{
  return (void*)&block[1] + block->attributes_bsize + block->columns_bsize + block->row_bsize * row_id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t get_row_bsize_from_columns(struct Block * block)
{
  int32_t single_row_bsize = 0;
  int32_t column_id;
  for (column_id = 0 ; column_id < block->num_columns ; column_id++)
  {
    struct Column * column = get_column(block, column_id);
    single_row_bsize += get_type_size(column->type);
  }
  
  return single_row_bsize;
}

int32_t find_column_id_by_name(struct Block * block, char * column_name)
{
  int32_t column_id;
  for (column_id = 0 ; column_id < block->num_columns ; column_id++)
  {
    struct Column * column = get_column(block, column_id);
    if (strcmp(column_name, column_get_name(column))==0) return column_id;
  }
  return -1;
}

int32_t find_attribute_id_by_name(struct Block * block, char * attribute_name)
{
  int32_t attribute_id;
  for (attribute_id = 0 ; attribute_id < block->num_columns ; attribute_id++)
  {
    struct Attribute * attribute = get_attribute(block, attribute_id);
    if (strcmp(attribute_name, attribute_get_name(attribute))==0) return attribute_id;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

char * attribute_get_name(struct Attribute * attribute)  { return (char*)attribute + sizeof(struct Attribute); }
char * attribute_get_value(struct Attribute * attribute) { return (char*)attribute + sizeof(struct Attribute) + attribute->name_length; }
void attribute_set_name(struct Attribute * attribute, char * name)   { strncpy((char*)attribute + sizeof(struct Attribute), name, attribute->name_length); }
void attribute_set_value(struct Attribute * attribute, char * value) { strncpy((char*)attribute + sizeof(struct Attribute) + attribute->name_length, value, attribute->value_length); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Block * _add_attribute(struct Block * block, int32_t type, char * name, void * value)
{
  if (block == NULL) { fprintf(stderr, "add_attribute called on a NULL block\n"); return NULL; }
  
  int32_t name_length = memory_pad(strlen(name)+1);
  int32_t value_length = type==STRING_TYPE ? memory_pad(strlen(value)+1) : get_type_size(type);
  
  int32_t old_attributes_bsize = block->attributes_bsize;
  int32_t single_attribute_bsize = sizeof(struct Attribute) + name_length + value_length;
  
  block->attributes_bsize += 
              single_attribute_bsize + 
              sizeof(int32_t); // attributes_offsets
  block->num_attributes ++;
  block = realloc_block(block);
  
  // move content after attributes, making room for the additional attribute
  if (block->num_columns > 0 || block->num_rows > 0)
  {
    memmove((char*)block + sizeof(struct Block) + old_attributes_bsize + single_attribute_bsize + sizeof(int32_t),
            (char*)block + sizeof(struct Block) + old_attributes_bsize,
            block->columns_bsize + block->data_bsize);
  }
  
  // shift attributes over so there is room for the attribute_offset
  if (old_attributes_bsize > 0)
  {
    memmove((char*)block + sizeof(struct Block) + sizeof(int32_t),
            (char*)block + sizeof(struct Block),
            old_attributes_bsize);
  }
  
  int32_t * attribute_offsets = get_attribute_offsets(block);
  
  int i;
  struct Attribute * attribute = get_first_attribute(block);
  for (i = 0 ; i < block->num_attributes - 1 ; i++)
  {
    attribute_offsets[i] = (int32_t)((char*)attribute - (char*)block - sizeof(struct Block));
    attribute = get_next_attribute(block, attribute);
  }
  
  attribute->type = type;
  attribute->name_length = name_length;
  attribute->value_length = value_length;
  attribute_set_name(attribute, name);
  attribute_set_value(attribute, value);
  attribute_offsets[block->num_attributes - 1] = (int32_t)((char*)attribute - (char*)block - sizeof(struct Block));
  
  return block;
}

struct Block * add_int_attribute(struct Block * block, char * name, int32_t value)   { return _add_attribute(block, INT_TYPE, name, &value); }
struct Block * add_long_attribute(struct Block * block, char * name, long value)     { return _add_attribute(block, LONG_TYPE, name, &value); }
struct Block * add_float_attribute(struct Block * block, char * name, float value)   { return _add_attribute(block, FLOAT_TYPE, name, &value); }
struct Block * add_double_attribute(struct Block * block, char * name, double value) { return _add_attribute(block, DOUBLE_TYPE, name, &value); }
struct Block * add_string_attribute(struct Block * block, char * name, char * value) { return _add_attribute(block, STRING_TYPE, name, value); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

char * column_get_name(struct Column * column)  { return (char*)column + sizeof(struct Column); }
void column_set_name(struct Column * column, char * name) { strncpy(column_get_name(column), name, column->name_length); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Block * _add_column(struct Block * block, int32_t type, char * name)
{
  if (block == NULL) { fprintf(stderr, "add_field called on a NULL block\n"); return NULL; }
  
  int32_t name_length = memory_pad(strlen(name)+1);
  int32_t old_columns_bsize = block->columns_bsize;
  int32_t old_data_bsize = block->data_bsize;
  int32_t old_row_bsize = block->row_bsize;
  
  block->row_bsize += get_type_size(type);
  
  int32_t single_column_bsize = sizeof(struct Column) + name_length;
  
  block->columns_bsize += 
              single_column_bsize + 
              sizeof(int32_t) + // column_offsets
              sizeof(int32_t); // cell_offsets
  block->num_columns++;
  block = set_num_rows(block, block->num_rows);
  block = realloc_block(block);
  
  // kbfu - interpolation of all rows of data, not easy
  if (block->num_rows > 0)
  {
    int row_id;
    for (row_id = block->num_rows - 1 ; row_id >= 0 ; row_id--)
    {
      memmove(
        (char*)block + sizeof(struct Block) + block->attributes_bsize + block->columns_bsize + block->row_bsize*row_id,
        (char*)block + sizeof(struct Block) + block->attributes_bsize + old_columns_bsize + old_row_bsize*row_id,
        old_row_bsize);
    }
  }
  
  // shift attributes over so there is room for the attribute_offsets and cell_offsets
  if (old_columns_bsize > 0)
  {
    memmove((char*)block + sizeof(struct Block) + block->attributes_bsize + sizeof(int32_t) + sizeof(int32_t),
            (char*)block + sizeof(struct Block) + block->attributes_bsize,
            old_columns_bsize);
  }
  
  int32_t * column_offsets = get_column_offsets(block);
  int32_t * cell_offsets = get_cell_offsets(block);
  cell_offsets[0] = 0;
  
  int o = 0;
  int i;
  struct Column * column = get_first_column(block);
  for (i = 0 ; i < block->num_columns - 1 ; i++)
  {
    cell_offsets[i] = o;
    o += get_type_size(column->type);
    column_offsets[i] = (int32_t)((char*)column - (char*)block - sizeof(struct Block) - block->attributes_bsize);
    column = get_next_column(block, column);
  }
  
  column->type = type;
  column->name_length = name_length;
  column_set_name(column, name);
  column_offsets[block->num_columns - 1] = (int32_t)((char*)column - (char*)block - sizeof(struct Block) - block->attributes_bsize);
  if (block->num_columns != 1) cell_offsets[block->num_columns - 1] = o;
  
  return block;
}

struct Block * add_int_column(struct Block * block, char * name)    { return _add_column(block, INT_TYPE, name); }
struct Block * add_long_column(struct Block * block, char * name)   { return _add_column(block, LONG_TYPE, name); }
struct Block * add_float_column(struct Block * block, char * name)  { return _add_column(block, FLOAT_TYPE, name); }
struct Block * add_double_column(struct Block * block, char * name) { return _add_column(block, DOUBLE_TYPE, name); }
struct Block * add_string_column_with_length(struct Block * block, char * name, int32_t length) { if (length < 5) length = 5; return _add_column(block, length, name); }

struct Block * add_xy_columns(struct Block * block)
{
  if (find_column_id_by_name(block, "x") == -1) block = add_float_column(block, "x");
  if (find_column_id_by_name(block, "y") == -1) block = add_float_column(block, "y");
  return block;
}

struct Block * add_xyz_columns(struct Block * block)
{
  block = add_xy_columns(block);
  if (find_column_id_by_name(block, "z") == -1) block = add_float_column(block, "z");
  return block;
}

struct Block * add_rgb_columns(struct Block * block)
{
  if (find_column_id_by_name(block, "red") == -1)   block = add_float_column(block, "red");
  if (find_column_id_by_name(block, "green") == -1) block = add_float_column(block, "green");
  if (find_column_id_by_name(block, "blue") == -1)  block = add_float_column(block, "blue");
  return block;
}

struct Block * add_rgba_columns(struct Block * block)
{
  block = add_rgb_columns(block);
  if (find_column_id_by_name(block, "alpha") == -1) block = add_float_column(block, "alpha");
  return block;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void * get_cell(struct Block * block, int32_t row_id, int32_t column_id)
{
  int32_t * cell_offsets = get_cell_offsets(block);
  return (char*)get_row(block, row_id) + cell_offsets[column_id];
}

int get_cell_as_int(struct Block * block, int32_t row_id, int32_t column_id)
{
  void * cell = get_cell(block, row_id, column_id);
  struct Column * column = get_column(block, column_id);
  
  int temp = 0;
  
  if (column->type == INT_TYPE) temp = *(int32_t*)cell;
  else if (column->type == LONG_TYPE) temp = *(long*)cell;
  else if (column->type == FLOAT_TYPE) temp = *(float*)cell;
  else if (column->type == DOUBLE_TYPE) temp = *(double*)cell;
  else if (column_is_string(column)) temp = atoi((char*)cell);
  
  return temp;
}

long get_cell_as_long(struct Block * block, int32_t row_id, int32_t column_id)
{
  void * cell = get_cell(block, row_id, column_id);
  struct Column * column = get_column(block, column_id);
  
  long temp = 0;
  
  if (column->type == INT_TYPE) temp = *(int32_t*)cell;
  else if (column->type == LONG_TYPE) temp = *(long*)cell;
  else if (column->type == FLOAT_TYPE) temp = *(float*)cell;
  else if (column->type == DOUBLE_TYPE) temp = *(double*)cell;
  else if (column_is_string(column)) temp = atol((char*)cell);
  
  return temp;
}

float get_cell_as_float(struct Block * block, int32_t row_id, int32_t column_id)
{
  void * cell = get_cell(block, row_id, column_id);
  struct Column * column = get_column(block, column_id);
  
  float temp = 0;
  
  if (column->type == INT_TYPE) temp = *(int32_t*)cell;
  else if (column->type == LONG_TYPE) temp = *(long*)cell;
  else if (column->type == FLOAT_TYPE) temp = *(float*)cell;
  else if (column->type == DOUBLE_TYPE) temp = *(double*)cell;
  else if (column_is_string(column)) temp = atof((char*)cell);
  
  return temp;
}

double get_cell_as_double(struct Block * block, int32_t row_id, int32_t column_id)
{
  void * cell = get_cell(block, row_id, column_id);
  struct Column * column = get_column(block, column_id);
  
  double temp = 0;
  
  if (column->type == INT_TYPE) temp = *(int32_t*)cell;
  else if (column->type == LONG_TYPE) temp = *(long*)cell;
  else if (column->type == FLOAT_TYPE) temp = *(float*)cell;
  else if (column->type == DOUBLE_TYPE) temp = *(double*)cell;
  else if (column_is_string(column)) temp = atof((char*)cell);
  
  return temp;
}

void set_cell(struct Block * block, int row_id, int column_id, void * value)
{
  if (block == NULL) { fprintf(stderr, "set_cell called with null block\n"); exit(0); }
  if (row_id >= block->num_rows) { fprintf(stderr, "set_cell called with row_id >= block->num_rows\n"); exit(0); }
  if (column_id >= block->num_columns) { fprintf(stderr, "set_cell called with column_id(%d) >= block->num_columns(%d)\n", column_id, block->num_columns); exit(0); }
  
  struct Column * column = get_column(block, column_id);
  void * cell = get_cell(block, row_id, column_id);
  memcpy(cell, value, get_type_size(column->type));
}

void set_cell_from_string(struct Block * block, int32_t row_id, int32_t column_id, char * data)
{
  if (block == NULL) { fprintf(stderr, "set_cell_from_string called with null block\n"); exit(0); }
  if (row_id >= block->num_rows) { fprintf(stderr, "set_cell_from_string called with row_id >= block->num_rows\n"); exit(0); }
  if (column_id >= block->num_columns) { fprintf(stderr, "set_cell_from_string called with column_id(%d) >= block->num_columns(%d)\n", column_id, block->num_columns); exit(0); }
  
  struct Column * column = get_column(block, column_id);
  void * cell = get_cell(block, row_id, column_id);
  if (column_is_string(column)) strncpy((char*)cell, data, column->type);
  else if (column->type == INT_TYPE) *(int*)cell = atoi(data);
  else if (column->type == LONG_TYPE) *(long*)cell = atol(data);
  else if (column->type == FLOAT_TYPE) *(float*)cell = atof(data);
  else if (column->type == DOUBLE_TYPE) *(double*)cell = atof(data);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Block * cached_xyz_block = NULL;
struct Block * cached_rgba_block = NULL;
int32_t cached_xyz_column_ids[3] = {0,0,0};
int32_t cached_rgba_column_ids[4] = {0,0,0,0};

void set_xy(struct Block * block, int32_t row_id, float x, float y)
{
  if (row_id > block->num_rows) { fprintf(stderr, "set_xy invalid row_id\n"); exit(0); }
  if (cached_xyz_block != block)
  {
    cached_xyz_column_ids[0] = find_column_id_by_name(block, "x");
    cached_xyz_column_ids[1] = find_column_id_by_name(block, "y");
    cached_xyz_column_ids[2] = find_column_id_by_name(block, "z");
    cached_xyz_block = block;
  }
  if (cached_xyz_column_ids[0] != -1) set_cell(block, row_id, cached_xyz_column_ids[0], &x);
  if (cached_xyz_column_ids[1] != -1) set_cell(block, row_id, cached_xyz_column_ids[1], &y);
}

void set_xyz(struct Block * block, int32_t row_id, float x, float y, float z)
{
  set_xy(block, row_id, x, y);
  if (cached_xyz_column_ids[2] != -1) set_cell(block, row_id, cached_xyz_column_ids[2], &z);
}

void set_rgb(struct Block * block, int32_t row_id, float r, float g, float b)
{
  if (row_id > block->num_rows) { fprintf(stderr, "set_rgb invalid row_id\n"); exit(0); }
  if (cached_rgba_block != block)
  {
    cached_rgba_column_ids[0] = find_column_id_by_name(block, "red");
    cached_rgba_column_ids[1] = find_column_id_by_name(block, "green");
    cached_rgba_column_ids[2] = find_column_id_by_name(block, "blue");
    cached_rgba_column_ids[3] = find_column_id_by_name(block, "alpha");
    cached_rgba_block = block;
  }
  if (cached_rgba_column_ids[0] != -1) set_cell(block, row_id, cached_rgba_column_ids[0], &r);
  if (cached_rgba_column_ids[1] != -1) set_cell(block, row_id, cached_rgba_column_ids[1], &g);
  if (cached_rgba_column_ids[2] != -1) set_cell(block, row_id, cached_rgba_column_ids[2], &b);
}

void set_rgba(struct Block * block, int32_t row_id, float r, float g, float b, float a)
{
  set_rgb(block, row_id, r, g, b);
  if (cached_rgba_column_ids[3] != -1) set_cell(block, row_id, cached_rgba_column_ids[3], &a);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Block * add_row(struct Block * block)
{
  if (block == NULL) { fprintf(stderr, "add_row called on a NULL block\n"); return NULL; }
  
  block = set_num_rows(block, block->num_rows + 1);
  
  return block;
}

struct Block * add_row_with_data(struct Block * block, int num_columns, ...)
{
  if (block == NULL) { fprintf(stderr, "add_row_with_data called on a NULL block\n"); return NULL; }
  if (block->num_columns != num_columns) { fprintf(stderr, "block num_columns not the same as provided num_columns\n"); return block; }
  
  block = add_row(block);
  
  int row_id = block->num_rows - 1;
  int column_id;
  va_list v1;
  va_start(v1, num_columns);
  for (column_id = 0 ; column_id < block->num_columns ; column_id++)
  {
    struct Column * column = get_column(block, column_id);
    if (column->type == INT_TYPE)         { int32_t value = va_arg(v1, int);          set_cell(block, row_id, column_id, &value); }
    else if (column->type == LONG_TYPE)   { long value = va_arg(v1, long);            set_cell(block, row_id, column_id, &value); }
    else if (column->type == FLOAT_TYPE)  { float value = (float)va_arg(v1, double);  set_cell(block, row_id, column_id, &value); }
    else if (column->type == DOUBLE_TYPE) { double value = va_arg(v1, double);        set_cell(block, row_id, column_id, &value); }
    else if (column_is_string(column))    { char * value = va_arg(v1, char *);        set_cell(block, row_id, column_id, value); }
  }
  va_end(v1);
  
  return block;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void inspect_block(struct Block * block)
{
  if (block == NULL) { fprintf(stderr, "inspect_block called on a NULL block\n"); return; }
  fprintf(stderr, "\nblock (%d+%d+%d=%d total size in bytes - with %ld byte header)\n", block->attributes_bsize, block->columns_bsize, block->data_bsize, block->attributes_bsize + block->columns_bsize + block->data_bsize, block_header_size());
  
  if (block->num_attributes == 0)
    fprintf(stderr, "     ->no_attributes\n");
  else
  {
    fprintf(stderr, "     ->attributes = [\n");
    int32_t * offset = (int32_t*)((char*)block + sizeof(struct Block));
    
    int attribute_id;
    for (attribute_id = 0 ; attribute_id < block->num_attributes ;  attribute_id++)
    {
      struct Attribute * attribute = get_attribute(block, attribute_id);
      char * name = attribute_get_name(attribute);
      char * value = attribute_get_value(attribute);
      if (attribute->type == INT_TYPE)         fprintf(stderr, "       [%d][%7s] \"%s\" = %d\n", attribute_id, get_type_name(attribute->type), name, *(int32_t*)value);
      else if (attribute->type == LONG_TYPE)   fprintf(stderr, "       [%d][%7s] \"%s\" = %ld\n", attribute_id, get_type_name(attribute->type), name, *(long*)value);
      else if (attribute->type == FLOAT_TYPE)  fprintf(stderr, "       [%d][%7s] \"%s\" = %f\n", attribute_id, get_type_name(attribute->type), name, *(float*)value);
      else if (attribute->type == DOUBLE_TYPE) fprintf(stderr, "       [%d][%7s] \"%s\" = %lf\n", attribute_id, get_type_name(attribute->type), name, *(double*)value);
      else if (attribute->type == STRING_TYPE) fprintf(stderr, "       [%d][%7s] \"%s\" = \"%s\"\n", attribute_id, get_type_name(attribute->type), name, (char*)value);
      else fprintf(stderr, "       [%d] has invalid attribute->type (%x)\n", attribute_id, attribute->type);
    }
    fprintf(stderr, "     ]\n");
  }
  
  if (block->num_columns == 0)
    fprintf(stderr, "     ->no_fields\n");
  else
  {
    fprintf(stderr, "     ->columns = [\n");
    int column_id;
    for (column_id = 0 ; column_id < block->num_columns ; column_id++)
    {
      struct Column * column = get_column(block, column_id);
      fprintf(stderr, "       [%d][%7s] = \"%s\"\n", column_id, get_type_name(column->type), column_get_name(column));
    }
    fprintf(stderr, "     ]\n");
  }
  
  if (block->num_rows == 0)
    fprintf(stderr, "     ->no_data\n");
  else
  {
    fprintf(stderr, "     ->data = [\n");
    int row_id;
    for (row_id = 0 ; row_id < block->num_rows ; row_id++)
    {
      fprintf(stderr, "       [%d] = { ", row_id);
      
      int column_id;
      for (column_id = 0 ; column_id < block->num_columns ; column_id++)
      {
        struct Column * column = get_column(block, column_id);
        if (column == NULL) continue;
        void * cell = get_cell(block, row_id, column_id);
        if (column_id != 0) fprintf(stderr, ", ");
        
        if (cell == NULL) fprintf(stderr, "NULL");
        else if (column->type == INT_TYPE)    fprintf(stderr, "%d", *(int32_t*)cell);
        else if (column->type == LONG_TYPE)   fprintf(stderr, "%ld", *(long*)cell);
        else if (column->type == FLOAT_TYPE)  fprintf(stderr, "%f", *(float*)cell);
        else if (column->type == DOUBLE_TYPE) fprintf(stderr, "%lf", *(double*)cell);
        else if (column_is_string(column))    fprintf(stderr, "\"%s\"", (char*)cell);
      }
      
      if (row_id >= 25 && row_id < block->num_rows - 5) { fprintf(stderr, " }\n       ....\n"); row_id = block->num_rows-5; continue; }
      fprintf(stderr, " }\n");
    }
    fprintf(stderr, "     ]\n");
  }
  fprintf(stderr, "\n");
}

char * get_type_name(int32_t type)
{
  if (type >= 0 && type < sizeof(block_type_names) / sizeof(block_type_names[0]))
    return block_type_names[type];
  else
  {
    static char get_type_name_temp[100] = "";
    sprintf(get_type_name_temp, "%d", type);
    return get_type_name_temp;
  }
}

