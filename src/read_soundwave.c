
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

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_soundwave
#include "scheme.h"

#define	BUFFER_LEN    1024  
#define	MAX_CHANNELS  6

int read_soundwave(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int max_channels = 6;
  char * filename = NULL;
  int c;
  while ((c = getopt(argc, argv, "f:c:")) != -1)
  switch (c)
  {
    case 'f':
      filename = malloc(strlen(optarg)+1);
      strcpy(filename, optarg);
      break;
    case 'c':
      max_channels = atoi(optarg);
      break;
  }
  
  static double data[BUFFER_LEN];
  SNDFILE *infile;
  
  SF_INFO	sfinfo;
  int	readcount;
  
  if (filename == NULL) { fprintf(pipe_err, "Usage: %s -f [filename.wav]\n", argv[0]); return 1; }
  if (!(infile = sf_open(filename, SFM_READ, &sfinfo))) { fprintf(pipe_err, "Not able to open input file '%s'\n%s\n", filename, sf_strerror(NULL)); return 1; }
  if (sfinfo.channels > MAX_CHANNELS) { fprintf(pipe_err, "Not able to process sound files with more than %d channels\n", MAX_CHANNELS); return 1; }
  if (sfinfo.channels > max_channels) { fprintf(pipe_err, "Warning: input file has %d channels, max_channels set to %d. (truncating the difference)\n", sfinfo.channels, max_channels); }
  
  struct Shape * shape = new_shape();
  shape->gl_type = GL_LINE_STRIP;
  set_num_vertexs(shape, sfinfo.frames);
  set_num_dimensions(shape, 0, (sfinfo.channels > max_channels) ? max_channels : sfinfo.channels);
  
  char temp[50];
  sprintf(temp, "%d", sfinfo.samplerate);
  set_attribute(shape, "samplerate", temp);
  set_attribute(shape, "original_filename", filename);
  
  #ifdef DEBUG
  fprintf(pipe_err, "%s: music file has %ld frames, %d channels\n", argv[0], (long)sfinfo.frames, sfinfo.channels);
  #endif
  
  struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
  int i = 0, k = 0;
  int count = 0;
  while ((readcount = sf_read_double(infile, data, BUFFER_LEN)))
  {
    double total = 0;
    k = 0;
    while (k < readcount)
    {
      float v[MAX_CHANNELS];
      for (i = 0 ; i < va->num_dimensions ; i++)
        v[i] = data[k+i];
      
      set_vertex(shape, 0, count, v);
      count++;
      k += sfinfo.channels;
    }
  }
  write_shape(pipe_out, shape);
  free_shape(shape);
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