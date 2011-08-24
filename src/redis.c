
/*

This requires hiredis which can be downloaded from: https://github.com/antirez/hiredis
I have included the lib and the header file in /ext but if that doesn't function on your platform you'll need to
compile it yourself.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "hiredis.h"

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_OR_OUT_IS_PIPED
#define SCHEME_FUNCTION redis
#include "scheme.h"

int redis(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char ip[300] = "127.0.0.1";
  int port = 6379;
  int perform_flush_db = 0;
  int c;
  while ((c = getopt(argc, argv, "i:p:f")) != -1)
  switch (c)
  {
    case 'i':
      strncpy(ip, optarg, sizeof(ip));
      break;
    case 'p':
      port = atoi(optarg);
      break;
    case 'f':
      perform_flush_db = 1;
      break;
    default:
      abort();
  }
  
  redisReply * reply;
  redisContext * context = redisConnect(ip, port);
  if (context->err)
  {
    fprintf(pipe_err, "%s: %s\n", argv[0], context->errstr);
    return 0;
  }
  
  if (stdout_is_piped()) // reading from redis
  {
    int max_shape_id = 0;
    
    reply = redisCommand(context, "GET next.s.id");
    if (context->err || reply == NULL) { fprintf(pipe_err, "%s: GET next.s.id %s\n", argv[0], context->errstr); return 0; }
    if (reply->type == REDIS_REPLY_INTEGER)     max_shape_id = reply->integer;
    else if (reply->type == REDIS_REPLY_STRING) max_shape_id = atoi(reply->str);
    else
    {
      fprintf(pipe_err, "redis has no next.s.id - likely no shape data in this database.\n");
      return 0;
    }
    freeReplyObject(reply);
     
    int shape_id;
    for (shape_id = 1 ; shape_id < max_shape_id ; shape_id++)
    {
      struct Shape * shape = new_shape();
      
      reply = redisCommand(context, "GET s:%d:unique_set_id", shape_id);
      if (context->err || reply == NULL || reply->type != REDIS_REPLY_STRING) { fprintf(pipe_err, "%s: %s\n", argv[0], context->errstr); return 0; }
      shape->unique_set_id = atoi(reply->str);
      freeReplyObject(reply);
      
      reply = redisCommand(context, "GET s:%d:gl_type", shape_id);
      if (context->err || reply == NULL || reply->type != REDIS_REPLY_STRING) { fprintf(pipe_err, "%s: %s\n", argv[0], context->errstr); return 0; }
      shape->gl_type = atoi(reply->str);
      freeReplyObject(reply);
      
      int va_index;
      for (va_index = 0 ; va_index < shape->num_vertex_arrays ; va_index++)
      {
        reply = redisCommand(context, "KEYS s:%d:va:%d:d:*", shape_id, va_index);
        if (context->err || reply == NULL || reply->type != REDIS_REPLY_ARRAY) { fprintf(pipe_err, "%s: KEYS %s\n", argv[0], context->errstr); return 0; }
        set_num_dimensions(shape, va_index, reply->elements);
        freeReplyObject(reply);
        
        int dim_index;
        for (dim_index = 0 ; dim_index < shape->vertex_arrays[va_index].num_dimensions ; dim_index++)
        {
          reply = redisCommand(context, "LRANGE s:%d:va:%d:d:%d 0 -1", shape_id, va_index, dim_index);
          if (context->err || reply == NULL || reply->type != REDIS_REPLY_ARRAY) { fprintf(pipe_err, "%s: LRANGE %s\n", argv[0], context->errstr); return 0; }
          
          set_num_vertexs(shape, reply->elements);
          
          int v_index;
          for (v_index = 0 ; v_index < reply->elements ; v_index++)
          {
            float * v = get_vertex(shape, va_index, v_index);
            v[dim_index] = atof(reply->element[v_index]->str);
          }
          freeReplyObject(reply);
        }
      }
      
      write_shape(pipe_out, shape);
      free_shape(shape);
    }
  }
  else if (stdin_is_piped()) // writing to redis
  {
    reply = redisCommand(context, "DEL next.s.id");
    if (context->err || reply == NULL || reply->type != REDIS_REPLY_INTEGER) { fprintf(pipe_err, "%s: %s\n", argv[0], context->errstr); return 0; }
    freeReplyObject(reply);
    
    reply = redisCommand(context, "KEYS s:*");
    if (context->err || reply == NULL) { fprintf(pipe_err, "%s: %s\n", argv[0], context->errstr); return 0; }
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements > 0)
    {
      int num_av = 0;
      char ** av = NULL;
      
      num_av++;
      av = (char**)realloc(av, sizeof(char*)*num_av);
      av[num_av-1] = malloc(4);
      sprintf(av[num_av-1], "DEL");
      
      int i;
      for (i = 0 ; i < reply->elements ; i++)
      {
        if (reply->element[i]->type == REDIS_REPLY_STRING)
        {
          num_av++;
          av = (char**)realloc(av, sizeof(char*)*num_av);
          av[num_av-1] = malloc(strlen(reply->element[i]->str)+1);
          strcpy(av[num_av-1], reply->element[i]->str);
        }
      }
      redisReply * replyTemp = redisCommandArgv(context, num_av, (const char **)av, NULL);
      if (context->err || replyTemp == NULL || replyTemp->type != REDIS_REPLY_INTEGER) { fprintf(pipe_err, "%s: DEL %s %s\n", argv[0], reply->element[i]->str, context->errstr); return 0; }
      freeReplyObject(replyTemp);
      
      for (i = 0 ; i < num_av ; i++) free(av[i]); free(av);
      av = NULL; num_av = 0;
    }
    freeReplyObject(reply);
    
    struct Shape * shape = NULL;
    while ((shape = read_shape(pipe_in)))
    {
      int redis_shape_id;
      
      reply = redisCommand(context, "INCR next.s.id");
      if (context->err || reply == NULL || reply->type != REDIS_REPLY_INTEGER) { fprintf(pipe_err, "%s: %s\n", argv[0], context->errstr); return 0; }
      redis_shape_id = reply->integer;
      freeReplyObject(reply);
      
      reply = redisCommand(context, "SET s:%d:unique_set_id %d", redis_shape_id, shape->unique_set_id);
      if (context->err || reply == NULL || reply->type != REDIS_REPLY_STATUS) { fprintf(pipe_err, "%s: %s\n", argv[0], context->errstr); return 0; }
      freeReplyObject(reply);
      
      reply = redisCommand(context, "SET s:%d:gl_type %d", redis_shape_id, shape->gl_type);
      if (context->err || reply == NULL || reply->type != REDIS_REPLY_STATUS) { fprintf(pipe_err, "%s: %s\n", argv[0], context->errstr); return 0; }
      freeReplyObject(reply);
      
      int num_av = 0;
      char ** av = NULL;
      
      num_av++;
      av = (char**)realloc(av, sizeof(char*)*num_av);
      av[num_av-1] = malloc(6);
      sprintf(av[num_av-1], "HMSET");
      
      num_av++;
      av = (char**)realloc(av, sizeof(char*)*num_av);
      av[num_av-1] = malloc(50);
      sprintf(av[num_av-1], "s:%d:attributes", redis_shape_id);
      
      int i;
      for (i = 0 ; i < shape->num_attributes ; i++)
      {
        num_av++;
        av = (char**)realloc(av, sizeof(char*)*num_av);
        av[num_av-1] = malloc(strlen(shape->attributes[i].name));
        strcpy(av[num_av-1], shape->attributes[i].name);
        
        num_av++;
        av = (char**)realloc(av, sizeof(char*)*num_av);
        av[num_av-1] = malloc(shape->attributes[i].value_length+1);
        strcpy(av[num_av-1], shape->attributes[i].value);
      }
      
      reply = redisCommandArgv(context, num_av, (const char**)av, NULL);
      if (context->err || reply == NULL || reply->type != REDIS_REPLY_STATUS) { fprintf(pipe_err, "%s: HMSET shape %d: %s\n", argv[0], redis_shape_id, context->errstr); return 0; }
      freeReplyObject(reply);
      
      for (i = 0 ; i < num_av ; i++) free(av[i]); free(av);
      av = NULL; num_av = 0;
      
      int va_index;
      for (va_index = 0 ; va_index < shape->num_vertex_arrays ; va_index++)
      {
        struct VertexArray * va = &shape->vertex_arrays[va_index];
        
        int dim_index;
        for (dim_index = 0 ; dim_index < va->num_dimensions ; dim_index++)
        {
          reply = redisCommand(context, "DEL s:%d:va:%d:d:%d", redis_shape_id, va_index, dim_index);
          if (context->err || reply == NULL || reply->type != REDIS_REPLY_INTEGER) { fprintf(pipe_err, "%s: DEL: %s\n", argv[0], context->errstr); return 0; }
          freeReplyObject(reply);
        }
        
        for (dim_index = 0 ; dim_index < va->num_dimensions ; dim_index++)
        {
          for (i = 0 ; i < num_av ; i++) free(av[i]); free(av);
          av = NULL; num_av = 0;
          
          num_av++;
          av = (char**)realloc(av, sizeof(char*)*num_av);
          av[num_av-1] = malloc(6);
          sprintf(av[num_av-1], "RPUSH");
          
          num_av++;
          av = (char**)realloc(av, sizeof(char*)*num_av);
          av[num_av-1] = malloc(50);
          sprintf(av[num_av-1], "s:%d:va:%d:d:%d", redis_shape_id, va_index, dim_index);
          
          int v_index;
          for (v_index = 0 ; v_index < shape->num_vertexs ; v_index++)
          {
            float * v = get_vertex(shape, va_index, v_index);
            
            num_av++;
            av = (char**)realloc(av, sizeof(char*)*num_av);
            av[num_av-1] = malloc(20);
            sprintf(av[num_av-1], "%.5f", v[dim_index]);
          }
          
          reply = redisCommandArgv(context, num_av, (const char**)av, NULL);
          if (context->err || reply == NULL || reply->type != REDIS_REPLY_INTEGER) { fprintf(pipe_err, "%s: RPUSH: %s\n", argv[0], context->errstr); return 0; }
          freeReplyObject(reply);
        }
        
        for (i = 0 ; i < num_av ; i++) free(av[i]); free(av);
        av = NULL; num_av = 0;
      }
      
      free_shape(shape);
    }
    reply = redisCommand(context, "SAVE");
    if (context->err || reply == NULL || reply->type != REDIS_REPLY_STATUS) { fprintf(pipe_err, "%s: %s\n", argv[0], context->errstr); return 0; }
    freeReplyObject(reply);
    
  }
  
  redisFree(context);
}
