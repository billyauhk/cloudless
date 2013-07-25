#include <stdio.h>
#include <glob.h>

int main(int argc, const char * argv[]){
  char pattern[100];
  scanf("%s",pattern);

  glob_t globbuf;
  int i = 0;
  globbuf.gl_offs = 0;

  glob(pattern, GLOB_DOOFFS, NULL, &globbuf);
  if(globbuf.gl_pathc != 0){
    while(globbuf.gl_pathv[i] != NULL){
      printf("%s\n",globbuf.gl_pathv[i]);
      i++;
    }
  }
  return 0;
}

