
/*

This requires sndfile which can be downloaded from: http://www.mega-nerd.com/libsndfile/
I have included the lib and the header file in /ext but if that doesn't function on your platform you'll need to
compile it yourself.  I built the included file from: http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.25.tar.gz

*/

#include <math.h>
#include <time.h>
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
  time_t start = time(NULL);
  
  int max_channels = MAX_CHANNELS;
  char * filename = NULL;
  int fft = 0;
  int normalize = 0;
  int output_size = 0;
  float low_pass = 400.0;
  float high_pass = 2000.0;
  int c;
  while ((c = getopt(argc, argv, "f:c:tno:l:h:")) != -1)
  switch (c)
  {
    case 'f': filename = malloc(strlen(optarg)+1); strcpy(filename, optarg); break;
    case 'c': max_channels = atoi(optarg); break;
    case 't': fft = 1; break;
    case 'n': normalize = 1; break;
    case 'o': output_size = atoi(optarg); break;
    case 'l': low_pass = atof(optarg); break;
    case 'h': high_pass = atof(optarg); break;
    default: abort(); break;
  }
  
  SNDFILE * infile;
  
  SF_INFO sfinfo;
  int readcount;
  
  if (filename == NULL) { fprintf(pipe_err, "Usage: %s -f [filename.wav]\n", argv[0]); return 1; }
  if (!(infile = sf_open(filename, SFM_READ, &sfinfo))) { fprintf(pipe_err, "Not able to open input file '%s'\n%s\n", filename, sf_strerror(NULL)); return 1; }
  if (sfinfo.channels > MAX_CHANNELS) { fprintf(pipe_err, "Not able to process sound files with more than %d channels\n", MAX_CHANNELS); return 1; }
  if (sfinfo.channels > max_channels) { fprintf(pipe_err, "Warning: input file has %d channels, max_channels set to %d. (truncating the difference)\n", sfinfo.channels, max_channels); }
  if (sfinfo.frames > 1024*1024 && output_size == 0) { fprintf(pipe_err, "%s: the supplied file has more then 1m samples (%lld infact), that's too many.  Provide -o [1000] to control output, -t to fft, -n to normalize.\n", argv[0], sfinfo.frames); return 1; }
  if (sfinfo.frames > 1024*1024*50 && fft) { fprintf(pipe_err, "%s: Warning: the supplied file has more then 50m samples, that's a huge amount.  This process may take upwards of 5 to 10 minutes to complete with -t (fft) active.\n", argv[0]); }
  
  int i;
  struct Shape ** shapes = (struct Shape**)malloc(sizeof(struct Shape*)*max_channels);
  for (i = 0 ; i < max_channels ; i++)
  {
    shapes[i] = new_shape();
    shapes[i]->gl_type = GL_LINE_STRIP;
  
    char temp[50];
    sprintf(temp, "%d", sfinfo.samplerate);
    set_attribute(shapes[i], "samplerate", temp);
    set_attribute(shapes[i], "original_filename", filename);
    sprintf(temp, "%d", i);
    set_attribute(shapes[i], "original_channel", temp);
    
    struct VertexArray * va = get_or_add_array(shapes[i], GL_VERTEX_ARRAY);
    if (fft) get_or_add_array(shapes[i], GL_COLOR_ARRAY);
    
    set_num_dimensions(shapes[i], 0, 1);
    set_num_vertexs(shapes[i], output_size);
  }
  
  #ifdef DEBUG
  fprintf(pipe_err, "%s: music file has %ld frames, %d channels\n", argv[0], (long)sfinfo.frames, sfinfo.channels);
  #endif
  
  int count = 0;
  float v[MAX_CHANNELS];
  if (output_size)
  {
    int num_samples_in_each_window = ceil((float)sfinfo.frames / (float)output_size);
    
    float input_time_duration = 1.0 / sfinfo.samplerate * num_samples_in_each_window;
    int index_of_low_pass_hertz = input_time_duration / (1.0 / low_pass);
    int index_of_high_pass_hertz = input_time_duration / (1.0 / high_pass);
    
    char value[50];
    sprintf(value, "%d", num_samples_in_each_window);
    for (i = 0 ; i < max_channels ; i++)
      set_attribute(shapes[i], "samples per vertex", value);
    
    if (output_size)
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
          for (i = 0 ; i < max_channels ; i++)
            v[i] += data[j+i] * data[j+i];
          j += sfinfo.channels;
        }
        
        for (i = 0 ; i < max_channels ; i++)
          v[i] = sqrt(v[i] * (1.0 / num_samples_in_each_window));
        
        for (i = 0 ; i < max_channels ; i++)
          set_vertex(shapes[i], 0, count, &v[i]);
        
        if (fft)
        {
          for (i = 0 ; i < max_channels ; i++)
          {
            int d = 0;
            int j = 0;
            while (j < readcount)
            {
              in[d] = data[j+i] * data[j+i]; // power
              j += sfinfo.channels;
              d++;
            }
            
            fftw_execute(p);
            
            float ratio[3] = { 0, 0, 0 };
            
            for (j = 1 ; j < index_of_low_pass_hertz ; j++)
              ratio[0] += cabs(out[j]);
            for (j = index_of_low_pass_hertz ; j < index_of_high_pass_hertz ; j++)
              ratio[1] += cabs(out[j]);
            for (j = index_of_high_pass_hertz ; j < (num_samples_in_each_window / 2.0) ; j++)
              ratio[2] += cabs(out[j]);
            
            float * v = get_vertex(shapes[i], 1, count);
            v[0] = ratio[0] / (ratio[0] + ratio[1] + ratio[2]);
            if (isnan(v[0])) v[0] = 0;
            v[1] = ratio[1] / (ratio[0] + ratio[1] + ratio[2]);
            if (isnan(v[1])) v[1] = 0;
            v[2] = ratio[2] / (ratio[0] + ratio[1] + ratio[2]);
            if (isnan(v[2])) v[2] = 0;
          }
        }
        if (count % (output_size / 20) == 0) fprintf(stderr, "%d%% complete in %ld seconds\n", (int)round(count / (float)output_size * 100.0), time(NULL) - start);
        count++;
      }
      
      fftw_destroy_plan(p);
      fftw_free(in);
      fftw_free(out);
      free(data);
    }
    
    if (normalize)
    {
      struct BBox * bbox = NULL;
      for (i = 0 ; i < max_channels ; i++)
        bbox = get_bbox(shapes[i], bbox);
      
      for (i = 0 ; i < max_channels ; i++)
      {
        int j;
        for (j = 0 ; j < shapes[i]->num_vertexs ; j++)
        {
          int k;
          float * v = get_vertex(shapes[i], 0, j);
          for (k = 0 ; k < bbox->num_minmax ; k++)
            v[k] = (v[k] - bbox->minmax[k].min) / (bbox->minmax[k].max - bbox->minmax[k].min);
        }
      }
      
      free_bbox(bbox);
    }
  }
  else
  {
    fprintf(stderr, "dah\n");
    /*static double data[BUFFER_LEN];
    
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
    }*/
  }
  for (i = 0 ; i < max_channels ; i++)
  {
    write_shape(pipe_out, shapes[i]);
    free_shape(shapes[i]);
  }
  free(shapes);
  fprintf(pipe_err, "%s: executed in about %ld seconds\n", argv[0], time(NULL) - start);
}
