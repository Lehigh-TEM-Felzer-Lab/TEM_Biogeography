// tem_invars.cpp
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241109 Created: which file and variable supplies each model input (varmap): one variable name per entry;
//          sand, silt and clay, storm and hurricane are separate entries.
// 20241110 Changed: model_resolution regrids the inputs that are not on the land-cover grid.
// 20241111 Changed: no regridding in the model: every input must be on the grid of the land-cover file, or the run
//          stops with a message (regrid before the run with cdo or xesmf). model_resolution, regrid_method and
//          regrid_dir are refused.
// 20241111 Changed: the fire inputs are LIGHTNING, POPDENS and GDP (GDSTEM-FIRE).
#include "tem_invars.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

#include "tem_input_check.h"
#include "tem_log.h"
#include "tem_netcdf_io.h"

namespace tem
{
namespace nc
{

namespace
{
struct InputDefinition
{
  const char* label;    // name used by the model
  const char* filekey;  // key of the file(s) in tem_config.yaml
  const char* variable; // variable the model expects in the file; several joined by | are read together
  const char* mapkey;   // key of varmap that names another variable (one per variable, joined by |)
};

// lulc and ncohort are not variables of the file: lulc reads all the land-cover fields (LULC_FIELDS in
// tem_netcdf_io.cpp) and ncohort counts the cohorts of a cell.
const InputDefinition inputs[] = {
  { "GIRR", "climate_file", "girr", "girr" },
  { "CLDINESS", "climate_file", "clouds", "clouds" },
  { "NIRR", "climate_file", "nirr", "nirr" },
  { "PAR", "climate_file", "par", "par" },
  { "TAIR", "climate_file", "tair", "tair" },
  { "PREC", "climate_file", "prec", "prec" },
  { "DTAIRRNG", "climate_file", "dtr", "dtr" },
  { "VPRPRESS", "climate_file", "vpr", "vpr" },
  { "WIND", "static_file", "wind", "wind" },
  { "AOT40", "static_file", "ozone", "ozone" },
  { "NDEP", "static_file", "ndep", "ndep" },
  { "MXCOHRTS", "lulc_file", "ncohort", "mxcohort" },
  { "TEXTURE", "static_file", "sand|silt|clay", "sand|silt|clay" },
  { "ELEV", "static_file", "elev", "elev" },
  { "MASK", "static_file", "mask", "mask" },
  { "LULCCHRT", "lulc_file", "lulc", "lulc" },
  { "CO2", "co2_file", "co2", "co2" },
  { "LIGHTNING", "lightning_file", "lightning_flashes", "lightning" },
  { "POPDENS", "population_file", "population_density", "popdens" },
  { "GDP", "gdp_file", "gdp", "gdp" },
  { "STORM", "storm_file", "storm_return_interval|hurricane_return_interval", "storm|hurricane" },
  { "NFERTCROP", "nfert_file", "nfertcrop", "nfertcrop" },
  { "NFERTPASTURE", "nfert_file", "nfertpasture", "nfertpasture" } };

[[noreturn]] void fail( const std::string& msg )
{
  tem::log::fatalRoot( "input error: " + msg );
}

std::vector<std::string> parts( const std::string& text )
{
  std::vector<std::string> out;
  size_t start = 0;
  for( ;; )
  {
    const size_t bar = text.find( '|', start );
    out.push_back( text.substr( start, bar - start ) );
    if( bar == std::string::npos )
      return out;
    start = bar + 1;
  }
}
} // namespace

Input rawSpec( const tem::yaml::Reader& config, const char* label );

namespace
{
// Every input must be on the grid of the land-cover file; the model does not interpolate.
Input checked( const tem::yaml::Reader& config, const char* label, const Input& in );

// An input with its variables in several files: each file is checked alone
Input checkedEach( const tem::yaml::Reader& config, const char* label, const Input& in )
{
  std::vector<std::string> names;
  size_t a = 0;
  for( ;; )
  {
    const size_t b = in.variables.find( '+', a );
    names.push_back( in.variables.substr( a, b - a ) );
    if( b == std::string::npos ) { break; }
    a = b + 1;
  }

  Input out = { "", in.variables, std::vector<std::string>() };

  for( size_t v = 0; v < names.size(); ++v )
  {
    const Input one = checked( config, label, { in.files[v], names[v], std::vector<std::string>() } );
    out.files.push_back( one.path );
  }

  out.path = out.files[0];
  return out;
}

Input checked( const tem::yaml::Reader& config, const char* label, const Input& in )
{
  const std::string l = label;

  if( !in.files.empty() ) { return checkedEach( config, label, in ); }

  if( "CO2" == l || "LULCCHRT" == l ) { return in; }

  if( config.has( "model_resolution" ) && !config.getOptional( "model_resolution" ).empty() && "null" != config.getOptional( "model_resolution" ) )
  {
    fail( "model_resolution is no longer a key: the model does not regrid. Regrid the inputs to the grid of the land-cover file before the run (cdo remapcon, xesmf) and delete model_resolution from the configuration." );
  }

  const std::string lulc = rawSpec( config, "LULCCHRT" ).path;
  const tem_check::Grid g = tem_check::readGrid( lulc ), s = tem_check::readGrid( in.path );

  if( !tem_check::sameGrid( g, s ) )
  {
    std::ostringstream m;
    m << l << " is on a different grid than the land cover.\n"
      << "  " << l << ": " << in.path << "  " << s.lon.size() << " x " << s.lat.size() << " points, " << tem_check::spacing( s.lon ) << " degrees\n"
      << "  land cover: " << lulc << "  " << g.lon.size() << " x " << g.lat.size() << " points, " << tem_check::spacing( g.lon ) << " degrees\n"
      << "The model does not interpolate. Regrid " << l << " to the grid of the land-cover file before the run (cdo remapcon, xesmf).";
    fail( m.str() );
  }

  return in;
}
}  // namespace

Input inputSpec( const tem::yaml::Reader& config, const char* label )
{
  return checked( config, label, rawSpec( config, label ) );
}

Input rawSpec( const tem::yaml::Reader& config, const char* label )
{
  for( size_t k = 0; k < sizeof( inputs ) / sizeof( inputs[0] ); ++k )
  {
    const InputDefinition& in = inputs[k];
    if( std::string( label ) != in.label )
      continue;

    const std::vector<std::string> files = config.getStringList( in.filekey );
    const std::vector<std::string> names = parts( in.variable ), keys = parts( in.mapkey );
    std::string spec, file;
    std::vector<std::string> wherever;
    for( size_t v = 0; v < names.size(); ++v )
    {
      std::string name = names[v], where;
      if( !find( name, files, where ) )
      {
        if( !config.has( keys[v].c_str() ) )
          fail( "No variable " + name + " in " + in.filekey + " and no varmap entry " + keys[v] + " for input " + label );
        name = config.getString( keys[v].c_str() );
        if( name.find_first_of( "+[]|" ) != std::string::npos )
          fail( "varmap " + keys[v] + ": " + name + " must be one variable name (no sums or options; list sand, silt and clay as three entries)" );
        if( !find( name, files, where ) )
          fail( "Variable " + name + " (varmap " + keys[v] + ") is not in " + in.filekey );
      }
      wherever.push_back( where );
      spec += ( spec.empty() ? "" : "+" ) + name;
    }
    file = wherever[0];
    bool same = true;
    for( size_t v = 1; v < wherever.size(); ++v ) { same = same && wherever[v] == file; }
    // the variables of an input may be in one file or in several files of the file key
    return { file, spec, same ? std::vector<std::string>() : wherever };
  }
  fail( std::string( "No input " ) + label );
}

} // namespace nc
} // namespace tem
