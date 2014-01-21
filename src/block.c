
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

struct CachedColumnIds {
  int32_t xyz[3];
  int32_t rgba[4];
  int32_t shape_row_id;
  int32_t shape_part_id;
  int32_t shape_part_type;
};

struct Block * cached_block_ptr = NULL;
struct CachedColumnIds cached_block_column_ids = { {0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0 };

void update_cached_block_column_ids(struct Block * block)
{
  cached_block_ptr = block;
  cached_block_column_ids.xyz[0] = get_column_id_by_name(cached_block_ptr, "x");
  cached_block_column_ids.xyz[1] = get_column_id_by_name(cached_block_ptr, "y");
  cached_block_column_ids.xyz[2] = get_column_id_by_name(cached_block_ptr, "z");

  cached_block_column_ids.rgba[0] = get_column_id_by_name(cached_block_ptr, "red");
  cached_block_column_ids.rgba[1] = get_column_id_by_name(cached_block_ptr, "green");
  cached_block_column_ids.rgba[2] = get_column_id_by_name(cached_block_ptr, "blue");
  cached_block_column_ids.rgba[3] = get_column_id_by_name(cached_block_ptr, "alpha");

  cached_block_column_ids.shape_row_id = get_column_id_by_name(cached_block_ptr, "shape_row_id");
  cached_block_column_ids.shape_part_id = get_column_id_by_name(cached_block_ptr, "shape_part_id");
  cached_block_column_ids.shape_part_type = get_column_id_by_name(cached_block_ptr, "shape_part_type");
}

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

struct Block * new_block() {
	if (sizeof(struct Block) != SIZEOF_STRUCT_BLOCK) {
		fprintf(stderr, "ERROR %s: sizeof(struct Block) is the wrong size (%d) - should be %d, perhaps padding or memory alignment works differently for your machine?\n", __func__, (int)sizeof(struct Block), (int)SIZEOF_STRUCT_BLOCK);
		exit(1);
	}
	
	struct Block * block = (struct Block*)malloc(sizeof(struct Block));
	if (block == NULL) {
		fprintf(stderr, "ERROR: %s, malloc failed\n", __func__);
		exit(1);
	}
	memset(block, 0, sizeof(struct Block));
	block->version = TUBE_BLOCK_VERSION;
	return block;
}

struct Block * new_block_from_row_bitmask(struct Block * block, uint32_t * row_bitmask) {
	if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return NULL; }
	if (row_bitmask == NULL) { fprintf(stderr, "%s called with a NULL row_bitmask\n", __func__); return NULL; }
	
	struct Block * temp = new_block();
	temp = copy_all_attributes(temp, block); 
	temp = copy_all_columns(temp, block); 
	
	int i, j = 0;
	for (i = 0 ; i < block->num_rows ; i++) {
		if (row_bitmask[i] == 1) {
			j++;
		}
	}
	
	temp = set_num_rows(temp, j);
	
	j = 0;
	for (i = 0 ; i < block->num_rows ; i++) {
		if (row_bitmask[i] == 1) {
			memcpy(get_row(temp, j), get_row(block, i), block->row_bsize);
			j++;
		}
	}
	return temp;
}

struct Block * realloc_block(struct Block * block)
{
  return (struct Block *)realloc(block, sizeof(struct Block) + block->attributes_bsize + block->columns_bsize + block->data_bsize);
}

struct Block * set_num_rows(struct Block * block, uint32_t num_rows)
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
  
  uint32_t is_block;
  if (fread(&is_block, sizeof(is_block), 1, fp) != 1) { fprintf(stderr, "fread block is_block failed.\n"); return NULL; }
  if (is_block != 1) { fprintf(stderr, "is_block != 1\n"); return NULL; }
  
  //int32_t block_version;
  //if (fread(&block_version, sizeof(block_version), 1, fp) != 1) { fprintf(stderr, "fread block block_version failed.\n"); return NULL; }
  
  struct Block * block = (struct Block*)malloc(sizeof(struct Block));
  if (fread(block, sizeof(struct Block), 1, fp) != 1) { fprintf(stderr, "fread block failed (error 1)\n"); return NULL; }
  if (block->version != TUBE_BLOCK_VERSION) { fprintf(stderr, "fread block is %d (compiled source is for version %d)\n", block->version, TUBE_BLOCK_VERSION); return NULL; }
  block = realloc_block(block);
  
  if (fread(&block[1], block->attributes_bsize + block->columns_bsize + block->data_bsize, 1, fp) != 1) { fprintf(stderr, "fread block failed (error 2)\n"); return NULL; }
  
  return block;
}

void write_block(FILE * fp, struct Block * block)
{
  if (block == NULL) { fprintf(stderr, "write_block passed null block.\n"); return; }
  
  float inf = INFINITY;
  if (fwrite(&inf, sizeof(inf), 1, fp) != 1) { fprintf(stderr, "fwrite block header failed.\n"); return; }
  
  uint32_t is_block = 1;
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
  char * full_command = (char*)malloc(length);
  sprintf(full_command, "");
  int i = 0;
  for (i = 0 ; i < argc ; i++)
  {
    if (strlen(full_command) + strlen(argv[i]) + 2 > length) {
      length += 100;
      full_command = (char*)realloc(full_command, length);
    }
    strcat(full_command, argv[i]);
    if (i < argc-1) strcat(full_command, " ");
  }
  block = add_string_attribute(block, "command", full_command);
  free(full_command);
  return block;
}

uint32_t memory_pad(uint32_t i, uint32_t bsize)
{
  return ceil(i / (float)bsize) * bsize;
}

/*int32_t get_type_size(int32_t type)
{
  if (type == TYPE.INT)               return sizeof(int32_t);
  else if (type == TYPE.LONG)         return sizeof(long);
  else if (type == TYPE.FLOAT)        return sizeof(float);
  else if (type == TYPE.DOUBLE)       return sizeof(double);
  else if (type >= MIN_STRING_LENGTH) return memory_pad(type+1);
  else fprintf(stderr, "column_get_cell_size invalid type '%d'\n", type);
  return 0;
}*/

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

struct Attribute * get_attribute(struct Block * block, uint32_t attribute_id)
{
  if (block == NULL || attribute_id > block->num_attributes)
  {
    fprintf(stderr, "get_attribute(%ld, %d) FAILED\n", (long int)block, attribute_id);
    return NULL;
  }
  
