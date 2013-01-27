
#ifndef STD_HELPERS_HEADER
#define STD_HELPERS_HEADER

#include <stdio.h>
#include <stdlib.h>

int stdin_is_piped_t(float timeout)
{
  fd_set rfds;
  struct timeval tv;
  int retval;
  
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  
  tv.tv_sec = floor(timeout);
  tv.tv_usec = (float)floor(timeout / 1000000.0);
  
  if (feof(stdin)) { fprintf(stderr, "feof() on stdin\n"); exit(1); }
  
  if (timeout != 0)
    retval = select(1, &rfds, NULL, NULL, &tv);
  else
    retval = select(1, &rfds, NULL, NULL, NULL);
  
  if (retval == -1) { fprintf(stderr, "select() on stdin failed.\n"); exit(1); }
  
  return (retval != 0);
}
int stdin_is_piped() { return stdin_is_piped_t(0); }

void assert_stdin_is_piped_t(float timeout)
{
  if (!stdin_is_piped_t(timeout))
  {
    fprintf(stderr, "needs a data source. (redirected pipe, using |)\n");
    exit(1);
  }
}
void assert_stdin_is_piped() { assert_stdin_is_piped_t(0); }


int stdout_is_piped()
{
  struct stat outstat = {0};
  struct stat errstat = {0};
  fstat(1, &outstat); // stdout
  fstat(2, &errstat); // stderr
  return (memcmp(&outstat, &errstat, sizeof outstat) != 0); // basically test if stdout == stderr
}

void assert_stdout_is_piped()
{
  if (!stdout_is_piped())
  {
    fprintf(stderr, "needs a data destination, the output is binary and will corrupt your console - try redirecting the output to a file (using [command] > [file])\n");
    exit(1);
  }
}

void assert_stdin_or_out_is_piped()
{
  if (!stdin_is_piped_t(0.2) && !stdout_is_piped())
  {
    fprintf(stderr, "needs a piped source or destination, the output is binary and will corrupt your console - try redirecting the output to a file (using [command] > [file])\n");
    exit(1);
  }
}

#endif
