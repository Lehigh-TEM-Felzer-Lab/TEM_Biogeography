// tem_outvars.h
// Registry of the output variables, in the manner of the diagnostic field registry of FMS: a
// variable is registered once, in code, with its name, long name, units and storage type.
// The built-in variables are listed in tem_outvars.cpp, where users add theirs.
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241109 Created: registry of the output variables.

#ifndef TEM_OUTVARS_H
#define TEM_OUTVARS_H

#include <string>

namespace tem
{
namespace nc
{

// Storage type in the NetCDF file: NC_FLOAT, NC_DOUBLE, NC_INT, NC_INT64
enum Kind
{
  Float32,
  Float64,
  Int32,
  Int64
};

struct OutputVar
{
  std::string name, long_name, units;
  Kind kind;
};

// Register a variable; registering a name again replaces it.
void registerOutput( const std::string& name, const std::string& long_name, const std::string& units,
                     Kind kind = Float32 );

// The registered variable, or NULL.
const OutputVar* findOutput( const std::string& name );

// Registers the variables of tem_outvars.cpp; called once by configureOutput.
void registerBuiltinOutputs();

} // namespace nc
} // namespace tem

#endif
