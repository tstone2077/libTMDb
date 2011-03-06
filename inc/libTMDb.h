/*
* License: GPL 3.0
* Author: Thurston Stone
*/

#pragma once

#include <string>
#include "sckt.h"
#include "tinyxml.h"
#include "Movie.h"

class TMDb
{
public:
  TMDb(std::string APIKey);
  ~TMDb();
  Movie * SearchForMovie(std::string movie);
private:
  std::string tmdbAPIKey;
};
