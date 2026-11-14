// tem_roles.cpp: see tem_roles.h
#include "tem_roles.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>

namespace tem
{
namespace roles
{
namespace
{
struct Role
{
  int cmnt;
  const char* keyword; // a part of the description, compared without regard to case
};

// The communities the code tests by number (ttem_gds.cpp: conversion and harvest fractions, storm damage, nitrogen input).
const Role ROLES[] = { { 1, "ice" },           { 2, "alpine" },          { 3, "wet tundra" },
                       { 4, "boreal" },        { 5, "coniferous" },      { 6, "deciduous" },
                       { 7, "broadleaved" },   { 8, "short grass" },     { 9, "tall grass" },
                       { 10, "mediterranean" }, { 11, "xeric shrub" },   { 12, "xeric woodland" },
                       { 13, "tropical evergreen" }, { 14, "tropical deciduous" } };

std::string lower( std::string s )
{
  std::transform( s.begin(), s.end(), s.begin(), []( unsigned char c ) { return std::tolower( c ); } );
  return s;
}
} // namespace

void checkCommunity( int cmnt, const std::string& description, const std::string& file )
{
  for( const Role& r : ROLES )
  {
    if( r.cmnt != cmnt )
      continue;
    if( lower( description ).find( r.keyword ) == std::string::npos )
    {
      std::cerr << std::endl
                << file << ": community " << cmnt << " is '" << description << "', but the model code treats community " << cmnt
                << " as '" << r.keyword << "' (GDSTEM numbering). Use a parameter set with this numbering, such as params/gdstem, "
                << "or renumber the communities (their values belong to the biology, not the number)." << std::endl;
      std::exit( -1 );
    }
    return;
  }
}
} // namespace roles
} // namespace tem
