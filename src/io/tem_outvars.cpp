// tem_outvars.cpp
// The output variables of XTEM. Each is registered with its name, long name, units and
// storage type. Only registered names get attributes in the output file, and only registered
// names should be written.
//
// To add an output variable: add one line to the table below, or call
//
//     tem::nc::registerOutput( "MYVAR", "my variable", "g C m-2" );                  // float32
//     tem::nc::registerOutput( "MYCOUNT", "my count", "1", tem::nc::Float64 );       // see Kind
//
// before the output is configured. Units with "month-1" are rates and are written per model step
// when the run is not monthly.
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241109 Created: registry of the output variables.
// Includes the cohort metadata variables standage, subtype, cmnt, carea and subarea.

#include "tem_outvars.h"

#include <map>

namespace tem
{
namespace nc
{

namespace
{
std::map<std::string, OutputVar>& table()
{
  static std::map<std::string, OutputVar> t;
  return t;
}
} // namespace

void registerOutput( const std::string& name, const std::string& long_name, const std::string& units, Kind kind )
{
  OutputVar v;
  v.name = name;
  v.long_name = long_name;
  v.units = units;
  v.kind = kind;
  table()[name] = v;
}

const OutputVar* findOutput( const std::string& name )
{
  const std::map<std::string, OutputVar>::const_iterator it = table().find( name );
  return it == table().end() ? 0 : &it->second;
}

} // namespace nc
} // namespace tem

