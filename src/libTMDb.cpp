/*
* License: GPL 3.0
* Author: Thurston Stone
*/
#include "libTMDb.h"
#include "Movie.h"
#include <string>

TMDb::TMDb(std::string APIKey)
{
   tmdbAPIKey = APIKey;
}

TMDb::~TMDb()
{
}

Movie * TMDb::SearchForMovie(std::string movie)
{
   Movie * m;
   return m;
}
