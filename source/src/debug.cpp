#include "debug.h"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

int Debug_Helper::n = 0; 

static FILE * lfile; 

bool log_init(const char * filename) {
  lfile = fopen(filename, "w"); 
  return lfile;
}

void log(char * cat, char * format, ...) {
  va_list args; 
  if (lfile) {
    va_start (args, format);
    fprintf(lfile, "%s\t", cat); 
    vfprintf(lfile, format, args); 
    fprintf(lfile, "\n"); 
    va_end (args);
  }
}


