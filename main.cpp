#ifndef TMDB_APIKEY
   #error TMDB_APIKEY NOT DEFINED
#endif
#include <stdio.h>
int main()
{
  printf("API Key: %s\n",TMDB_APIKEY);
}
