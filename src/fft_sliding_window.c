
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
  int buffer_len = 1024*64;
  int direction = 1; // default = forward
  float half_way = 500.0;
  int c;
  while ((c = getopt(argc, argv, "b:f:d:h:")) != -1)
  switch (c)
  {
    case 'b': buffer_len = atoi(optarg); break;
    case 'f': filename = malloc(strlen(optarg)+1); strcpy(filename, optarg); break;
    case 'd': if (strcmp(optarg, "backward")==0) direction = 0;
         else if (strcmp(optarg, "forward")==0) direction = 1;
              break;
    case 'h': half_way = atof(optarg); break;
    default: abort(); break;
  }
  
  fftw_plan p;
  
  complex * in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * buffer_len);
  complex * out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * buffer_len);
  
  struct Shape * shape = new_shape();
  while (shape = read_shape(pipe_in))
  {
    if (shape->num_vertexs / 2.0 < buffer_len) { fprintf(stderr, "%s: ERROR: shape provided with too few vertexs to perform FFT\n", argv[0]); break; }
    
    if (get_attribute(shape, "samplerate") == NULL) { fprintf(stderr, "%s: ERROR shape provided without a samplerate\n", argv[0]); break; }
    int samplerate = atoi(get_attribute(shape, "samplerate"));
    
    float input_time_duration = 1.0 / samplerate * buffer_len;
    int index_of_half_way_hertz = input_time_duration / (1.0 / half_way);
    
    int i, j, k;
    
    struct Shape * out_shape = new_shape();
    set_num_dimensions(out_shape, 0, shape->vertex_arrays[0].num_dimensions);
    set_num_vertexs(out_shape, shape->num_vertexs / (buffer_len / 2.0) + 1);
    out_shape->gl_type = GL_LINE_STRIP;
    for (i = 0 ; i < shape->num_attributes ; i++)
      set_attribute(out_shape, shape->attributes[i].name, shape->attributes[i].value);
    
    for (j = 0 ; j < shape->vertex_arrays[0].num_dimensions ; j++)
    {
      for (k = 0 ; k < shape->num_vertexs ; )
      {
        for (i = 0 ; i < buffer_len ; i++)
        {
          if (i + k >= shape->num_vertexs) break;
          float * v = get_vertex(shape, 0, i + k);
          in[i] = v[j];
        }
        
        p = fftw_plan_dft_1d(buffer_len, in, out, direction == 0 ? FFTW_BACKWARD : FFTW_FORWARD, FFTW_ESTIMATE);
        fftw_execute(p);
        
        float cabs_below_half_way = 0;
        float cabs_above_half_way = 0;
        for (i = 1 ; i < buffer_len / 2.0 ; i++)
        {
          if (i < index_of_half_way_hertz) cabs_below_half_way += cabs(out[i]);
          else if (i >= index_of_half_way_hertz) cabs_above_half_way += cabs(out[i]);
        }
        
        float * v = get_vertex(out_shape, 0, k / (buffer_len / 2));
        v[j] = cabs_below_half_way / cabs_above_half_way;
        
        //float v[5] = { cabs_below_half_way / cabs_above_half_way, 0, 0, 0, 0 };
        //set_vertex(out_shape, v);
        
        k += buffer_len / 2;
      }
    }
    
    write_shape(pipe_out, out_shape);
    free_shape(out_shape);
    free_shape(shape);
  }
  free(in);
  free(out);
}