  int32_t * attribute_offsets = get_attribute_offsets(block);
  return (struct Attribute*)((char*)block + sizeof(struct Block) + attribute_offsets[attribute_id]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern inline uint32_t * get_column_offsets(const struct Block * block) {
  return (uint32_t*)((char*)block + sizeof(struct Block) + block->attributes_bsize);
}

extern inline uint32_t * get_cell_offsets(const struct Block * block) {
  return (uint32_t*)((char*)block + sizeof(struct Block) + block->attributes_bsize + sizeof(uint32_t)*block->num_columns);
}

extern inline struct Column * get_first_column(const struct Block * block) {
  return (block == NULL) ? NULL : (struct Column *)((char*)block + sizeof(struct Block) + block->attributes_bsize + (sizeof(int32_t)+sizeof(int32_t))*block->num_columns);
}

extern inline struct Column * get_next_column(const struct Block * block, const struct Column * column) {
  if (block == NULL || column == NULL) return NULL;
  if (column >= get_first_column(block) + block->columns_bsize) return NULL;
  return (struct Column *)((char*)column + sizeof(struct Column) + column->name_length);
}

struct Column * get_column(struct Block * block, uint32_t column_id)
{
  if (block == NULL || column_id > block->num_columns)
  {
    fprintf(stderr, "get_column(%ld, %d) FAILED\n", (long int)block, column_id);
    return NULL;
  }
  
  uint32_t * column_offsets = get_column_offsets(block);
  return (struct Column*)((char*)block + sizeof(struct Block) + block->attributes_bsize + column_offsets[column_id]);
  
  int i = 0;
  struct Column * column = get_first_column(block);
  for (i = 0 ; i != column_id ; i++)
    column = get_next_column(block, column);
  
  return column;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void * _get_row_with_different_row_bsize(struct Block * block, uint32_t row_id, uint32_t different_row_bsize)
{
  return (void*)((char*)&block[1] + block->attributes_bsize + block->columns_bsize + different_row_bsize * row_id);
}

void * get_row(struct Block * block, uint32_t row_id)
{
  return (void*)((char*)&block[1] + block->attributes_bsize + block->columns_bsize + block->row_bsize * row_id);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t get_row_bsize_from_columns(struct Block * block)
{
  uint32_t single_row_bsize = 0;
  uint32_t column_id;
  for (column_id = 0 ; column_id < block->num_columns ; column_id++)
  {
    struct Column * column = get_column(block, column_id);
    single_row_bsize += column->bsize;
  }
  
  return single_row_bsize;
}

int32_t get_column_id_by_name(struct Block * block, const char * column_name)
{
  if (block == NULL || column_name == NULL) return -1;
  uint32_t column_id;
  for (column_id = 0 ; column_id < block->num_columns ; column_id++)
  {
    struct Column * column = get_column(block, column_id);
    if (strcmp(column_name, column_get_name(column))==0) return column_id;
  }
  return -1;
}

int32_t get_column_id_by_name_or_exit(struct Block * block, const char * column_name)
{
  int32_t id = get_column_id_by_name(block, column_name);
  if (id == -1)
  {
    fprintf(stderr, "get_column_id_by_name_or_exit failed on column '%s'\n", column_name);
    exit(EXIT_FAILURE);
  }
  return id;
}

struct Column * get_column_by_name(struct Block * block, const char * column_name)
{
  return get_column(block, get_column_id_by_name(block, column_name));
}

int32_t get_attribute_id_by_name(struct Block * block, const char * attribute_name)
{
  if (block == NULL || attribute_name == NULL) return -1;
  int32_t attribute_id;
  for (attribute_id = 0 ; attribute_id < block->num_attributes ; attribute_id++)
  {
    struct Attribute * attribute = get_attribute(block, attribute_id);
    if (strcmp(attribute_name, attribute_get_name(attribute))==0) return attribute_id;
  }
  return -1;
}

struct Attribute * get_attribute_by_name(struct Block * block, const char * attribute_name)
{
  int attribute_id = get_attribute_id_by_name(block, attribute_name);
  if (attribute_id == -1) return NULL;
  return get_attribute(block, attribute_id);
}

int32_t get_attribute_value_as_int32(struct Block * block, const char * attribute_name)
{
  struct Attribute * attr = get_attribute(block, get_attribute_id_by_name(block, attribute_name));
  if      (attr->type == TYPE_INT && attr->value_length == 4) return *(int32_t*)attribute_get_value(attr);
  else if (attr->type == TYPE_INT && attr->value_length == 8) return *(int64_t*)attribute_get_value(attr);
  else if (attr->type == TYPE_UINT && attr->value_length == 4) return *(uint32_t*)attribute_get_value(attr);
  else if (attr->type == TYPE_UINT && attr->value_length == 8) return *(uint64_t*)attribute_get_value(attr);
  const char * value = get_attribute_value_as_string(block, attribute_name);
  if (value != NULL) return atoi(value);
  else
  {
    fprintf(stderr, "'%s' called on '%s' field, doesn't exist - returning 0\n", __func__, attribute_name);
    return 0;
  }
}

double get_attribute_value_as_double(struct Block * block, const char * attribute_name)
{
  int attribute_id = get_attribute_id_by_name(block, attribute_name);
  if (attribute_id == -1) 
  {
    fprintf(stderr, "'%s' called on '%s' field, doesn't exist - returning 0\n", __func__, attribute_name);
    return 0;
  }
  struct Attribute * attribute = get_attribute(block, attribute_id);
  
  if (attribute->type != TYPE_FLOAT)
  {
    fprintf(stderr, "'%s' called on '%s' field , doesn't exist - returning 0\n", __func__, attribute_name);
    return 0;
  }
  else
  {
    if      (attribute->value_length == 4) return (double)(*(float*)attribute_get_value(attribute));
    else if (attribute->value_length == 8) return (double)(*(double*)attribute_get_value(attribute));
  }
  return 0;
}

const char * get_attribute_value_as_string(struct Block * block, const char * attribute_name)
{
  struct Attribute * attribute = get_attribute_by_name(block, attribute_name);
  if (attribute == NULL) return NULL;
  
  if (attribute->type == TYPE_CHAR)
    return attribute_get_value(attribute);
  else
    fprintf(stderr, "'%s' doesn't support attribute '%s' of type '%s'\n", __func__, attribute_name, get_type_name(attribute->type, attribute->value_length));
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

char * attribute_get_name(struct Attribute * attribute)  { return (char*)attribute + sizeof(struct Attribute); }
void * attribute_get_value(struct Attribute * attribute) { return (char*)attribute + sizeof(struct Attribute) + attribute->name_length; }
void attribute_set_name(struct Attribute * attribute, const char * name)   { strncpy((char*)attribute + sizeof(struct Attribute), name, attribute->name_length); }
void attribute_set_value(struct Attribute * attribute, void * value)
{
  if (attribute->type == TYPE_CHAR) strncpy((char*)attribute + sizeof(struct Attribute) + attribute->name_length, (char*)value, attribute->value_length);
  else memcpy((char*)attribute + sizeof(struct Attribute) + attribute->name_length, value, attribute->value_length);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Block * _add_attribute(struct Block * block, enum TYPE type, uint32_t value_length, const char * name, void * value)
{
  if (block == NULL) { fprintf(stderr, "add_attribute called on a NULL block\n"); return NULL; }
  
  if (value_length != memory_pad(value_length, 4)) { fprintf(stderr, "warning: value_length for _add_attribute wasn't memory_pad'ed\n"); }
  
  int32_t name_length = memory_pad(strlen(name)+1, 4);
  //value_length = memory_pad(value_length+((type==TYPE_CHAR)?1:0), 4);
  
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

struct Block * add_int32_attribute(struct Block * block, const char * name, int32_t value) { return _add_attribute(block, TYPE_INT, sizeof(int32_t), name, &value); }
struct Block * add_int64_attribute(struct Block * block, const char * name, int64_t value) { return _add_attribute(block, TYPE_INT, sizeof(int64_t), name, &value); }
struct Block * add_float_attribute(struct Block * block, const char * name, float value)   { return _add_attribute(block, TYPE_FLOAT, sizeof(float), name, &value); }
struct Block * add_double_attribute(struct Block * block, const char * name, double value) { return _add_attribute(block, TYPE_FLOAT, sizeof(double), name, &value); }
struct Block * add_string_attribute(struct Block * block, const char * name, const char * value) { return _add_attribute(block, TYPE_CHAR, memory_pad(strlen(value)+1, 4), name, (void*)value); }

struct Block * copy_all_attributes(struct Block * block, struct Block * src)
{
  int i;
  for (i = 0 ; i < src->num_attributes ; i++)
  {
    struct Attribute * attr = get_attribute(src, i);
    block = _add_attribute(block, attr->type, attr->value_length, attribute_get_name(attr), attribute_get_value(attr));
  }
  return block;
}

struct Block * append_block(struct Block * block, struct Block * src)
{
  if (block == NULL || src == NULL) { fprintf(stderr, "append_block called with NULL blocks\n"); return block; }
  if (block->num_columns != src->num_columns) { fprintf(stderr, "append_block called with conflicting num_columns (%d vs %d)\n", block->num_columns, src->num_columns); return block; }
  //if (memcmp(block, src, sizeof(struct Block))!=0) { fprintf(stderr, "append_block called with conflicting block headers\n"); return block; }
  int i;
  for (i = 0 ; i < block->num_columns ; i++)
    if (memcmp(get_column(block, i), get_column(src, i), sizeof(struct Column))!=0) { fprintf(stderr, "append_block called with conflicting columns\n"); return block; }
  
  block = set_num_rows(block, block->num_rows + src->num_rows);
  memcpy(get_row(block, block->num_rows - src->num_rows), get_row(src, 0), src->data_bsize);
  
  return block;
}

void fprintf_attribute_value(FILE * fp, struct Block * block, uint32_t attribute_id)
{
  struct Attribute * attr = get_attribute(block, attribute_id);
  switch (attr->type) {
    case TYPE_INT:
      if      (attr->value_length == 4) { fprintf(fp, "%d", *(int32_t*)attribute_get_value(attr)); break; }
      else if (attr->value_length == 8) { fprintf(fp, "%lld", *(int64_t*)attribute_get_value(attr)); break; }
      else { fprintf(stderr, "bad %s %s:(%d)\n", __func__, __FILE__, __LINE__); break; }
    case TYPE_FLOAT:
      if      (attr->value_length == 4) { fprintf(fp, "%f", *(float*)attribute_get_value(attr)); break; }
      else if (attr->value_length == 8) { fprintf(fp, "%lf", *(double*)attribute_get_value(attr)); break; }
      else { fprintf(stderr, "bad %s %s:(%d)\n", __func__, __FILE__, __LINE__); break; }
    case TYPE_CHAR:
      fprintf(fp, "%s", (char*)attribute_get_value(attr)); break;
    default:
      fprintf(stderr, "bad %s %s:(%d)\n", __func__, __FILE__, __LINE__); break;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

char * column_get_name(struct Column * column)  { return (char*)column + sizeof(struct Column); }
void column_set_name(struct Column * column, const char * name) { strncpy(column_get_name(column), name, column->name_length); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Block * _add_column(struct Block * block, enum TYPE type, uint32_t bsize, const char * name)
{
  if (block == NULL) { fprintf(stderr, "add_field called on a NULL block\n"); return NULL; }
  
  int32_t name_length = memory_pad(strlen(name)+1, 4);
  int32_t old_columns_bsize = block->columns_bsize;
  int32_t old_data_bsize = block->data_bsize;
  int32_t old_row_bsize = block->row_bsize;
  
  if (bsize != memory_pad(bsize, 4)) fprintf(stderr, "warning: _add_column called with bsize which isn't memory_pad'ed\n");
  
  //if (type == TYPE_CHAR) bsize = memory_pad(bsize+1, 4);
  block->row_bsize += bsize;
  
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
  
  uint32_t * column_offsets = get_column_offsets(block);
  uint32_t * cell_offsets = get_cell_offsets(block);
  
  int o = 0;
  int i;
  struct Column * column = get_first_column(block);
  for (i = 0 ; i < block->num_columns - 1 ; i++)
  {
    //fprintf(stderr, "%d: %d (add %d)\n", i, o, get_type_size(column->type));
    cell_offsets[i] = o;
    o += column->bsize;//get_type_size(column->type);
    column_offsets[i] = (uint32_t)((char*)column - (char*)block - sizeof(struct Block) - block->attributes_bsize);
    column = get_next_column(block, column);
  }
  
  column->type = type;
  column->bsize = bsize;
  column->name_length = name_length;
  column_set_name(column, name);
  column_offsets[block->num_columns-1] = (uint32_t)((char*)column - (char*)block - sizeof(struct Block) - block->attributes_bsize);
  //if (block->num_columns != 1)
  cell_offsets[block->num_columns-1] = o;
  
  return block;
}

struct Block * add_int32_column(struct Block * block, const char * name)  { return _add_column(block, TYPE_INT, sizeof(int32_t), name); }
struct Block * add_int64_column(struct Block * block, const char * name)  { return _add_column(block, TYPE_INT, sizeof(int64_t), name); }
struct Block * add_float_column(struct Block * block, const char * name)  { return _add_column(block, TYPE_FLOAT, sizeof(float), name); }
struct Block * add_double_column(struct Block * block, const char * name) { return _add_column(block, TYPE_FLOAT, sizeof(double), name); }
struct Block * add_string_column_with_length(struct Block * block, const char * name, uint32_t length) { return _add_column(block, TYPE_CHAR, memory_pad(length+1,4), name); }

struct Block * add_int32_column_and_blank(struct Block * block, const char * name)  { block = add_int32_column(block, name); blank_column_values(block, name); return block; }
struct Block * add_int64_column_and_blank(struct Block * block, const char * name)  { block = add_int64_column(block, name); blank_column_values(block, name); return block; }
struct Block * add_float_column_and_blank(struct Block * block, const char * name)  { block = add_float_column(block, name); blank_column_values(block, name); return block; }
struct Block * add_double_column_and_blank(struct Block * block, const char * name) { block = add_double_column(block, name); blank_column_values(block, name); return block; }
struct Block * add_string_column_with_length_and_blank(struct Block * block, const char * name, uint32_t length) { block = add_string_column_with_length(block, name, length); blank_column_values(block, name); return block; }

struct Block * set_string_column_length(struct Block * block, uint32_t column_id, uint32_t length) {
  if (block == NULL) { fprintf(stderr, "%s called on NULL block\n", __func__); return block; }
  if (column_id >= block->num_columns) { fprintf(stderr, "%s called with column_id(%d) >= block->num_columns(%d)\n", __func__, column_id, block->num_columns); exit(0); }
  
  struct Column * column = get_column(block, column_id);
  if (column->type != TYPE_CHAR) { fprintf(stderr, "%s called, column '%s' is not a string\n", __func__, column_get_name(column)); return block; }
  
  length = memory_pad(length+1, 4);
  if (column->bsize == length) return block;

  if (column->bsize > length) { fprintf(stderr, "%s called, only supports increasing field size (%d->%d)\n", __func__, column->bsize, length); return block; }
  
  uint32_t cell_delta = length - column->bsize;

  uint32_t old_bsize = column->bsize;
  uint32_t old_row_bsize = block->row_bsize;
  uint32_t old_data_bsize = block->data_bsize;

  uint32_t new_row_bsize = block->row_bsize + cell_delta;
  uint32_t new_data_bsize = new_row_bsize * block->num_rows;

  uint32_t * cell_offsets = get_cell_offsets(block);
  int i = 0;
  // all cells to the right need to move forward a little bit
  for (i = column_id + 1 ; i < block->num_columns ; i++)
    cell_offsets[i] += cell_delta;
  
  column->bsize = length;
  block->row_bsize = new_row_bsize;
  block->data_bsize = new_data_bsize;

  block = realloc_block(block);

  //fprintf(stderr, "row_bsize changes from %d to %d\n", old_row_bsize, new_row_bsize);
  //fprintf(stderr, "data_bsize changes from %d to %d\n", old_data_bsize, new_data_bsize);

  // kbfu, spread out those data
  for (i = block->num_rows - 1 ; i >= 0 ; i--)
  {
    if (column_id != block->num_columns-1)
    {
      void * dest = get_cell(block, i, column_id + 1);
      void * src = _get_row_with_different_row_bsize(block, i, old_row_bsize) + cell_offsets[column_id+1] - cell_delta;
      //fprintf(stderr, "move %ld from %ld to %ld\n", old_row_bsize - old_bsize - ((long)get_cell(block, i, column_id) - (long)get_row(block, i)), 
      //  (long)src - (long)_get_row_with_different_row_bsize(block, i, old_row_bsize), 
      //  (long)dest - (long)get_row(block, i));
      memmove(dest, src, old_row_bsize - old_bsize - ((long)get_cell(block, i, column_id) - (long)get_row(block, i)));
    }
    void * dest = get_row(block, i);
    void * src = _get_row_with_different_row_bsize(block, i, old_row_bsize);
    memmove(dest, src, ((long)get_cell(block, i, column_id) - (long)get_row(block, i)) + old_bsize);
  }
  block->row_bsize = new_row_bsize;

  return block;
}

struct Block * add_shape_columns(struct Block * block) {
	if (block == NULL) { fprintf(stderr, "%s called on NULL block\n", __func__); return block; }
	block = add_int32_column(block, "shape_row_id");
	block = add_int32_column(block, "shape_part_id");
	block = add_int32_column(block, "shape_part_type");
	block = add_xy_columns(block);
	return block;
}


struct Block * add_xy_columns(struct Block * block) {
	if (block == NULL) { fprintf(stderr, "%s called on NULL block\n", __func__); return block; }
	update_cached_block_column_ids(block);
	if (get_column_id_by_name(block, "x") == -1) {
		block = add_float_column(block, "x");
	}
	if (get_column_id_by_name(block, "y") == -1) {
		block = add_float_column(block, "y");
	}
	return block;
}

struct Block * add_xyz_columns(struct Block * block) {
	if (block == NULL) { fprintf(stderr, "%s called on NULL block\n", __func__); return block; }
	update_cached_block_column_ids(block);
	block = add_xy_columns(block);
	if (get_column_id_by_name(block, "z") == -1) {
		block = add_float_column(block, "z");
	}
	return block;
}

struct Block * add_rgb_columns(struct Block * block) {
	if (block == NULL) { fprintf(stderr, "%s called on NULL block\n", __func__); return block; }
	update_cached_block_column_ids(block);
	if (get_column_id_by_name(block, "red") == -1) {
		block = add_float_column(block, "red");
	}
	if (get_column_id_by_name(block, "green") == -1) {
		block = add_float_column(block, "green");
	}
	if (get_column_id_by_name(block, "blue") == -1) {
		block = add_float_column(block, "blue");
	}
	return block;
}

struct Block * add_rgba_columns(struct Block * block) {
	if (block == NULL) { fprintf(stderr, "%s called on NULL block\n", __func__); return block; }
	update_cached_block_column_ids(block);
	block = add_rgb_columns(block);
	if (get_column_id_by_name(block, "alpha") == -1) {
		block = add_float_column(block, "alpha");
	}
	return block;
}

void blank_column_values(struct Block * block, const char * column_name) {
	if (block == NULL) { fprintf(stderr, "%s called on NULL block\n", __func__); return; }
	if (column_name == NULL) { fprintf(stderr, "%s called with NULL column_name\n", __func__); return; }
	uint32_t column_id = get_column_id_by_name(block, column_name);
	if (column_id == -1) {
		fprintf(stderr, "%s called on column '%s', column not found.\n", column_name, __func__);
		return;
	}
	struct Column * column = get_column(block, column_id);
	
	int row_id;
	for (row_id = 0 ; row_id < block->num_rows ; row_id++) {
		memset(get_cell(block, row_id, column_id), 0, column->bsize);//get_type_size(column->type));
	}
}

/*struct Block * column_string_set_length(struct Block * block, uint32_t column_id, int32_t length)
{
  fprintf(stderr, "column_string_set_length() not implemented\n");
  exit(0);
}*/

struct Block * copy_all_columns(struct Block * block, struct Block * src)
{
  int i;
  for (i = 0 ; i < src->num_columns ; i++)
  {
    struct Column * column = get_column(src, i);
    block = _add_column(block, column->type, column->bsize, column_get_name(column));
  }
  if (block->row_bsize != src->row_bsize)
  {
    fprintf(stderr, "copy_all_columns(%d, %d)\n", block->row_bsize, src->row_bsize);
    //exit(1);
  }
  return block;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void * get_cell(struct Block * block, uint32_t row_id, uint32_t column_id)
{
  uint32_t * cell_offsets = get_cell_offsets(block);
  return (char*)get_row(block, row_id) + cell_offsets[column_id];
}

int32_t get_cell_as_int32(struct Block * block, uint32_t row_id, uint32_t column_id)
{
  void * cell = get_cell(block, row_id, column_id);
  struct Column * column = get_column(block, column_id);
  
  int32_t temp = -1;
  
  if      (column->type == TYPE_INT   && column->bsize == 4) temp = *(int32_t*)cell;
  else if (column->type == TYPE_INT   && column->bsize == 8) temp = *(int64_t*)cell;
  else if (column->type == TYPE_FLOAT && column->bsize == 4) temp = *(float*)cell;
  else if (column->type == TYPE_FLOAT && column->bsize == 8) temp = *(double*)cell;
  else if (column->type == TYPE_CHAR) temp = atoi((char*)cell);
  else { fprintf(stderr, "bad %s\n", __func__); return 0; }
  
  return temp;
}

int64_t get_cell_as_int64(struct Block * block, uint32_t row_id, uint32_t column_id)
{
  void * cell = get_cell(block, row_id, column_id);
  struct Column * column = get_column(block, column_id);
  
  int64_t temp = 0;
  
  if      (column->type == TYPE_INT   && column->bsize == 4) temp = *(int32_t*)cell;
  else if (column->type == TYPE_INT   && column->bsize == 8) temp = *(int64_t*)cell;
  else if (column->type == TYPE_FLOAT && column->bsize == 4) temp = *(float*)cell;
  else if (column->type == TYPE_FLOAT && column->bsize == 8) temp = *(double*)cell;
  else if (column->type == TYPE_CHAR) temp = atol((char*)cell);
  else { fprintf(stderr, "bad %s\n", __func__); return 0; }
  
  return temp;
}

float get_cell_as_float(struct Block * block, uint32_t row_id, uint32_t column_id)
{
  void * cell = get_cell(block, row_id, column_id);
  struct Column * column = get_column(block, column_id);
  
  float temp = 0;
  
  if      (column->type == TYPE_INT   && column->bsize == 4) temp = *(int32_t*)cell;
  else if (column->type == TYPE_INT   && column->bsize == 8) temp = *(int64_t*)cell;
  else if (column->type == TYPE_FLOAT && column->bsize == 4) temp = *(float*)cell;
  else if (column->type == TYPE_FLOAT && column->bsize == 8) temp = *(double*)cell;
  else if (column->type == TYPE_CHAR) temp = atof((char*)cell);
  else { fprintf(stderr, "bad %s\n", __func__); return 0; }
  
  return temp;
}

double get_cell_as_double(struct Block * block, uint32_t row_id, uint32_t column_id)
{
  void * cell = get_cell(block, row_id, column_id);
  struct Column * column = get_column(block, column_id);
  
  double temp = 0;
  
  if      (column->type == TYPE_INT   && column->bsize == 4) temp = *(int32_t*)cell;
  else if (column->type == TYPE_INT   && column->bsize == 8) temp = *(int64_t*)cell;
  else if (column->type == TYPE_FLOAT && column->bsize == 4) temp = *(float*)cell;
  else if (column->type == TYPE_FLOAT && column->bsize == 8) temp = *(double*)cell;
  else if (column->type == TYPE_CHAR) temp = atof((char*)cell);
  else { fprintf(stderr, "bad %s\n", __func__); return 0; }
  
  return temp;
}

char * get_cell_as_char_temp = NULL;
char * get_cell_as_char(struct Block * block, uint32_t row_id, uint32_t column_id)
{
  void * cell = get_cell(block, row_id, column_id);
  struct Column * column = get_column(block, column_id);
  
  if (get_cell_as_char_temp == NULL) get_cell_as_char_temp = malloc(500);
  
  if      (column->type == TYPE_INT   && column->bsize == 4) snprintf(get_cell_as_char_temp, 500, "%d",  *(int32_t*)cell);
  else if (column->type == TYPE_INT   && column->bsize == 8) snprintf(get_cell_as_char_temp, 500, "%ld", *(long*)cell);
  else if (column->type == TYPE_FLOAT && column->bsize == 4) snprintf(get_cell_as_char_temp, 500, "%f",  *(float*)cell);
  else if (column->type == TYPE_FLOAT && column->bsize == 8) snprintf(get_cell_as_char_temp, 500, "%lf", *(double*)cell);
  else if (column->type == TYPE_CHAR) strncpy(get_cell_as_char_temp, (char*)cell, 500);
  else { fprintf(stderr, "bad %s\n", __func__); return NULL; }

  return get_cell_as_char_temp;
}

void set_cell(struct Block * block, uint32_t row_id, uint32_t column_id, void * value)
{
  if (block == NULL) { fprintf(stderr, "set_cell called with null block\n"); exit(0); }
  if (row_id >= block->num_rows) { fprintf(stderr, "set_cell called with row_id >= block->num_rows\n"); exit(0); }
  if (column_id >= block->num_columns) { fprintf(stderr, "set_cell called with column_id(%d) >= block->num_columns(%d)\n", column_id, block->num_columns); exit(0); }
  
  struct Column * column = get_column(block, column_id);
  void * cell = get_cell(block, row_id, column_id);
  if (value == NULL)
    memset(cell, 0, column->bsize);//get_type_size(column->type));
  else
    memcpy(cell, value, column->bsize);//get_type_size(column->type));
}

void set_cell_from_int32(struct Block * block, uint32_t row_id, uint32_t column_id, int32_t data)
{
	if (block == NULL) { fprintf(stderr, "%s called with null block\n", __func__); exit(0); }
	if (row_id >= block->num_rows) { fprintf(stderr, "%s called with row_id(%d) >= block->num_rows(%d)\n", __func__, row_id, block->num_rows); exit(0); }
	if (column_id >= block->num_columns) { fprintf(stderr, "%s called with column_id(%d) >= block->num_columns(%d)\n", __func__, column_id, block->num_columns); exit(0); }
  
  struct Column * column = get_column(block, column_id);
  void * cell = get_cell(block, row_id, column_id);
  if      (column->type == TYPE_INT && column->bsize == 4) *(int32_t*)cell = data;
  else if (column->type == TYPE_INT && column->bsize == 8) *(int64_t*)cell = data;
  else if (column->type == TYPE_FLOAT && column->bsize == 4) *(float*)cell = data;
  else if (column->type == TYPE_FLOAT && column->bsize == 8) *(double*)cell = data;
  else if (column->type == TYPE_CHAR) snprintf((char*)cell, column->bsize, "%d", data);
  else fprintf(stderr, "bad %s\n", __func__);
}

void set_cell_from_int64(struct Block * block, uint32_t row_id, uint32_t column_id, int64_t data) {
	if (block == NULL) { fprintf(stderr, "%s called with null block\n", __func__); exit(0); }
	if (row_id >= block->num_rows) { fprintf(stderr, "%s called with row_id(%d) >= block->num_rows(%d)\n", __func__, row_id, block->num_rows); exit(0); }
	if (column_id >= block->num_columns) { fprintf(stderr, "%s called with column_id(%d) >= block->num_columns(%d)\n", __func__, column_id, block->num_columns); exit(0); }
	
	struct Column * column = get_column(block, column_id);
	void * cell = get_cell(block, row_id, column_id);
	if      (column->type == TYPE_INT && column->bsize == 4) *(int32_t*)cell = data;
	else if (column->type == TYPE_INT && column->bsize == 8) *(int64_t*)cell = data;
	else if (column->type == TYPE_FLOAT && column->bsize == 4) *(float*)cell = data;
	else if (column->type == TYPE_FLOAT && column->bsize == 8) *(double*)cell = data;
	else if (column->type == TYPE_CHAR) snprintf((char*)cell, column->bsize, "%lld", data);
	else fprintf(stderr, "bad %s\n", __func__);
}

void set_cell_from_double(struct Block * block, uint32_t row_id, uint32_t column_id, double data)
{
  if (block == NULL) { fprintf(stderr, "set_cell_from_double called with null block\n"); exit(0); }
  if (row_id >= block->num_rows) { fprintf(stderr, "set_cell_from_double called with row_id >= block->num_rows\n"); exit(0); }
  if (column_id >= block->num_columns) { fprintf(stderr, "set_cell_from_double called with column_id(%d) >= block->num_columns(%d)\n", column_id, block->num_columns); exit(0); }
  
  struct Column * column = get_column(block, column_id);
  void * cell = get_cell(block, row_id, column_id);
  if      (column->type == TYPE_INT && column->bsize == 4) *(int32_t*)cell = data;
  else if (column->type == TYPE_INT && column->bsize == 8) *(int64_t*)cell = data;
  else if (column->type == TYPE_FLOAT && column->bsize == 4) *(float*)cell = data;
  else if (column->type == TYPE_FLOAT && column->bsize == 8) *(double*)cell = data;
  else if (column->type == TYPE_CHAR) snprintf((char*)cell, column->bsize, "%f", data);
  else fprintf(stderr, "bad %s\n", __func__);
}

void set_cell_from_string(struct Block * block, uint32_t row_id, uint32_t column_id, char * data)
{
  if (block == NULL) { fprintf(stderr, "set_cell_from_string called with null block\n"); exit(0); }
  if (row_id >= block->num_rows) { fprintf(stderr, "set_cell_from_string called with row_id >= block->num_rows\n"); exit(0); }
  if (column_id >= block->num_columns) { fprintf(stderr, "set_cell_from_string called with column_id(%d) >= block->num_columns(%d)\n", column_id, block->num_columns); exit(0); }
  
  struct Column * column = get_column(block, column_id);
  void * cell = get_cell(block, row_id, column_id);
  if      (column->type == TYPE_INT && column->bsize == 4) *(int32_t*)cell = atoi(data);
  else if (column->type == TYPE_INT && column->bsize == 8) *(int64_t*)cell = atol(data);
  else if (column->type == TYPE_FLOAT && column->bsize == 4) *(float*)cell = atof(data);
  else if (column->type == TYPE_FLOAT && column->bsize == 8) *(double*)cell = atof(data);
  else if (column->type == TYPE_CHAR) strncpy((char*)cell, data, column->bsize);
  else fprintf(stderr, "bad %s\n", __func__);
}

void fprintf_cell(FILE * fp, struct Block * block, uint32_t row_id, uint32_t column_id)
{
  struct Column * column = get_column(block, column_id);
  switch (column->type) {
    case TYPE_INT:
      if      (column->bsize == 4) fprintf(fp, "%d", *(int32_t*)get_cell(block, row_id, column_id));
      else if (column->bsize == 8) fprintf(fp, "%lld", *(int64_t*)get_cell(block, row_id, column_id));
      else                         fprintf(stderr, "bad %s %s:(%d)\n", __func__, __FILE__, __LINE__);
      break;
    case TYPE_FLOAT:
      if      (column->bsize == 4) fprintf(fp, "%f", *(float*)get_cell(block, row_id, column_id));
      else if (column->bsize == 8) fprintf(fp, "%lf", *(double*)get_cell(block, row_id, column_id));
      else                         fprintf(stderr, "bad %s %s:(%d)\n", __func__, __FILE__, __LINE__);
      break;
    case TYPE_CHAR:
      fprintf(fp, "%s", (char*)get_cell(block, row_id, column_id));
      break;
    default:
      fprintf(stderr, "%s can't fprint_cell column '%s' of type %d\n", __func__, column_get_name(column), column->type);
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t get_next_shape_start(struct Block * block, int32_t shape_start_id)
{
  if (shape_start_id < 0 || shape_start_id >= block->num_rows) return 0;
  if (cached_block_ptr != block) update_cached_block_column_ids(block);
  
  if (cached_block_column_ids.shape_row_id == -1) { fprintf(stderr, "%s called on block without shape_row_id field.\n", __func__); return 0; }
  if (cached_block_column_ids.shape_part_id == -1) { fprintf(stderr, "%s called on block without shape_part_id field.\n", __func__); return 0; }
  
  int32_t shape_row_id = get_cell_as_int32(block, shape_start_id, cached_block_column_ids.shape_row_id);
  //int32_t shape_part_id = get_cell_as_int32(block, row_start_id, cached_block_column_ids.shape_part_id);
  
  int i;
  for (i = shape_start_id ; i < block->num_rows ; i++)
  {
    if (shape_row_id != get_cell_as_int32(block, i, cached_block_column_ids.shape_row_id))
    {
      return i;
    }
  }
  return block->num_rows;
}

int32_t get_next_part_start(struct Block * block, int32_t part_start_id)
{
  if (part_start_id < 0 || part_start_id >= block->num_rows) return 0;
  if (cached_block_ptr != block) update_cached_block_column_ids(block);
  
  if (cached_block_column_ids.shape_row_id == -1) { fprintf(stderr, "%s called on block without shape_row_id field.\n", __func__); return 0; }
  if (cached_block_column_ids.shape_part_id == -1) { fprintf(stderr, "%s called on block without shape_part_id field.\n", __func__); return 0; }
  
  int32_t shape_row_id = get_cell_as_int32(block, part_start_id, cached_block_column_ids.shape_row_id);
  int32_t shape_part_id = get_cell_as_int32(block, part_start_id, cached_block_column_ids.shape_part_id);
  
  int i;
  for (i = part_start_id ; i < block->num_rows ; i++)
  {
    if (shape_row_id != get_cell_as_int32(block, i, cached_block_column_ids.shape_row_id) ||
        shape_part_id != get_cell_as_int32(block, i, cached_block_column_ids.shape_part_id))
    {
      return i;
    }
  }
  return block->num_rows;
}

int32_t get_new_shape_row_id(struct Block * block) {
	int shape_row_id = 1;
	if (block == NULL) { fprintf(stderr, "%s called on NULL block\n", __func__); return 0; }
	
	if (block->num_rows == 0) return shape_row_id;
	
	int shape_row_id_column_id = get_column_id_by_name(block, "shape_row_id");
	if (shape_row_id_column_id == -1) {
		fprintf(stderr, "%s called on block without shape_row_id field.\n", __func__);
		return 0;
	} else {
		return get_cell_as_int32(block, block->num_rows-1, shape_row_id_column_id) + 1;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

double get_x(struct Block * block, uint32_t row_id)
{
  if (row_id > block->num_rows) { fprintf(stderr, "get_x invalid row_id (%d)\n", row_id); exit(0); }
  if (cached_block_ptr != block) update_cached_block_column_ids(block);
	if (cached_block_column_ids.xyz[0] != -1) {
		return get_cell_as_double(block, row_id, cached_block_column_ids.xyz[0]);
	} else {
		return 0;
	}
}

double get_y(struct Block * block, uint32_t row_id) {
	if (row_id > block->num_rows) { fprintf(stderr, "get_y invalid row_id (%d)\n", row_id); exit(0); }
	if (cached_block_ptr != block) update_cached_block_column_ids(block);
	if (cached_block_column_ids.xyz[1] != -1) {
		return get_cell_as_double(block, row_id, cached_block_column_ids.xyz[1]);
	} else {
		return 0;
	}
}

double get_z(struct Block * block, uint32_t row_id) {
	if (row_id > block->num_rows) { fprintf(stderr, "get_z invalid row_id (%d)\n", row_id); exit(0); }
	if (row_id > block->num_rows) { fprintf(stderr, "%s invalid row_id (%d)\n", __func__, row_id); return 0; }
	if (cached_block_ptr != block) update_cached_block_column_ids(block);
	if (cached_block_column_ids.xyz[2] != -1) {
		return get_cell_as_double(block, row_id, cached_block_column_ids.xyz[2]);
	} else {
		return 0;
	}
}

void set_shape_part(struct Block * block, uint32_t row_id, int shape_row_id, int shape_part_id, int shape_part_type) {
	if (block == NULL) { fprintf(stderr, "%s called on NULL block\n", __func__); return; }
	if (row_id > block->num_rows) { fprintf(stderr, "%s invalid row_id (%d)\n", __func__, row_id); return; }
	if (cached_block_ptr != block) update_cached_block_column_ids(block);
	if (cached_block_column_ids.shape_row_id != -1) set_cell_from_double(block, row_id, cached_block_column_ids.shape_row_id, shape_row_id);
	if (cached_block_column_ids.shape_part_id != -1) set_cell_from_double(block, row_id, cached_block_column_ids.shape_part_id, shape_part_id);
	if (cached_block_column_ids.shape_part_type != -1) set_cell_from_double(block, row_id, cached_block_column_ids.shape_part_type, shape_part_type);
}

void set_xy(struct Block * block, uint32_t row_id, double x, double y)
{
  if (row_id > block->num_rows) { fprintf(stderr, "set_xy invalid row_id (%d)\n", row_id); exit(0); }
  if (cached_block_ptr != block) update_cached_block_column_ids(block);
  if (cached_block_column_ids.xyz[0] != -1) set_cell_from_double(block, row_id, cached_block_column_ids.xyz[0], x);
  if (cached_block_column_ids.xyz[1] != -1) set_cell_from_double(block, row_id, cached_block_column_ids.xyz[1], y);
}

void set_xyz(struct Block * block, uint32_t row_id, double x, double y, double z)
{
  set_xy(block, row_id, x, y);
  if (cached_block_column_ids.xyz[2] != -1) set_cell_from_double(block, row_id, cached_block_column_ids.xyz[2], z);
}

void set_rgb(struct Block * block, uint32_t row_id, float r, float g, float b)
{
  if (row_id > block->num_rows) { fprintf(stderr, "set_rgb invalid row_id (%d)\n", row_id); exit(0); }
  if (cached_block_ptr != block) update_cached_block_column_ids(block);
  if (cached_block_column_ids.rgba[0] != -1) set_cell(block, row_id, cached_block_column_ids.rgba[0], &r);
  if (cached_block_column_ids.rgba[1] != -1) set_cell(block, row_id, cached_block_column_ids.rgba[1], &g);
  if (cached_block_column_ids.rgba[2] != -1) set_cell(block, row_id, cached_block_column_ids.rgba[2], &b);
}

void set_rgba(struct Block * block, uint32_t row_id, float r, float g, float b, float a)
{
  set_rgb(block, row_id, r, g, b);
  if (cached_block_column_ids.rgba[3] != -1) set_cell(block, row_id, cached_block_column_ids.rgba[3], &a);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Block * add_row(struct Block * block)
{
  if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return NULL; }
  
  block = set_num_rows(block, block->num_rows + 1);
  
  return block;
}

struct Block * add_row_and_blank(struct Block * block)
{
	if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return NULL; }
	block = add_row(block);
	memset(get_row(block, block->num_rows-1), 0, block->row_bsize);
	return block;
}

struct Block * add_row_with_data(struct Block * block, int num_columns, ...)
{
	if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return NULL; }
  if (block->num_columns != num_columns) { fprintf(stderr, "block num_columns not the same as provided num_columns\n"); return block; }
  
  block = add_row(block);
  
  int row_id = block->num_rows - 1;
  int column_id;
  va_list v1;
  va_start(v1, num_columns);
  for (column_id = 0 ; column_id < block->num_columns ; column_id++)
  {
    struct Column * column = get_column(block, column_id);
    if      (column->type == TYPE_INT && column->bsize == 4)   { int32_t value = va_arg(v1, int);          set_cell(block, row_id, column_id, &value); }
    else if (column->type == TYPE_INT && column->bsize == 8)   { long value = va_arg(v1, long);            set_cell(block, row_id, column_id, &value); }
    else if (column->type == TYPE_FLOAT && column->bsize == 4) { float value = (float)va_arg(v1, double);  set_cell(block, row_id, column_id, &value); }
    else if (column->type == TYPE_FLOAT && column->bsize == 8) { double value = va_arg(v1, double);        set_cell(block, row_id, column_id, &value); }
    else if (column->type == TYPE_CHAR)    { char * value = va_arg(v1, char *);        set_cell(block, row_id, column_id, value); }
    else fprintf(stderr, "bad %s\n", __func__);
  }
  va_end(v1);
  
  return block;
}

struct Block * insert_row_before(struct Block * block, uint32_t row_id) {
	if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return NULL; }
	if (row_id >= block->num_rows) { fprintf(stderr, "%s called on block with invalid row_id (%d)\n", __func__, row_id); return block; }
	
	block = add_row(block);
	memmove(get_row(block, row_id+1), get_row(block, row_id), block->row_bsize * (block->num_rows - row_id));
	
	fprintf(stderr, " NOT TESTED\n");
	
	return block;
}

struct Block * insert_row_after(struct Block * block, uint32_t row_id) {
	if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return NULL; }
	if (row_id >= block->num_rows) { fprintf(stderr, "%s called on block with invalid row_id (%d)\n", __func__, row_id); return block; }
	
	block = add_row(block);
	memmove(get_row(block, row_id+2), get_row(block, row_id+1), block->row_bsize * (block->num_rows - row_id - 1));
	
	fprintf(stderr, " NOT TESTED\n");
	
	return block;
}

struct Block * remove_row(struct Block * block, uint32_t row_id) {
	if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return NULL; }
	if (row_id >= block->num_rows) { fprintf(stderr, "%s called on block with invalid row_id (%d)\n", __func__, row_id); return block; }
	
	if (row_id < block->num_rows - 1) {
		memmove(get_row(block, row_id), get_row(block, row_id+1), block->row_bsize * (block->num_rows - row_id - 1));
	}
	block = set_num_rows(block, block->num_rows-1);
	
	return block;
}


int command_argc = 0;
char ** command_argv = NULL;
void add_command_in_foreach(int argc, char ** argv) {
	command_argc = argc;
	command_argv = argv;
}

int foreach_block(FILE * fpin, FILE * fpout, int free_blocks,
		struct Block * (*blockFuncPtr)(struct Block * block)) {
	if (blockFuncPtr == NULL) return 0;
	int count = 0;
	struct Block * block = NULL;
	while ((block = read_block(fpin))) {
		
		if (command_argc != 0) {
			block = add_command(block, command_argc, command_argv);
		}
		
		count++;
		if (blockFuncPtr != NULL) {
			block = (*blockFuncPtr)(block);
		}
		
		if (block == NULL) {
			continue;
		}
		
		if (fpout != NULL) {
			write_block(fpout, block);
		}
		
		if (free_blocks) {
			free_block(block);
		}
	}
	return count;
}

int foreach_shape(struct Block * block,
		int (*shapeFuncPtr)(struct Block * block, uint32_t shape_start_id, uint32_t shape_end_id)) {
	if (shapeFuncPtr == NULL) return 0;
	if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return 0; }
	
	int shape_start_id = 0, shape_end_id;
	while ((shape_end_id = get_next_shape_start(block, shape_start_id))) {
		int offset = (*shapeFuncPtr)(block, shape_start_id, shape_end_id);
		shape_end_id += offset;
		
		if (shape_end_id == block->num_rows) {
			break; // last shape
		}
		shape_start_id = shape_end_id;
	}
  return 0;
}

int foreach_part(struct Block * block, uint32_t shape_start_id, uint32_t shape_end_id,
		int (*partFuncPtr)(struct Block * block, uint32_t part_start_id, uint32_t part_end_id)) {
	if (partFuncPtr == NULL) return 0;
	if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return 0; }
	
	int part_start_id = shape_start_id, part_end_id;
	while ((part_end_id = get_next_part_start(block, part_start_id))) {
		int offset = (*partFuncPtr)(block, part_start_id, part_end_id);
		part_end_id += offset;
		
		if (part_end_id == shape_end_id) {
			break; // last part of shape
		}
		part_start_id = part_end_id;
	}
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Block * sort_block_using_int32_column(struct Block * block, int32_t column_id, char order)
{
	if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return block; }
  if (block->num_rows <= 1) return block;
  
  int32_t * indexes = malloc(sizeof(int32_t)*block->num_rows);
  
  struct Block * ret = new_block();
  ret = copy_all_attributes(ret, block);
  ret = copy_all_columns(ret, block);
  ret = set_num_rows(ret, block->num_rows);
  
  int i,j;
  for (i = 0 ; i < block->num_rows ; i++)
    indexes[i] = i;
  
  int loop_count = 0;
  
  int sorted = 0;
  while (sorted == 0)
  {
    sorted = 1;
    for (i = 0 ; i < block->num_rows - 1 ; i++)
    {
      if ((order && get_cell_as_int32(block, indexes[i], column_id) < get_cell_as_int32(block, indexes[i+1], column_id)) || 
          (!order && get_cell_as_int32(block, indexes[i], column_id) > get_cell_as_int32(block, indexes[i+1], column_id)))
      {
        int32_t temp = indexes[i];
        indexes[i] = indexes[i+1];
        indexes[i+1] = temp;
        sorted = 0;
        i--;
      }
    }
    loop_count++;
  }
  
  for (i = 0 ; i < block->num_rows ; i++)
    memcpy(get_row(ret, i), get_row(block, indexes[i]), block->row_bsize);
  
  fprintf(stderr, "really slow sort algorithm, took %d loops for %d rows\n", loop_count, block->num_rows);
  
  free(indexes);
  free_block(block);
  
  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void inspect_block(struct Block * block) {
	if (block == NULL) { fprintf(stderr, "%s called on a NULL block\n", __func__); return; }
	fprintf(stderr, "\nblock (%d+%d+%d=%d total size in bytes - with %d byte header)\n", block->attributes_bsize, block->columns_bsize, block->data_bsize, block->attributes_bsize + block->columns_bsize + block->data_bsize, (int)SIZEOF_STRUCT_BLOCK);
  
  if (block->num_attributes == 0)
    fprintf(stderr, "     ->no_attributes\n");
  else
  {
    fprintf(stderr, "     ->attributes(count:%d, %d bytes) = [\n", block->num_attributes, block->attributes_bsize);
    fprintf(stderr, "       ->attributes_offsets(");
    int attribute_id;
    int32_t * attribute_offset = get_attribute_offsets(block);
    for (attribute_id = 0 ; attribute_id < block->num_attributes ;  attribute_id++)
      fprintf(stderr, "%s%d", (attribute_id==0)?"":", ", attribute_offset[attribute_id]);
    fprintf(stderr, ")\n");
    
    for (attribute_id = 0 ; attribute_id < block->num_attributes ;  attribute_id++)
    {
      struct Attribute * attribute = get_attribute(block, attribute_id);
      char * name = attribute_get_name(attribute);
      void * value = attribute_get_value(attribute);
      fprintf(stderr, "       [%2d][%7s] \"%s\" = ", attribute_id, get_type_name(attribute->type, attribute->value_length), name);
      fprintf_attribute_value(stderr, block, attribute_id);
      fprintf(stderr, "\n");
    }
    fprintf(stderr, "     ]\n");
  }
  
  if (block->num_columns == 0)
    fprintf(stderr, "     ->no_fields\n");
  else
  {
    fprintf(stderr, "     ->columns(count:%d, %d bytes) = [\n", block->num_columns, block->columns_bsize);
    fprintf(stderr, "       ->column_offsets(");
    int column_id;
    uint32_t * column_offset = get_column_offsets(block);
    uint32_t * cell_offset = get_cell_offsets(block);
    for (column_id = 0 ; column_id < block->num_columns ;  column_id++)
      fprintf(stderr, "%s%d", (column_id==0)?"":", ", column_offset[column_id]);
    fprintf(stderr, ")\n");
    fprintf(stderr, "       ->cell_offsets(");
    for (column_id = 0 ; column_id < block->num_columns ;  column_id++)
      fprintf(stderr, "%s%d", (column_id==0)?"":", ", cell_offset[column_id]);
    fprintf(stderr, ")\n");
    
    for (column_id = 0 ; column_id < block->num_columns ; column_id++)
    {
      struct Column * column = get_column(block, column_id);
      fprintf(stderr, "       [%2d][%7s][%3d][%3d] = \"%s\"\n", 
        column_id, get_type_name(column->type, column->bsize), 
        column_offset[column_id], cell_offset[column_id], 
        column_get_name(column));
    }
    fprintf(stderr, "     ]\n");
  }
  
  if (block->num_rows == 0)
    fprintf(stderr, "     ->no_data(row_bsize %d bytes)\n", block->row_bsize);
  else
  {
    fprintf(stderr, "     ->data(count:%d, %d bytes (%d each row)) = [\n", block->num_rows, block->data_bsize, block->row_bsize);
    int row_id;
    for (row_id = 0 ; row_id < block->num_rows ; row_id++)
    {
      fprintf(stderr, "       [%2d] = { ", row_id);
      
      int column_id;
      for (column_id = 0 ; column_id < block->num_columns ; column_id++)
      {
        struct Column * column = get_column(block, column_id);
        if (column == NULL) continue;
        void * cell = get_cell(block, row_id, column_id);
        if (column_id != 0) fprintf(stderr, ", ");
        
        if      (cell == NULL) fprintf(stderr, "NULL");
        else if (column->type == TYPE_INT && column->bsize == 4)   fprintf(stderr, "%d", *(int32_t*)cell);
        else if (column->type == TYPE_INT && column->bsize == 8)   fprintf(stderr, "%lld", *(int64_t*)cell);
        else if (column->type == TYPE_FLOAT && column->bsize == 4) fprintf(stderr, "%f", *(float*)cell);
        else if (column->type == TYPE_FLOAT && column->bsize == 8) fprintf(stderr, "%lf", *(double*)cell);
        else if (column->type == TYPE_CHAR)    fprintf(stderr, "\"%s\"", (char*)cell);
      }
      
      if (row_id >= 25 && row_id < block->num_rows - 5) { fprintf(stderr, " }\n       ....\n"); row_id = block->num_rows-5; continue; }
      fprintf(stderr, " }\n");
    }
    fprintf(stderr, "     ]\n");
  }
  fprintf(stderr, "\n");
}

const char block_type_names[12][20] = {
  "unknown",
  "int8", "int16", "int32", "int64",
  "uint8", "uint16", "uint32", "uint64",
  "float", "double", "char"
};

const char * get_type_name(enum TYPE type, uint32_t bsize)
{
  if      (type == 0)                        return block_type_names[0];
  else if (type == TYPE_INT && bsize == 1)   return block_type_names[1];
  else if (type == TYPE_INT && bsize == 2)   return block_type_names[2];
  else if (type == TYPE_INT && bsize == 4)   return block_type_names[3];
  else if (type == TYPE_INT && bsize == 8)   return block_type_names[4];
  else if (type == TYPE_UINT && bsize == 1)  return block_type_names[5];
  else if (type == TYPE_UINT && bsize == 2)  return block_type_names[6];
  else if (type == TYPE_UINT && bsize == 4)  return block_type_names[7];
  else if (type == TYPE_UINT && bsize == 8)  return block_type_names[8];
  else if (type == TYPE_FLOAT && bsize == 4) return block_type_names[9];
  else if (type == TYPE_FLOAT && bsize == 8) return block_type_names[10];
  else if (type == TYPE_CHAR)
  {
    static char get_type_name_temp[100] = "";
    sprintf(get_type_name_temp, "char%3d", bsize);
    return get_type_name_temp;
  }
  return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////

struct Param {
	char name[30];
	char name_char;
	enum TYPE type;
	void * dest;
	int required;
	int found;
};

struct Params {
	struct Param * params;
	int num_params;
};

struct Params * _add_param(struct Params * params, const char * name, char name_char, enum TYPE type, void * dest, int required) {
	if (params == NULL) {
		params = (struct Params *)malloc(sizeof(struct Params));
		memset(params, 0, sizeof(struct Params));
	}
	params->params = (struct Param *)realloc(params->params, sizeof(struct Param)*(params->num_params+1));
	params->num_params++;
	memset(&params->params[params->num_params-1], 0, sizeof(struct Param));
	
	if (name != NULL) {
		strncpy(params->params[params->num_params-1].name, name, 30);
	}
	
	params->params[params->num_params-1].name_char = name_char;
	params->params[params->num_params-1].type = type;
	params->params[params->num_params-1].dest = dest;
	params->params[params->num_params-1].required = required;
	return params;
}

struct Params * add_string_param(struct Params * params, const char * name, char name_char, char * dest, int required) {
	return _add_param(params, name, name_char, TYPE_CHAR, (void*)dest, required);
}

struct Params * add_float_param(struct Params * params, const char * name, char name_char, float * dest, int required) {
	return _add_param(params, name, name_char, TYPE_FLOAT, (void*)dest, required);
}

struct Params * add_flag_param(struct Params * params, const char * name, char name_char, int * dest, int required) {
	return _add_param(params, name, name_char, 0, (void*)dest, required);
}

struct Params * add_int_param(struct Params * params, const char * name, char name_char, int * dest, int required) {
	return _add_param(params, name, name_char, TYPE_INT, (void*)dest, required);
}

struct Params * add_longlong_param(struct Params * params, const char * name, char name_char, long long * dest, int required) {
	return _add_param(params, name, name_char, TYPE_LONGLONG, (void*)dest, required);
}

int eval_params(struct Params * params, int argc, char ** argv) {
	if (params == NULL) { fprintf(stderr, "%s called with NULL params\n", __func__); return 0; }
	
	struct option * long_options = (struct option*)malloc(sizeof(struct option)*(params->num_params+1));
	memset(long_options, 0, sizeof(struct option)*(params->num_params+1));
	
	char char_list[100] = "";
	
	//fprintf(stderr, "%d\n", params->num_params);
	int i;
	for (i = 0 ; i < params->num_params ; i++) {
		char temp[3] = { 0, 0, 0 };
		//fprintf(stderr, "%d: %d\n", i, params->params[i].name_char);
		temp[0] = params->params[i].name_char;
		if (params->params[i].type) {
			temp[1] = ':';
		}
		strcat(char_list, temp);
		
		struct option * curr_option = &long_options[i];
		curr_option->name = params->params[i].name;
		if (params->params[i].type == 0) { // flag
			curr_option->has_arg = 0;
			curr_option->flag = params->params[i].dest;
			curr_option->val = 1;
		} else {
			curr_option->has_arg = 1;
			curr_option->flag = 0;
			curr_option->val = params->params[i].name_char;
		}
	}
	
	int c;
	while (1) {
		
		int option_index = 0;
		c = getopt_long(argc, argv, char_list, long_options, &option_index);
		if (c == -1) break;
		if (c == 0) continue;
		
		for (i = 0 ; i < params->num_params ; i++) {
			if (c == params->params[i].name_char) {
				if (params->params[i].type == TYPE_CHAR) {
					if (optarg == NULL) {
						fprintf(stderr, "argument for string param '%s' is required\n", params->params[i].name);
						break;
					} else {
						strcpy(params->params[i].dest, optarg);
						params->params[i].found = 1;
					}
				} else if (params->params[i].type == TYPE_INT) {
					if (optarg == NULL) {
						fprintf(stderr, "argument for int param '%s' is required\n", params->params[i].name);
						break;
					} else {
						int temp = atoi(optarg);
						(*(int*)params->params[i].dest) = temp;
						params->params[i].found = 1;
					}
				} else if (params->params[i].type == TYPE_LONGLONG) {
					if (optarg == NULL) {
						fprintf(stderr, "argument for longlong param '%s' is required\n", params->params[i].name);
						break;
					} else {
						long long temp = atoll(optarg);
						(*(long long*)params->params[i].dest) = temp;
						params->params[i].found = 1;
					}
				} else if (params->params[i].type == TYPE_FLOAT) {
					if (optarg == NULL) {
						fprintf(stderr, "argument for float param '%s' is required\n", params->params[i].name);
						break;
					} else {
						float temp = atof(optarg);
						(*(float*)params->params[i].dest) = temp;
						params->params[i].found = 1;
					}
				} else {
					fprintf(stderr, "error\n");
				}
				break;
			}
		}
		if (i == params->num_params) {
			fprintf(stderr, "ERROR: superfluous param\n");
			abort();
		}
	}
	
	for (i = 0 ; i < params->num_params ; i++) {
		if (params->params[i].required && !params->params[i].found) {
			fprintf(stderr, "%s: ERROR: param '%s' required. ABORTING\n", argv[0], params->params[i].name);
			abort();
		}
	}
	
	free(long_options);
	free(params);
  return 0;
}

