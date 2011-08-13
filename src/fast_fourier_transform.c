
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
#define SCHEME_FUNCTION fast_fourier_transform
#include "scheme.h"

int fast_fourier_transform(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * filename = NULL;
  int buffer_len = 1024;
  int direction = 1; // default = forward
  int cabs_result = 0; // default = no
  int c;
  while ((c = getopt(argc, argv, "b:f:d:c")) != -1)
  switch (c)
  {
    case 'b':
      buffer_len = atoi(optarg);
      break;
    case 'f':
      filename = malloc(strlen(optarg)+1);
      strcpy(filename, optarg);
      break;
    case 'd':
      if (strcmp(optarg, "backward")==0) direction = 0;
      if (strcmp(optarg, "forward")==0) direction = 1;
      break;
    case 'c':
      cabs_result = 1;
      break;
    default:
      abort();
      break;
  }
  
  complex * in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * buffer_len);
  complex * out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * buffer_len);
  
  fftw_plan p;
  struct Shape * shape = new_shape();
  while (shape = read_shape(pipe_in))
  {
    int i, j = 0;
    
    for (i = 0 ; i < shape->num_vertexs ; i++)
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
    
    struct Shape * out_shape = new_shape();
    for (i = 0 ; i < shape->num_attributes ; i++)
      set_attribute(out_shape, shape->attributes[i].name, shape->attributes[i].value);
    
    if (cabs_result || direction == 0)
      set_num_dimensions(out_shape, 0, 1);
    
    out_shape->gl_type = GL_LINE_STRIP;
    for (j = 0 ; j < buffer_len ; j++)
    {
      float v[3] = { creal(out[j]), cimag(out[j]), 0 };
      if (cabs_result) v[0] = cabs(out[j]);
      
      //float v[3] = { direction == 0 ? creal(out[j]) : cabs(out[j]) };
      //if (cabs(out[j]) > peak) { peak = cabs(out[j]); peak_index = j; }
      append_vertex(out_shape, v);
    }
    
    write_shape(pipe_out, out_shape);
    free_shape(out_shape);
    
    j = 0;
    free_shape(shape);
    break;
  }
  free(in);
  free(out);
}

/*
// cc hehe.c -Isrc -lsndfile -Lsrc/.libs/ ; ./a.out
// cc hehe.c -Isrc -lsndfile -Lsrc/.libs/ -lfftw3 -lm ; ./a.out | ~/work/pipes/read_csv | ~/work/pipes/write_png

#define		BUFFER_LEN	4410//10024
#define		MAX_CHANNELS	6

int main(int argc, char * argv)
{
  static double data[BUFFER_LEN];
  SNDFILE *infile;//, *outfile;
  
  SF_INFO	sfinfo;
  int	readcount;
  //const char *infilename = "examples/sine.wav";
  //const char *infilename = "Mists_of_Time-4T.wav";
  const char *infilename = "Morning.wav";
  
  if (!(infile = sf_open(infilename, SFM_READ, &sfinfo)))
  {
    printf ("Not able to open input file %s.\n", infilename);
    puts (sf_strerror(NULL));
    return 1;
  }
  
  if (sfinfo.channels > MAX_CHANNELS)
  {
    printf ("Not able to process more than %d channels\n", MAX_CHANNELS);
    return 1;
  }
  
  //printf("sfinfo.samplerate = %d\n", sfinfo.samplerate);
  //printf("sfinfo.channels = %d\n", sfinfo.channels);
  
  //printf("drop table freq;\n");
  //printf("create table freq (id int primary key auto_increment, i int, a float(15,5));\n");
  
  complex * in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * BUFFER_LEN);
  complex * out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * BUFFER_LEN);
  
  fftw_plan p;
  long count = 0;
  while ((readcount = sf_read_double(infile, data, BUFFER_LEN)))
  {
    int total = 0;
    int k;
    //for (k = 0 ; k < BUFFER_LEN ; k ++)
    //  total += data[k];
        
    //if (total == 0)
    //  continue;
    
    for (k = 0 ; k < readcount ; k += sfinfo.channels)
    {
      in[k/sfinfo.channels] = data[k];
    }
    
    p = fftw_plan_dft_1d(readcount/sfinfo.channels, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);
    
    float max_in = -10000;
    float max_out = -10000;
    for (k = 0 ; k < readcount/sfinfo.channels ; k++)
    {
      if (cabs(in[k]) > max_in) max_in = cabs(in[k]);
      if (cabs(out[k]) > max_out) max_out = cabs(out[k]);
    }
    
    fprintf(stderr, "max_in = %g\n", max_in);
    fprintf(stderr, "max_out = %g\n", max_out);
    
    //printf("INSERT INTO freq (i, a) values ");
    for (k = 0 ; k < readcount/sfinfo.channels ; k++)
    {
      printf("%g,%g,1\n", k / (float)(readcount/sfinfo.channels) * max_in, cabs(in[k]), k);
      //printf("%g,%g,1\n", k / (float)(readcount/sfinfo.channels) * max_out, cabs(out[k]), k);
    }
    //printf("\n");
    
    count += readcount;
    fprintf(stderr,"readcount = %d\n", readcount);
    break;
    if (count > 44100)
      break;
    //int chan, k;
    //for (chan = 0 ; chan < sfinfo.channels ; chan++)
    //  for (k = chan ; k < count ; k += sfinfo.channels)
    //    count++;
    //process_data(data, readcount, sfinfo.channels);
    //sf_write_double(outfile, data, readcount);
  }
  //printf("count = %ld\n", count);
  fftw_destroy_plan(p);
  
  sf_close(infile);

}
*/