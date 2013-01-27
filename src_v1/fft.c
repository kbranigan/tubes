
/*

This requires libfftw which can be downloaded from: http://www.fftw.org/
I have included the lib and the header file in /ext but if that doesn't function on your platform you'll need to
compile it yourself.  I built the included file from: http://www.fftw.org/fftw-3.3.tar.gz

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION fft
#include "scheme.h"

int fft(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * filename = NULL;
  int buffer_len = 1024*8;
  int direction = 1; // default = forward
  int cabs_result = 0; // default = no
  int sliding_window_result = 0; // default = do whole shape
  int c;
  while ((c = getopt(argc, argv, "b:f:d:cs")) != -1)
  switch (c)
  {
    case 'b': buffer_len = atoi(optarg); break;
    case 'f': filename = malloc(strlen(optarg)+1); strcpy(filename, optarg); break;
    case 'd': if (strcmp(optarg, "backward")==0) direction = 0;
         else if (strcmp(optarg, "forward")==0) direction = 1;
              break;
    case 'c': cabs_result = 1; break;
    case 's': sliding_window_result = 1; break;
    default: abort(); break;
  }
  
  fftw_plan p;
  struct Shape * shape = NULL;
  while (shape = read_shape(pipe_in))
  {
    complex * in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * shape->num_vertexs);
    complex * out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * shape->num_vertexs);
    
    int i;
    for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      float * v = get_vertex(shape, 0, i);
      in[i] = v[0];
      //fprintf(stderr, "in[%d] = %f\n", i, in[i]);
    }
    
    p = fftw_plan_dft_1d(buffer_len, in, out, direction == 0 ? FFTW_BACKWARD : FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    
    for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      //fprintf(stderr, "out[%d] = %f\n", i, cabs(out[i]));
      float * v = get_vertex(shape, 0, i);
      v[0] = cabs(out[i]);
    }
    
    fftw_free(in);
    fftw_free(out);
    
    write_shape(stdout, shape);
    free_shape(shape);
  }
  /*
  return;
  
  int half_way = 0;
  
  fftw_plan p;
  struct Shape * shape = new_shape();
  while (shape = read_shape(pipe_in))
  {*/
    /*
    if (shape->num_vertexs / 2.0 < buffer_len) { fprintf(stderr, "%s: ERROR: shape provided with too few vertexs to perform FFT\n", argv[0]); break; }
    
    if (get_attribute(shape, "samplerate") == NULL) { fprintf(stderr, "%s: ERROR shape provided without a samplerate\n", argv[0]); break; }
    int samplerate = atoi(get_attribute(shape, "samplerate"));
    
    float input_time_duration = 1.0 / samplerate * buffer_len;
    int index_of_500_hertz = input_time_duration / (1.0 / 500.0);
    
    int i, j = 0, k;
    int fftw_count = 0;
    
    //for (i = 0 ; i < buffer_len ; i++)
    //{
    //  float * v = get_vertex(shape, 0, i + shape->num_vertexs / 2.0); // half way through the file
    //  in[i] = v[0];
    //}
    
    //fftw_count ++;
    //p = fftw_plan_dft_1d(buffer_len, in, out, direction == 0 ? FFTW_BACKWARD : FFTW_FORWARD, FFTW_ESTIMATE);
    //fftw_execute(p);
    
    //float cabs_total = 0;
    //for (i = 1 ; i < buffer_len / 2 ; i++)
    //  cabs_total += cabs(out[i]);
    
    //fprintf(stderr, "cabs_total = %f\n", cabs_total);
    
    //float cabs_total = 0;
    //for (i = 0 ; i < buffer_len / 2 ; i++)
    //{
    //  cabs_total += cabs(out[i]);
    //}
    //fprintf(stderr, "cabs_total = %f\n", cabs_total);
    
    //float cabs_accum = 0;
    //for (i = 0 ; i < buffer_len / 2 ; i++)
    //{
    //  cabs_accum += cabs(out[i]);
    //  
    //  if (cabs_accum > cabs_total / 2.0)
    //    { half_way = i; break; }
    //  
    //}
    //fprintf(stderr, "half_way = %d\n", half_way);
    
    //break;
    
    int limit_num_vertexs = 0;//shape->num_vertexs / 8.0; // kbfu
    
    get_or_add_array(shape, GL_COLOR_ARRAY);
    set_num_dimensions(shape, 1, 4);
    
    float color[4] = { 0, 0, 0, 1 };
    
    for (k = 0 ; k < shape->num_vertexs ; k++)
    {
      if (limit_num_vertexs != 0 && k > limit_num_vertexs) break;
      for (i = 0 ; i < buffer_len ; i++)
      {
        if (i + k >= shape->num_vertexs) break;
        float * v = get_vertex(shape, 0, i + k);//buffer_len);
        in[i] = v[0];
        //dv[0] += v[0];
      }
      
      fftw_count ++;
      //fprintf(stderr, "in: %f %f %f %f %f\n", creal(in[0]), creal(in[1]), creal(in[2]), creal(in[3]), creal(in[4]));
      p = fftw_plan_dft_1d(buffer_len, in, out, direction == 0 ? FFTW_BACKWARD : FFTW_FORWARD, FFTW_ESTIMATE);
      fftw_execute(p);
      //fprintf(stderr, "out: %f %f %f %f %f\n", creal(out[0]), creal(out[1]), creal(out[2]), creal(out[3]), creal(out[4]));
      
      //for (i = 0 ; i < buffer_len ; i++)
      //  fprintf(stderr, "%d: %f %f\n", i, in[i], cabs(out[i]));
      
      //exit(1);
      
      float cabs_total = 0;
      color[0] = 0; color[1] = 0; color[2] = 0;
      for (i = 1 ; i < buffer_len / 2.0 ; i++)
      {
        cabs_total += cabs(out[i]);
        
        if (i < index_of_500_hertz) color[0] += cabs(out[i]);// * (i / 27.0);
        else if (i >= index_of_500_hertz) color[2] += cabs(out[i]);// / (i / 27.0);
        
        //if (i < buffer_len * 0.04) color[0] += cabs(out[i]);       // low pass = red
        //else if (i > buffer_len * 0.11) color[2] += cabs(out[i]);  // high pass = blue
        //else color[1] += cabs(out[i]);                             // mid pass = green
        
        //if (i < buffer_len * first) color[0] += cabs(out[i]);       // low pass = red
        //else if (i > buffer_len * first && 
        //         i < buffer_len * second) color[1] += cabs(out[i]); // mid pass = green
        //else if (i > buffer_len * second &&
        //         i < buffer_len * third) color[2] += cabs(out[i]);  // high pass = blue
      }
      
      //float max = color[0] > color[2] ? color[0] : color[2];
      color[0] /= cabs_total;
      color[2] /= cabs_total;
      
      //fprintf(stderr, "%d: %f %f %f\n", k, cabs_total, color[0], color[2]);
      
      //if (color[0] > 1.0 || color[2] > 1.0)
      //{
      //  for (i = 0 ; i < buffer_len / 2 ; i++)
      //    cabs_total += cabs(out[i]);
      //  //fprintf(stderr, "new cabs_total = %f\n", cabs_total);
      //  color[0] /= cabs_total;
      //  color[2] /= cabs_total;
      //}
      
      //fprintf(stderr, "%d (%f %f)\n", k, color[0], color[2]);
      
      //if (color[2] > max) max = color[2];
      //if (max > 0)
      //{
      //  color[0] = color[0] / max;
      //  color[1] = color[1] / max;
      //  color[2] = color[2] / max;
      //}
      //fprintf(stderr, "max = %f, color = %f,%f,%f\n", max, color[0], color[1], color[2]);
      //exit(1);
      
      for (i = k ; i < k + buffer_len/2 ; i++)
      {
        if (i >= shape->num_vertexs) break;
        //color[1] = (k-i)/(buffer_len/2.0);
        set_vertex(shape, 1, i, color);
      }
      k += buffer_len/2;
      //fprintf(stderr, "dv[0] = %f\n", dv[0]);
    }
    fprintf(stderr, "fftw_count = %d\n", fftw_count);
    */
    /*for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      if (j < buffer_len)
      {
        float * v = get_vertex(shape, 0, i);
        //if (v[0] == 0.0) continue;
        if (shape->vertex_arrays[0].num_dimensions == 2)
          in[j] = v[0] + I * v[1];
        else
          in[j] = v[0];
        
        j++;
      }
    }
    
    p = fftw_plan_dft_1d(buffer_len, in, out, direction == 0 ? FFTW_BACKWARD : FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    
    for (j = 0 ; j < buffer_len ; j++)
    {
      float v[3] = { creal(out[j]), cimag(out[j]), 0 };
      if (cabs_result) v[0] = cabs(out[j]);
      
      //float v[3] = { direction == 0 ? creal(out[j]) : cabs(out[j]) };
      //if (cabs(out[j]) > peak) { peak = cabs(out[j]); peak_index = j; }
      append_vertex(out_shape, v);
    }*/
    /*
    if (limit_num_vertexs != 0) set_num_vertexs(shape, limit_num_vertexs);
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
  free(in);
  free(out);*/
}
