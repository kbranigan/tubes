
/*

This requires sndfile which can be downloaded from: http://www.mega-nerd.com/libsndfile/
I have included the lib and the header file in /ext but if that doesn't function on your platform you'll need to
compile it yourself.  I built the included file from: http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.25.tar.gz

*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <complex.h>
#include <sndfile.h>
#include <fftw3.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_soundwave
#include "scheme.h"

#define BUFFER_LEN    1024  
#define MAX_CHANNELS  12

int read_soundwave(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int max_channels = MAX_CHANNELS;
  char * filename = NULL;
  int rms = 0;
  int fft = 0;
  int normalize = 0;
  int output_size = 0;
  float half_way = 500.0;
  int c;
  while ((c = getopt(argc, argv, "f:c:rtno:h:")) != -1)
  switch (c)
  {
    case 'f': filename = malloc(strlen(optarg)+1); strcpy(filename, optarg); break;
    case 'c': max_channels = atoi(optarg); break;
    case 'r': rms = 1; break;
    case 't': fft = 1; break;
    case 'n': normalize = 1; break;
    case 'o': output_size = atoi(optarg); break;
    case 'h': half_way = atof(optarg); break;
    default: abort(); break;
  }
  
  SNDFILE * infile;
  
  SF_INFO sfinfo;
  int readcount;
  
  if (filename == NULL) { fprintf(pipe_err, "Usage: %s -f [filename.wav]\n", argv[0]); return 1; }
  if (!(infile = sf_open(filename, SFM_READ, &sfinfo))) { fprintf(pipe_err, "Not able to open input file '%s'\n%s\n", filename, sf_strerror(NULL)); return 1; }
  if (sfinfo.channels > MAX_CHANNELS) { fprintf(pipe_err, "Not able to process sound files with more than %d channels\n", MAX_CHANNELS); return 1; }
  if (sfinfo.channels > max_channels) { fprintf(pipe_err, "Warning: input file has %d channels, max_channels set to %d. (truncating the difference)\n", sfinfo.channels, max_channels); }
  if (sfinfo.frames > 1024*1024 && output_size == 0) { fprintf(pipe_err, "%s: the supplied file has more then 1m samples, that's too many.  Provide '-r' to summarize.\n", argv[0]); return 1; }
  
  struct Shape * shape = new_shape();
  shape->gl_type = GL_LINE_STRIP;
  
  char temp[50];
  sprintf(temp, "%d", sfinfo.samplerate);
  set_attribute(shape, "samplerate", temp);
  set_attribute(shape, "original_filename", filename);
  
  #ifdef DEBUG
  fprintf(pipe_err, "%s: music file has %ld frames, %d channels\n", argv[0], (long)sfinfo.frames, sfinfo.channels);
  #endif
  
  int count = 0;
  float v[MAX_CHANNELS];
  if (output_size)
  {
    int num_samples_in_each_window = ceil((float)sfinfo.frames / (float)output_size);
    
    float input_time_duration = 1.0 / sfinfo.samplerate * num_samples_in_each_window;
    int index_of_half_way_hertz = input_time_duration / (1.0 / half_way);
    
    struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
    struct VertexArray * cva = get_or_add_array(shape, GL_COLOR_ARRAY);
    
    set_num_dimensions(shape, 0, (sfinfo.channels > max_channels) ? max_channels : sfinfo.channels);
    set_num_vertexs(shape, output_size);
    
    char value[50];
    sprintf(value, "%d", num_samples_in_each_window);
    set_attribute(shape, "samples per vertex", value);
    
    if (rms)
    {
      double * data = (double*)malloc(sizeof(double) * num_samples_in_each_window * sfinfo.channels);
      
      fftw_plan p;
      complex * in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * num_samples_in_each_window);
      complex * out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * num_samples_in_each_window);
      p = fftw_plan_dft_1d(num_samples_in_each_window, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
      
      int count = 0;
      while ((readcount = sf_read_double(infile, data, num_samples_in_each_window * sfinfo.channels)))
      {
        memset(v, 0, sizeof(v));
        
        int j = 0;
        while (j < readcount)
        {
          int d;
          for (d = 0 ; d < sfinfo.channels ; d++)
            v[d] += data[j+d] * data[j+d];
          j += sfinfo.channels;
        }
        
        for (j = 0 ; j < sfinfo.channels ; j++)
          v[j] = sqrt(v[j] * (1.0 / num_samples_in_each_window));
        
        set_vertex(shape, 0, count, v);
        
        if (fft)
        {
          int d = 0;
          int j = 0;
          while (j < readcount)
          {
            in[d] = data[j];// * data[j+d];
            j += sfinfo.channels;
            d++;
          }
          
          fftw_execute(p);
          
          float ratio[2] = { 0, 0 };
          
          int i;
          for (i = 1 ; i < num_samples_in_each_window / 2.0 ; i++)
            ratio[(i < index_of_half_way_hertz)] += cabs(out[i]);
          
          float * v = get_vertex(shape, 1, count);// / (num_samples_in_each_window / 2.0));
          v[0] = ratio[0] / (ratio[0] + ratio[1]);
          v[1] = ratio[1] / (ratio[0] + ratio[1]);
        }
        count++;
      }
      
      fftw_free(in);
      fftw_free(out);
      free(data);
    }
    
    if (normalize)
    {
      fprintf(pipe_err, "%s: Normalizing result\n", argv[0]);
      
      int i, k;
      struct BBox * bbox = get_bbox(shape, NULL);
      for (i = 0 ; i < shape->num_vertexs ; i++)
      {
        float * v = get_vertex(shape, 0, i);
        for (k = 0 ; k < va->num_dimensions ; k++)
          v[k] = (v[k] - bbox->minmax[k].min) / (bbox->minmax[k].max - bbox->minmax[k].min);
      }
      free_bbox(bbox);
    }
  }
  else
  {
    static double data[BUFFER_LEN];
    
    struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
    set_num_vertexs(shape, sfinfo.frames);
    set_num_dimensions(shape, 0, (sfinfo.channels > max_channels) ? max_channels : sfinfo.channels);
    
    while ((readcount = sf_read_double(infile, data, BUFFER_LEN)))
    {
      int k = 0;
      while (k < readcount)
      {
        int i;
        for (i = 0 ; i < va->num_dimensions ; i++)
          v[i] = data[k+i];
        
        set_vertex(shape, 0, count, v);
        count++;
        k += sfinfo.channels;
      }
    }
  }
  write_shape(pipe_out, shape);
  free_shape(shape);
}
