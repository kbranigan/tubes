
#include <stdio.h>
#include "../src/scheme.h"

#include <ruby.h>

#ifdef RUBY_RUBY_H // ruby 1.9.2
#include <ruby/io.h>
#define GET_STDIO_FILE(rfile) rb_io_stdio_file(RFILE(rfile)->fptr)

#else // ruby 1.8.7
#include <rubyio.h>
#define GET_STDIO_FILE(rfile) RFILE(rfile)->fptr->f
#endif

#ifdef HAVE_RUBY_ENCODING_H
# include <ruby/encoding.h>
# define ENCODED_STR_NEW2(str, encoding) \
   ({ \
    VALUE _string = rb_str_new2((const char *)str); \
     int _enc = rb_enc_find_index(encoding); \
     rb_enc_associate_index(_string, _enc); \
     _string; \
   })
#else
# define ENCODED_STR_NEW2(str, encoding) \
   rb_str_new2((const char *)str)
#endif

VALUE rb_shape_new(struct Shape * shape)
{
  VALUE r = rb_hash_new();
  
  rb_hash_aset(r, ENCODED_STR_NEW2("unique_set_id", "ASCII-8BIT"), INT2NUM(shape->unique_set_id));
  rb_hash_aset(r, ENCODED_STR_NEW2("version", "ASCII-8BIT"), INT2NUM(shape->version));
  rb_hash_aset(r, ENCODED_STR_NEW2("gl_type", "ASCII-8BIT"), INT2NUM(shape->gl_type));
  
  if (shape->num_attributes > 0)
  {
    VALUE attrs = rb_ary_new();
    uint32_t i;
    for (i = 0 ; i < shape->num_attributes ; i++)
    {
      VALUE row = rb_ary_new();
      rb_ary_push(row, ENCODED_STR_NEW2(shape->attributes[i].name, "ASCII-8BIT"));
      rb_ary_push(row, ENCODED_STR_NEW2(shape->attributes[i].value, "ASCII-8BIT"));
      rb_ary_push(attrs, row);
    }
    rb_hash_aset(r, ENCODED_STR_NEW2("attributes", "ASCII-8BIT"), attrs);
  }
  
  if (shape->num_vertexs > 0 && shape->num_vertex_arrays > 0)
  {
    VALUE arrays = rb_hash_new();
    uint32_t i;
    for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    {
      VALUE array = rb_hash_new();
      VALUE vertexs = rb_ary_new();
      
      uint32_t j;
      for (j = 0 ; j < shape->num_vertexs ; j++)
      {
        VALUE vertex = rb_ary_new();
        float * v = get_vertex(shape, i, j);
        
        uint32_t k;
        for (k = 0 ; k < shape->vertex_arrays[i].num_dimensions ; k++)
          rb_ary_push(vertex, rb_float_new(*(v+k)));
        
        rb_ary_push(vertexs, vertex);
      }
      rb_hash_aset(array, ENCODED_STR_NEW2("vertexs", "ASCII-8BIT"), vertexs);
      rb_hash_aset(array, ENCODED_STR_NEW2("num_dimensions", "ASCII-8BIT"), INT2NUM(shape->vertex_arrays[i].num_dimensions));
      rb_hash_aset(arrays, INT2NUM(shape->vertex_arrays[i].array_type), array);
    }
    rb_hash_aset(r, ENCODED_STR_NEW2("vertex_arrays", "ASCII-8BIT"), arrays);
  }
  return r;
}

static int foreach_array(VALUE key, VALUE val, struct Shape * shape)
{
  int array_type = 0;
  if (TYPE(key) == T_FIXNUM)      array_type = NUM2INT(key);
  else if (TYPE(key) == T_STRING) array_type = atoi(RSTRING_PTR(key));
  if (shape == NULL || (long)shape == Qnil) { fprintf(stderr, "foreach_array expects a shape, provided NULL\n"); return 0; }
  
  VALUE vertexs = rb_hash_aref(val, ENCODED_STR_NEW2("vertexs", "ASCII-8BIT"));
  long int num_vertexs = RARRAY_LEN(vertexs);
  if (shape->num_vertexs != 0 && shape->num_vertexs != num_vertexs) { fprintf(stderr, "foreach_array received an array with %ld vertexs, however the shape had %d vertexs. (They should be the same)\n", RARRAY_LEN(vertexs), shape->num_vertexs); return 0; }
  
  int num_dimensions = NUM2INT(rb_hash_aref(val, ENCODED_STR_NEW2("num_dimensions", "ASCII-8BIT")));
  
  float f[4] = {0,0,0,0};
  int i;
  for (i = 0 ; i < RARRAY_LEN(vertexs) ; i++)
  {
    VALUE v = rb_ary_entry(vertexs, i);
    int j = 0;
    for (j = 0 ; j < num_dimensions ; j++)
      f[j] = (float)rb_num2dbl(rb_ary_entry(v, j));
    
    append_vertex(shape, f);
  }
  return 0;
}

