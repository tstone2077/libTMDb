#pragma once

#include <string>
//#include <vector>

class Movie
{
public:
   Movie();
   ~Movie();
private:
   double score;
   int popularity;
   bool translated;
   bool adult;
   std::string language;
   std::string originalName;
   std::string name;
   std::string alternativeName;
   std::string type;
   int id;
   std::string imdb_id;
   std::string url;
   int votes;
   double rating;
   std::string certification;
   std::string overview;
   std::string released;
   //std::vector<std::string> imageURLs;
   std::string lastModified;
   int version;
};