namespace tem
{
namespace nc
{

namespace
{
struct Builtin
{
  const char* name;
  const char* long_name;
  const char* units;
};

const Builtin builtin[] = {
    { "leafc", "leaf carbon", "g C m-2" },
    { "sapwoodc", "sapwood carbon", "g C m-2" },
    { "heartwoodc", "heartwood carbon", "g C m-2" },
    { "rootc", "root carbon", "g C m-2" },
    { "seedc", "seed carbon", "g C m-2" },
    { "labilec", "labile carbon", "g C m-2" },
    { "soilorgc", "soil organic carbon", "g C m-2" },
    { "leafn", "leaf nitrogen", "g N m-2" },
    { "sapwoodn", "sapwood nitrogen", "g N m-2" },
    { "heartwoodn", "heartwood nitrogen", "g N m-2" },
    { "rootn", "root nitrogen", "g N m-2" },
    { "seedn", "seed nitrogen", "g N m-2" },
    { "labilen", "labile nitrogen", "g N m-2" },
    { "soilorgn", "soil organic nitrogen", "g N m-2" },
    { "availn", "available soil nitrogen", "g N m-2" },
    { "fozone", "ozone effect scalar on GPP", "1" },
    { "doc", "dissolved organic carbon", "g C m-2" },
    { "don", "dissolved organic nitrogen", "g N m-2" },
    { "smoist", "soil moisture", "mm" },
    { "vsm", "volumetric soil moisture", "percent" },
    { "pctp", "soil moisture as percent of pore space", "percent" },
    { "rgrndh2o", "rain-fed ground water", "mm" },
    { "sgrndh2o", "snow-melt ground water", "mm" },
    { "fpc", "foliage projective cover", "" },
    { "alloclc", "carbon allocated to leaf", "g C m-2 month-1" },
    { "allocsc", "carbon allocated to sapwood", "g C m-2 month-1" },
    { "allochc", "carbon allocated to heartwood", "g C m-2 month-1" },
    { "allocrc", "carbon allocated to root", "g C m-2 month-1" },
    { "allocseedc", "carbon allocated to seed", "g C m-2 month-1" },
    { "allocilc", "carbon allocated to leaf (I variant)", "g C m-2 month-1" },
    { "allocisc", "carbon allocated to sapwood (I variant)", "g C m-2 month-1" },
    { "allocihc", "carbon allocated to heartwood (I variant)", "g C m-2 month-1" },
    { "allocirc", "carbon allocated to root (I variant)", "g C m-2 month-1" },
    { "allociseedc", "carbon allocated to seed (I variant)", "g C m-2 month-1" },
    { "vegingpp", "gross primary production before nitrogen limitation", "g C m-2 month-1" },
    { "gpp", "gross primary production", "g C m-2 month-1" },
    { "veginnpp", "net primary production before nitrogen limitation", "g C m-2 month-1" },
    { "npp", "net primary production", "g C m-2 month-1" },
    { "gpr", "gross plant respiration", "g C m-2 month-1" },
    { "rvmaint", "vegetation maintenance respiration", "g C m-2 month-1" },
    { "rmleaf", "leaf maintenance respiration", "g C m-2 month-1" },
    { "rmsapwood", "sapwood maintenance respiration", "g C m-2 month-1" },
    { "rmroot", "root maintenance respiration", "g C m-2 month-1" },
    { "rmseed", "seed maintenance respiration", "g C m-2 month-1" },
    { "rmlabile", "labile maintenance respiration", "g C m-2 month-1" },
    { "rvgrwth", "vegetation growth respiration", "g C m-2 month-1" },
    { "ltrlc", "leaf litterfall carbon", "g C m-2 month-1" },
    { "ltrsc", "sapwood litterfall carbon", "g C m-2 month-1" },
    { "ltrhc", "heartwood litterfall carbon", "g C m-2 month-1" },
    { "ltrrc", "root litterfall carbon", "g C m-2 month-1" },
    { "ltrseedc", "seed litterfall carbon", "g C m-2 month-1" },
    { "rh", "heterotrophic respiration", "g C m-2 month-1" },
    { "allocln", "nitrogen allocated to leaf", "g N m-2 month-1" },
    { "allocsn", "nitrogen allocated to sapwood", "g N m-2 month-1" },
    { "allochn", "nitrogen allocated to heartwood", "g N m-2 month-1" },
    { "allocrn", "nitrogen allocated to root", "g N m-2 month-1" },
    { "allocseedn", "nitrogen allocated to seed", "g N m-2 month-1" },
    { "allociln", "nitrogen allocated to leaf (I variant)", "g N m-2 month-1" },
    { "allocisn", "nitrogen allocated to sapwood (I variant)", "g N m-2 month-1" },
    { "allocihn", "nitrogen allocated to heartwood (I variant)", "g N m-2 month-1" },
    { "allocirn", "nitrogen allocated to root (I variant)", "g N m-2 month-1" },
    { "allociseedn", "nitrogen allocated to seed (I variant)", "g N m-2 month-1" },
    { "ninput", "nitrogen input to the soil", "g N m-2 month-1" },
    { "agfertn", "agricultural nitrogen fertilizer", "g N m-2 month-1" },
    { "veginnup", "vegetation nitrogen uptake before nitrogen limitation", "g N m-2 month-1" },
    { "vegnup", "vegetation nitrogen uptake", "g N m-2 month-1" },
    { "nresorbl", "leaf nitrogen resorption", "g N m-2 month-1" },
    { "nresorbs", "sapwood nitrogen resorption", "g N m-2 month-1" },
    { "nresorbr", "root nitrogen resorption", "g N m-2 month-1" },
    { "nresorbseed", "seed nitrogen resorption", "g N m-2 month-1" },
    { "ltrln", "leaf litterfall nitrogen", "g N m-2 month-1" },
    { "ltrsn", "sapwood litterfall nitrogen", "g N m-2 month-1" },
    { "ltrhn", "heartwood litterfall nitrogen", "g N m-2 month-1" },
    { "ltrrn", "root litterfall nitrogen", "g N m-2 month-1" },
    { "ltrseedn", "seed litterfall nitrogen", "g N m-2 month-1" },
    { "micronup", "microbial nitrogen uptake", "g N m-2 month-1" },
    { "netnmin", "net nitrogen mineralization", "g N m-2 month-1" },
    { "nlost", "nitrogen lost from the ecosystem", "g N m-2 month-1" },
    { "docprod", "dissolved organic carbon production", "g C m-2 month-1" },
    { "lchdoc", "dissolved organic carbon leaching", "g C m-2 month-1" },
    { "donprod", "dissolved organic nitrogen production", "g N m-2 month-1" },
    { "lchdon", "dissolved organic nitrogen leaching", "g N m-2 month-1" },
    { "nfixs", "asymbiotic nitrogen fixation", "g N m-2 month-1" },
    { "nfixn", "symbiotic nitrogen fixation", "g N m-2 month-1" },
    { "frdl", "scalar of rooting depth or light effect on GPP (FRDL)", "1" },
    { "fco2", "CO2 effect scalar on GPP", "1" },
    { "fh2o", "moisture effect scalar on GPP", "1" },
    { "temp", "temperature effect scalar on GPP", "1" },
    { "fo3", "ozone effect scalar (FO3)", "1" },
    { "lchdin", "dissolved inorganic nitrogen leaching", "g N m-2 month-1" },
    { "irrigate", "irrigation water", "mm month-1" },
    { "ineet", "estimated evapotranspiration before moisture limitation", "mm month-1" },
    { "eet", "estimated evapotranspiration", "mm month-1" },
    { "rperc", "rain percolation", "mm month-1" },
    { "sperc", "snow-melt percolation", "mm month-1" },
    { "rrun", "rain runoff", "mm month-1" },
    { "srun", "snow-melt runoff", "mm month-1" },
    { "gc", "canopy conductance", "" },
    { "gs", "stomatal conductance", "" },
    { "pecan", "evaporation from the canopy", "mm month-1" },
    { "pesoil", "evaporation from the soil", "mm month-1" },
    { "totec", "total ecosystem carbon", "g C m-2" },
    { "vegc", "vegetation carbon", "g C m-2" },
    { "vegn", "vegetation nitrogen", "g N m-2" },
    { "snowpack", "snowpack", "mm" },
    { "availh2o", "available soil water", "mm" },
    { "nep", "net ecosystem production", "g C m-2 month-1" },
    { "nce", "net carbon exchange", "g C m-2 month-1" },
    { "nbp", "net carbon exchange of the biome", "g C m-2 month-1" },
    { "lai", "leaf area index", "m2 m-2" },
    { "pet", "potential evapotranspiration", "mm month-1" },
    { "snowinf", "snow-melt infiltration", "mm month-1" },
    { "h2oyield", "water yield", "mm month-1" },
    { "agprodc", "AGPRODC", "" },
    { "agprodn", "AGPRODN", "" },
    { "prod10c", "PROD10C", "" },
    { "prod100c", "PROD100C", "" },
    { "totprodc", "TOTPRODC", "" },
    { "residc", "RESIDC", "" },
    { "crpstubc", "CRPSTUBC", "" },
    { "agfprodn", "AGFPRODN", "" },
    { "prod10n", "PROD10N", "" },
    { "prod100n", "PROD100N", "" },
    { "totprodn", "TOTPRODN", "" },
    { "residn", "RESIDN", "" },
    { "crpstubn", "CRPSTUBN", "" },
    { "convertc", "converted carbon", "g C m-2 month-1" },
    { "vconvrtc", "VCONVRTC", "" },
    { "sconvrtc", "SCONVRTC", "" },
    { "slashc", "SLASHC", "" },
    { "cflux", "CFLUX", "" },
    { "convertn", "converted nitrogen", "g N m-2 month-1" },
    { "vconvrtn", "VCONVRTN", "" },
    { "sconvrtn", "SCONVRTN", "" },
    { "slashn", "SLASHN", "" },
    { "nretent", "NRETENT", "" },
    { "nvretent", "NVRETENT", "" },
    { "nsretent", "NSRETENT", "" },
    { "agfprodc", "AGFPRODC", "" },
    { "fresidc", "FRESIDC", "" },
    { "fresidn", "FRESIDN", "" },
    { "agprodfc", "AGPRODFC", "" },
    { "agprodfn", "AGPRODFN", "" },
    { "residfc", "RESIDFC", "" },
    { "residfn", "RESIDFN", "" },
    { "prdf10c", "PRDF10C", "" },
    { "prdf10n", "PRDF10N", "" },
    { "prd10fc", "PRD10FC", "" },
    { "prd10fn", "PRD10FN", "" },
    { "prdf100c", "PRDF100C", "" },
    { "prdf100n", "PRDF100N", "" },
    { "prd100fc", "PRD100FC", "" },
    { "prd100fn", "PRD100FN", "" },
    { "totfprdc", "TOTFPRDC", "" },
    { "totfprdn", "TOTFPRDN", "" },
    { "totprdfc", "TOTPRDFC", "" },
    { "totprdfn", "TOTPRDFN", "" },
    { "cropc", "CROPC", "" },
    { "natvegc", "NATVEGC", "" },
    { "cropn", "CROPN", "" },
    { "natvegn", "NATVEGN", "" },
    { "cropstrn", "CROPSTRN", "" },
    { "natstrn", "NATSTRN", "" },
    { "cropston", "CROPSTON", "" },
    { "natston", "NATSTON", "" },
    { "croplai", "CROPLAI", "" },
    { "natlai", "NATLAI", "" },
    { "cropfpc", "CROPFPC", "" },
    { "natfpc", "NATFPC", "" },
    { "crpingpp", "CRPINGPP", "" },
    { "natingpp", "NATINGPP", "" },
    { "cropgpp", "CROPGPP", "" },
    { "natgpp", "NATGPP", "" },
    { "crpinnpp", "CRPINNPP", "" },
    { "natinnpp", "NATINNPP", "" },
    { "cropnpp", "CROPNPP", "" },
    { "natnpp", "NATNPP", "" },
    { "cropgpr", "CROPGPR", "" },
    { "natgpr", "NATGPR", "" },
    { "crprmnt", "CRPRMNT", "" },
    { "natrvmnt", "NATRVMNT", "" },
    { "crprgrw", "CRPRGRW", "" },
    { "natrgrw", "NATRGRW", "" },
    { "cropltrc", "CROPLTRC", "" },
    { "natltrc", "NATLTRC", "" },
    { "crpinnup", "CRPINNUP", "" },
    { "natinnup", "NATINNUP", "" },
    { "cropnup", "CROPNUP", "" },
    { "natvnup", "NATVNUP", "" },
    { "crpnmobl", "CRPNMOBL", "" },
    { "natvnmbl", "NATVNMBL", "" },
    { "crpnrsrb", "CRPNRSRB", "" },
    { "nvnrsrb", "NVNRSRB", "" },
    { "cropltrn", "CROPLTRN", "" },
    { "natltrn", "NATLTRN", "" },
    { "clippings", "CLIPPINGS", "" },
    { "standdeadc", "STANDDEADC", "" },
    { "standdeadn", "STANDDEADN", "" },
    { "volac", "volatilized carbon", "g C m-2 month-1" },
    { "volan", "volatilized nitrogen", "g N m-2 month-1" },
    { "areaburn", "fraction of the cohort burned by wildfire in the step (veg_fire: 1)", "1" },
    { "potevap", "potential evapotranspiration, Shuttleworth-Wallace with FAO-56 reference surface resistance", "mm month-1" },
    { "actevap", "actual evapotranspiration, canopy and soil Penman-Monteith (no interception term)", "mm month-1" },
    { "girr", "global incoming solar radiation", "W m-2" },
    { "nirr", "net irradiance", "W m-2" },
    { "par", "photosynthetically active radiation", "W m-2" },
    { "cldiness", "cloudiness", "percent" },
    { "tair", "air temperature", "degC" },
    { "prec", "precipitation", "mm month-1" },
    { "rain", "rainfall", "mm month-1" },
    { "snowfall", "snowfall", "mm month-1" },
    { "co2", "atmospheric CO2 concentration", "ppmv" },
    { "vpr", "vapor pressure", "hPa" },
    { "trange", "diurnal temperature range", "degC" },
    { "taird", "daytime air temperature", "degC" },
    { "tairn", "nighttime air temperature", "degC" },
    { "vpdd", "daytime vapor pressure deficit", "hPa" },
    { "vpdn", "nighttime vapor pressure deficit", "hPa" },
    { "ws10", "10 m wind speed", "m s-1" },
    { "aot40", "ozone exposure above 40 ppb", "ppb h" },
    { "dayl", "day length", "hours" },
};
} // namespace

void registerBuiltinOutputs()
{
  for( size_t i = 0; i < sizeof( builtin ) / sizeof( builtin[0] ); ++i )
  {
    registerOutput( builtin[i].name, builtin[i].long_name, builtin[i].units );
  }
  registerOutput( "currentveg", "TEM vegetation type of the cohort", "1", Int32 );
  registerOutput( "potveg", "TEM potential vegetation type of the cohort", "1", Int32 );
  registerOutput( "standage", "stand age of the cohort", "year", Int32 );
  registerOutput( "subtype", "TEM vegetation subtype of the cohort", "1", Int32 );
  registerOutput( "cmnt", "TEM community type of the cohort", "1", Int32 );
  registerOutput( "carea", "grid cell area", "km2", Float64 );
  registerOutput( "subarea", "area of the cohort", "km2", Float64 );
  registerOutput( "agstate", "agriculture state of the cohort (0 natural, 1 agriculture)", "1", Int32 );
}

} // namespace nc
} // namespace tem
