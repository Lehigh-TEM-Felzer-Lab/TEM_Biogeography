// tem_invars.h
// Which file and variable supplies each model input (the varmap handling).
//
// Each input belongs to a group with its own file key in tem_config.yaml (climate_file,
// static_file, lulc_file, co2_file, lightning_file,
// population_file, nfert_file, storm_file). TEM uses its standard
// variable name when a file of the group has it; otherwise the flat varmap entry of that name; otherwise the run
// stops. To add an input, add a line to the table in tem_invars.cpp.
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241109 Created: which file and variable supplies each model input (varmap): one variable name per entry;
//          sand, silt and clay, storm and hurricane are separate entries.

#ifndef TEM_INVARS_H
#define TEM_INVARS_H

#include <string>

#include "tem_yaml.h"
#include "tem_netcdf_io.h"

namespace tem
{
namespace nc
{

// Resolve the dataset and variable selector for a model input label.
Input inputSpec( const tem::yaml::Reader& config, const char* label );

} // namespace nc
} // namespace tem

#endif