struct Shape * shape_rb_to_shape(VALUE shape_rb)
{
  if (TYPE(shape_rb) != T_HASH) rb_raise(rb_eArgError, "rb_shape_to_shape() expects a Hash");
  
  struct Shape * shape = new_shape();
  VALUE v;
  
  v = rb_hash_aref(shape_rb, ENCODED_STR_NEW2("unique_set_id", "ASCII-8BIT"));
  if (TYPE(v) != T_FIXNUM) rb_raise(rb_eArgError, "unique_set_id is not provided or is not a Fixnum");
  shape->unique_set_id = NUM2INT(v);
  
  v = rb_hash_aref(shape_rb, ENCODED_STR_NEW2("version", "ASCII-8BIT"));
  if (TYPE(v) != T_FIXNUM) rb_raise(rb_eArgError, "version is not provided or is not a Fixnum");
  shape->version = NUM2INT(v);
  
  v = rb_hash_aref(shape_rb, ENCODED_STR_NEW2("gl_type", "ASCII-8BIT"));
  if (TYPE(v) != T_FIXNUM) rb_raise(rb_eArgError, "gl_type is not provided or is not a Fixnum");
  shape->gl_type = NUM2INT(v);
  
  VALUE arrays = rb_hash_aref(shape_rb, ENCODED_STR_NEW2("vertex_arrays", "ASCII-8BIT"));
  if (TYPE(arrays) != T_HASH) rb_raise(rb_eArgError, "write_shape() expects a hash with a 'vertex_arrays' key containing a hash (was %d)", TYPE(arrays));
  rb_hash_foreach(arrays, foreach_array, (unsigned long)shape);
  
  VALUE attributes = rb_hash_aref(shape_rb, ENCODED_STR_NEW2("attributes", "ASCII-8BIT"));
  if (TYPE(attributes) != T_ARRAY) rb_raise(rb_eArgError, "write_shape() expects a hash with a 'attributes' key containing an array (was %d)", TYPE(attributes));
  int i;
  for (i = 0 ; i < RARRAY_LEN(attributes) ; i++)
  {
    VALUE v = rb_ary_entry(attributes, i);
    if (TYPE(v) != T_ARRAY) continue;
    
    char * name = RSTRING_PTR(rb_ary_entry(v, 0));
    char * value = RSTRING_PTR(rb_ary_entry(v, 1));
    set_attribute(shape, name, value);
  }
  
  return shape;
}

VALUE method_write_shape(VALUE self, VALUE fp, VALUE shape_rb)
{
  if (TYPE(fp) != T_FILE) rb_raise(rb_eArgError, "write_shape() TYPE(fp) = '%d' (in C)", TYPE(fp));
  if (TYPE(shape_rb) != T_HASH) rb_raise(rb_eArgError, "write_shape() expects a file pointer like STDIN and a shape");
  
  struct Shape * shape = shape_rb_to_shape(shape_rb);
  VALUE shape_again = rb_shape_new(shape);
  
  write_shape(GET_STDIO_FILE(fp), shape);
  free_shape(shape);
}

VALUE method_read_one(VALUE self, VALUE fp)
{
  struct Shape * shape = NULL;
  shape = read_shape(GET_STDIO_FILE(fp));
  VALUE shape_rb = rb_shape_new(shape);
  free_shape(shape);
  return shape_rb;
}

VALUE method_read_each(VALUE self, VALUE fp)
{
  struct Shape * shape = NULL;
  while ((shape = read_shape(GET_STDIO_FILE(fp))))
  {
    rb_yield(rb_shape_new(shape));
    free_shape(shape);
  }
}

VALUE method_new_shape(VALUE self)
{
  struct Shape * shape = new_shape();
  VALUE shape_rb = rb_shape_new(shape);
  free_shape(shape);
  return shape_rb;
}

VALUE method_assert_stdin_is_piped(VALUE self)
{
  assert_stdin_is_piped();
}

VALUE method_assert_stdout_is_piped(VALUE self)
{
  assert_stdout_is_piped();
}

void Init_Pipes()
{
  VALUE Pipes_module = rb_define_module("Pipes");
  rb_define_singleton_method(Pipes_module, "new_shape", method_new_shape, 0);
  rb_define_singleton_method(Pipes_module, "read_one", method_read_one, 1);
  rb_define_singleton_method(Pipes_module, "read_each", method_read_each, 1);
  rb_define_singleton_method(Pipes_module, "write", method_write_shape, 2);
  rb_define_singleton_method(Pipes_module, "assert_stdin_is_piped", method_assert_stdin_is_piped, 0);
  rb_define_singleton_method(Pipes_module, "assert_stdout_is_piped", method_assert_stdout_is_piped, 0);
}
