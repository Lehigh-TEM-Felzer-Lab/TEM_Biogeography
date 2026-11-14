// tem_roles.h
// The model code tests community numbers directly: ice is 1, tall grassland 9, and so on (the GDSTEM numbering, for example
// the conversion fractions of a land-use change and the storm damage of a community). A parameter set that numbers the
// communities differently would run without an error and give wrong results, so the description of every community that the
// code relies on is checked when the vegetation file is read.
//
// Version: XTEM version 4.5
// 20241111 Created.

#ifndef TEM_ROLES_H
#define TEM_ROLES_H

#include <string>

namespace tem
{
namespace roles
{
// Stops the run with a message when community number cmnt has a description that does not match its role in the code.
// Communities that the code does not test by number are not checked.
void checkCommunity( int cmnt, const std::string& description, const std::string& file );
}
}

#endif
