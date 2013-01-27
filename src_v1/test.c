
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "scheme.h"

//#include "produce_unit_circle.c"
//#include "bbox.c"

int main(int argc, char ** argv)
{
  fprintf(stderr, "INFINITY = %d\n", (-INFINITY < 0));
  
  
  return 0;
  /*int fd[2];
  int status = pipe(fd);
  if (status == -1)
  {
    fprintf(stderr, "pipe error\n");
    return 0;
  }*/
  
  //fcntl(fd[0], F_SETFL, O_NONBLOCK);
  
  //char buf1[10] = "lol";
  //char buf2[10] = "      ";
  //write(fd[1], "lol", 3);
  //read(fd[0], buf, 3);
  /*
  FILE * fout = fdopen(fd[1], "w");
  FILE * fin = fdopen(fd[0], "r");
  //FILE * fout = fopen("drat.b", "w");
  
  switch (fork()) {
    case 0:
      fprintf(stderr, "bbox\n");
      bbox(1, NULL, fin, stdout, stderr);
      exit(EXIT_SUCCESS);
    default:
      fprintf(stderr, "produce_unit_circle\n");
      produce_unit_circle(1, NULL, stdin, fout, stderr);
      exit(EXIT_SUCCESS);
  }*/
  
  //fwrite(buf1, sizeof(buf1), 1, fout);
  //fread(buf2, 2, 1, fin);
  
  //fprintf(stderr, "errno = %d (EOF=%d)\n", ferror(fin), feof(fin));
  
  fprintf(stderr, "done\n");
}
