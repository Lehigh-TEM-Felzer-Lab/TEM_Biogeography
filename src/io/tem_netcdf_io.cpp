// tem_netcdf_io.cpp - see tem_netcdf_io.h
//
// Version: XTEM version 4.5
// 20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
// 20241110 Changed: col and row renamed lon and lat.
// 20241109 Created: NetCDF input and output (replaces the text readers and writers).
// 20241109 Updated: run_length: the time axis of every input must cover the run (at least one record per
//          step, month or year as the input is sampled); a shorter one stops the run with a message, a longer
//          one is read from its start. Only the steps run are written (setRunEnd).
// 20241109 Changed: a varmap value is one variable name; variable options (units, scale, offset, box, year)
//          and sums are removed, units come from the units attribute.
// 20241109 Updated: the time axis must increase (as in FMS time_interp).
// 20241109 Updated: the land-cover input is the cell mask; an input without data in a land-cover cell is an
//          error.
// 20241109 Changed: Step arrays are sized MAXCYCLE of temconsts_gds.hpp (TEM_MAXCYCLE removed).
// 20241110 Changed: The time units and calendars are decoded in tem_time.cpp. Record and step tables are vectors; cohortCount(); all inputs need the same grid and
//          first year; non-finite output stops the run.
// 20241110 Changed: Uses tem_time and tem_input_check; the cells that run are land cover and an optional
//          mask; a mask of 0 excludes the cell.
// Grid, mask and cell partition are built once (buildGrid); the cohort dimension comes from the land-cover input and is the same on every rank.

#include "tem_netcdf_io.h"
#include "diag_manager.h"
#include "mpi_layer.h"
#include "netcdf_io.h"
#include "tem_log.h"
#include "tem_outvars.h"
#include "tem_time.h"
#include "tem_input_check.h"
#include "ttimestep.h"
#include "temconsts_gds.hpp"
#include <glob.h>

#include <netcdf.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <iostream>
#include <climits>
#include <map>
#include <ctime>
#include <set>
#include <yaml.h>
#include <sstream>

namespace tem
{
namespace nc
{

static int g_transient_years = 0;
static long g_run_length = 0; // run_length, its units and the steps it makes, for messages
static long g_run_steps = 0;
static int g_run_startyear = -999999;  // clmstartyr
static tem_check::StartYear g_start;  // first year of the time axis of the first time-dependent input
static int g_end_year = -99999; // last transient year and the steps run in it
static int g_end_steps = 0;
static std::string g_run_units;
static int transient_years()
{
  return g_transient_years;
}

namespace
{

const double MISSING = -999999.9; // temconsts_gds.hpp
const double PI = 3.14159265358979323846;

[[noreturn]] void die( const std::string& msg )
{
  std::cerr << std::endl << "NetCDF input: " << msg << std::endl;
  std::exit( -1 );
}

void check( int status, const std::string& what )
{
  if( status != NC_NOERR )
    die( what + ": " + nc_strerror( status ) );
}

std::string lower( std::string s )
{
  for( size_t i = 0; i < s.size(); ++i )
    s[i] = std::tolower( static_cast<unsigned char>( s[i] ) );
  return s;
}

std::vector<std::string> split( const std::string& s, char sep )
{
  std::vector<std::string> out;
  std::string cur;
  std::istringstream is( s );
  while( std::getline( is, cur, sep ) )
    out.push_back( cur );
  return out;
}

std::string attr_text( int ncid, int varid, const char* name )
{
  size_t len = 0;
  if( nc_inq_attlen( ncid, varid, name, &len ) != NC_NOERR )
    return "";
  std::vector<char> buf( len + 1, 0 );
  if( nc_get_att_text( ncid, varid, name, &buf[0] ) != NC_NOERR )
    return "";
  return std::string( &buf[0], len );
}

bool attr_double( int ncid, int varid, const char* name, double* out )
{
  return nc_get_att_double( ncid, varid, name, out ) == NC_NOERR;
}

// ---- calendars and time ---------------------------------------------------------------

struct StepTable
{
  int n;
  std::vector<double> days;
  std::vector<int> month;
  StepTable() : n( 12 ), days( 12 ), month( 12 )
  {
    for( int i = 0; i < 12; ++i )
    {
      days[i] = tem_time::MDAYS[i];
      month[i] = i;
    }
  }
};

StepTable& steptable()
{
  static StepTable t;
  return t;
}

// ---- units -------------------------------------------------------------------------

std::string norm_units( std::string u )
{
  u = lower( u );
  std::string out;
  for( size_t i = 0; i < u.size(); ++i )
  {
    if( u[i] == ' ' || u[i] == '*' || u[i] == '^' )
      continue;
    // CF writes degree_Celsius, degrees_C, degC and Celsius: the underscore is dropped
    if( u[i] == '_' )
      continue;
    out += u[i];
  }
  return out;
}

// target: units TEM expects for the label; returns false if the units are not understood
bool convert( const std::string& from, const std::string& target, bool is_range, double dim,
              double* mul, double* add )
{
  const std::string u = norm_units( from );
  *mul = 1.0;
  *add = 0.0;
  if( target.empty() || u.empty() )
    return target.empty();
  if( target == "degC" )
  {
    // the common spellings of the three scales: K, kelvin, degK, degree_K, degrees_Kelvin; C, celsius, degC,
    // degree_Celsius, degrees_C; F, fahrenheit, degF, degrees_Fahrenheit
    std::string t = u;
    if( 0 == t.compare( 0, 7, "degrees" ) ) { t = t.substr( 7 ); }
    else if( 0 == t.compare( 0, 6, "degree" ) ) { t = t.substr( 6 ); }
    else if( 0 == t.compare( 0, 3, "deg" ) ) { t = t.substr( 3 ); }
    if( t == "k" || t == "kelvin" )
    {
      *add = is_range ? 0.0 : -273.15;
      return true;
    }
    if( t == "c" || t == "celsius" || t == "°c" )
      return true;
    if( t == "f" || t == "fahrenheit" )
    {
      *mul = 1 / 1.8;
      *add = is_range ? 0.0 : -32 / 1.8;
      return true;
    }
    return false;
  }
  if( target == "mm/month" )
  {
    if( u == "kgm-2s-1" || u == "mms-1" || u == "kg/m2/s" || u == "mm/s" )
    {
      *mul = 86400.0 * dim;
      return true;
    }
    if( u == "mmday-1" || u == "mm/day" || u == "mmd-1" || u == "mm/d" || u == "kgm-2day-1" )
    {
      *mul = dim;
      return true;
    }
    // ERA5 monthly means report the mean daily accumulation in metres (long name: m of water equivalent)
    if( u == "m" || u == "mday-1" || u == "m/day" || u == "mofwaterequivalent" )
    {
      *mul = 1000.0 * dim;
      return true;
    }
    if( u == "mm" || u == "mmmonth-1" || u == "mm/month" || u == "kgm-2" || u == "kgm-2month-1" )
      return true;
    return false;
  }
  if( target == "W m-2" )
  {
    if( u == "wm-2" || u == "w/m2" || u == "w/m^2" )
      return true;
    if( u == "jm-2" || u == "j/m2" || u == "jm-2day-1" )
    {
      *mul = 1.0 / 86400.0;
      return true;
    }
    if( u == "mjm-2day-1" )
    {
      *mul = 1.0e6 / 86400.0;
      return true;
    }
    return false;
  }
  if( target == "hPa" )
  {
    if( u == "hpa" || u == "mb" || u == "mbar" || u == "millibar" )
      return true;
    if( u == "pa" )
    {
      *mul = 0.01;
      return true;
    }
    if( u == "kpa" )
    {
      *mul = 10.0;
      return true;
    }
    return false;
  }
  if( target == "m s-1" )
    return u == "ms-1" || u == "m/s";
  if( target == "m" )
  {
    if( u == "m" || u == "meter" || u == "metre" || u == "meters" || u == "metres" )
      return true;
    if( u == "km" )
    {
      *mul = 1000.0;
      return true;
    }
    if( u == "ft" )
    {
      *mul = 0.3048;
      return true;
    }
    return false;
  }
  if( target == "%" )
  {
    if( u == "%" || u == "percent" )
      return true;
    if( u == "1" || u == "fraction" || u == "0-1" )
    {
      *mul = 100.0;
      return true;
    }
    return false;
  }
  return true;
}

// units each TEM variable label is read in (tem_io.py VARNAMES)
std::string target_units( const std::string& label )
{
  if( label == "TAIR" || label == "DTAIRRNG" )
    return "degC";
  if( label == "PREC" )
    return "mm/month";
  if( label == "NIRR" || label == "GIRR" || label == "PAR" )
    return "W m-2";
  if( label == "VPRPRESS" )
    return "hPa";
  if( label == "WIND" )
    return "m s-1";
  if( label == "ELEV" )
    return "m";
  if( label == "CLDINESS" )
    return "%";
  return "";
}

// ---- dataset access --------------------------------------------------------------------

// cells are dealt in turn to the compute processes (MPI); every process reads the same inputs. part_rank -1: a process that runs no cells (the I/O rank)
int& part_rank()
{
  static int r = 0;
  return r;
}

int& part_size()
{
  static int n = 1;
  return n;
}

std::map<std::string, int>& open_files()
{
  static std::map<std::string, int> f;
  return f;
}

int ncid_of( const std::string& path, bool skip_non_netcdf = false )
{
  std::map<std::string, int>::iterator it = open_files().find( path );
  if( it != open_files().end() )
    return it->second;
  int id = -1;
  netcdf_io::NetcdfFile_t file;
  const int status = netcdf_io::open_file( file, path );
  if( skip_non_netcdf && status == NC_ENOTNC )
    return -1;
  check( status, "cannot open " + path );
  id = file.ncid;
  open_files()[path] = id;
  return id;
}

enum Role
{
  Extra,
  Lon,
  Lat,
  Time
};

struct Axis
{
  int dimid;
  std::string name;
  size_t len;
  Role role;
};

bool one_of( const std::string& s, const char* const* list )
{
  for( ; *list; ++list )
    if( s == *list )
      return true;
  return false;
}

Role classify( int ncid, int dimid, const std::string& name )
{
  static const char* const lons[] = { "lon", "longitude", "x", "nav_lon", "xc", "long", 0 };
  static const char* const lats[] = { "lat", "latitude", "y", "nav_lat", "yc", 0 };
  static const char* const times[] = { "time", "valid_time", "t", "month", 0 };
  const std::string n = lower( name );
  if( one_of( n, lons ) )
    return Lon;
  if( one_of( n, lats ) )
    return Lat;
  if( one_of( n, times ) )
    return Time;
  int vid; // coordinate variable with CF attributes
  if( nc_inq_varid( ncid, name.c_str(), &vid ) == NC_NOERR )
  {
    const std::string ax = lower( attr_text( ncid, vid, "axis" ) );
    const std::string sn = lower( attr_text( ncid, vid, "standard_name" ) );
    const std::string un = lower( attr_text( ncid, vid, "units" ) );
    if( ax == "x" || sn == "longitude" || un == "degrees_east" || un == "degree_east" )
      return Lon;
    if( ax == "y" || sn == "latitude" || un == "degrees_north" || un == "degree_north" )
      return Lat;
    if( ax == "t" || sn == "time" )
      return Time;
  }
  (void) dimid;
  return Extra;
}

struct Field
{
  std::string name, units;
  int ncid, varid;
  std::vector<Axis> axes;        // in the variable's dimension order
  int ilon, ilat, itime, iextra; // positions in axes, -1 if absent
  double scale, offset;
  bool has_fill;
  double fill;
  bool optional;
  double dflt; // value when an optional variable is absent from the file
  bool present;
  std::vector<size_t> select; // index for each axis when it is fixed (extra axes)
  bool ragged = false;        // stored as a contiguous ragged array: one value per cohort that exists
};

// A contiguous ragged array (CF Conventions 9.3.3), the storage of a land cover with a different number of cohorts
// in each cell and year. The count variable ncohort(time, lat, lon) has the attribute sample_dimension naming the
// dimension of the data variables. Records are written by longitude, then latitude, then time, then cohort, so that
// the records of one longitude column are one contiguous slab and nothing is stored for a cohort that does not exist.
struct Ragged
{
  bool active = false;
  int ncid = -1, cntvid = -1;
  std::vector<std::string> names; // dimension names of ncohort, file order
  std::vector<size_t> lens;
  int ilon = -1, ilat = -1, itime = -1;
  size_t maxc = 1;
  std::vector<size_t> colstart; // first record of each longitude column; colstart.back() is the total
  // the loaded column
  std::vector<int> cnt;         // [t][lat] number of cohorts
  std::vector<size_t> off;      // [t][lat] first record of the cell and year in the column slab
};

// monthly steps of a time axis
struct TimeAxis
{
  std::vector<long> year;  // per step
  std::vector<int> month;  // 1..12 per step
  std::vector<long> years; // distinct years, ascending
  std::map<long, std::vector<int>>
      step; // year -> step index of each month (12) or one step (annual)
  bool monthly;
  bool native; // the file holds steps(), not 12, time steps per year (and steps() != 12)
  bool climatology;
  std::string calendar;
};

struct Grid
{
  std::vector<double> lon; // ascending, -180..180
  std::vector<double> lat; // ascending
  double dlon, dlat;
  size_t cohort_count;
  bool built;
  struct Cell
  {
    int ilon, ilat;
  };
  std::vector<Cell> cells;    // the cells of this process
  std::vector<Cell> allcells; // the cells of all processes (the mask of the output)
  Grid() : dlon( 0 ), dlat( 0 ), cohort_count( 1 ), built( false )
  {
  }
};

Grid& grid()
{
  static Grid g;
  return g;
}

double wrap180( double lon )
{
  while( lon > 180.0 )
    lon -= 360.0;
  while( lon < -180.0 )
    lon += 360.0;
  return lon;
}

std::string& region_name()
{
  static std::string r = "NC";
  return r;
}

} // namespace

// ---- Stream ----------------------------------------------------------------------------

class Stream
{
public:
  Stream( const Input& input, const std::string& label );
  int next( Record& rec );
  bool done() const
  {
    return cell_ >= grid().cells.size();
  }
  bool timed() const // the input has a time axis that is not a 12 month climatology
  {
    return has_time_ && !time_.climatology;
  }
  bool midCell() const // a transient stream that has delivered part of the years of a cell
  {
    if( !has_time_ || time_.climatology )
      return false;
    if( 0 == ei_ )
      return yi_ != 0;
    // The cursor is inside a year of the cell last read. It is mid-cell only when a later cohort of that year exists
    // or a later year follows; a cell with fewer cohorts than the largest count ends before the last index.
    if( cell_ < grid().cells.size() && iterate_ && !count_ && !fields_.empty() && fields_[0].present && prepared_ )
    {
      const Grid::Cell c = grid().cells[cell_];
      const size_t pt = static_cast<size_t>( time_.step.find( time_.years[yi_] )->second[0] );
      for( size_t k = ei_; k < nextra_; ++k )
      {
        const double probe = raw( 0, pt, k, latmap_[c.ilat] );
        if( !std::isnan( probe ) && !( fields_[0].has_fill && probe == fields_[0].fill ) )
          return true;
      }
      return yi_ + 1 < time_.years.size();
    }
    return true;
  }
  bool valid_at( int ilon,
                 int ilat ); // data at every time step (first extra index) and, for MASK, > 0
  const std::string& label() const
  {
    return label_;
  }
  const std::vector<double>& lons() const
  {
    return lon_;
  }
  const std::vector<double>& lats() const
  {
    return lat_;
  }
  void buildGrid();
  void prepare(); // bind to the shared grid once it is built
  Field& field( size_t i )
  {
    return fields_[i];
  }

private:
  std::string label_, target_;
  std::vector<Field> fields_;
  std::vector<double> lon_, lat_;    // coordinates of fields_[0], native order
  std::vector<int> lonmap_, latmap_; // grid index -> native index
  TimeAxis time_;
  bool has_time_;
  int fixed_year_;
  size_t nextra_; // number of iterated extra indices (1 if none)
  bool iterate_;
  bool count_; // ncohort: one record per cell and year holding the number of cohorts
  bool prepared_;
  size_t cell_, yi_, ei_;
  // Ring of num_io_buffers tiles (time_interp_external2 of FMS keeps num_io_buffers records the same way): a tile holds every
  // field for all time, extra and lat steps and the longitudes first .. first + width - 1
  struct Tile
  {
    int first = 0, width = 0;
    std::vector<std::vector<double>> data; // per field: [t][e][lat native][lon in the tile]
  };
  std::vector<Tile> tiles_;
  std::vector<int> ibuf_; // first longitude held by each tile
  size_t next_buf_;       // the tile that is replaced next
  size_t cur_tile_;       // tile of the cell being read
  int cur_lon_;           // native longitude of the cell being read
  bool collective_;       // every process makes the same reads (the check of the inputs in buildGrid)
  std::vector<std::vector<int>> extra_valid_; // per cell handled lazily
  Ragged rag_;

  bool open_ragged( Field& f, const std::string& path, const std::string& var );
  void load_ragged_counts( int native_lon );

  void open_field( Field& f, const std::string& path, const std::string& var );
  void load_column( int native_lon );
  int tile_width( int native_lon, int* tile0 ) const;
  void set_collective( bool on )
  {
    collective_ = on;
  }
  double raw( size_t fi, size_t t, size_t e, int native_lat ) const;
  double value( size_t fi, size_t t, size_t e, int native_lat, int year, int month,
                double stepdays = -1.0 ) const;
};

namespace
{

std::vector<std::unique_ptr<Stream>>& streams()
{
  static std::vector<std::unique_ptr<Stream>> s;
  return s;
}

// The first five fields are required. The rest have defaults.
struct LulcField
{
  const char* name;
  double dflt;
  bool optional;
};
const LulcField LULC_FIELDS[] = {
    { "standage", 0, false },   { "chrtarea", 0, false },   { "potveg", 0, false },
    { "currentveg", 0, false }, { "subtype", 0, false },    { "agstate", 0, true },
    { "agprevstate", 0, true }, { "disturbflag", 0, true }, { "isrccohort", 0, true } };

} // namespace

Stream::Stream( const Input& input, const std::string& label )
    : label_( label ), target_( target_units( label ) ), has_time_( false ), fixed_year_( -999 ),
      nextra_( 1 ), iterate_( false ), count_( false ), prepared_( false ), cell_( 0 ), yi_( 0 ), ei_( 0 ),
      next_buf_( 0 ), cur_tile_( 0 ), cur_lon_( -1 ), collective_( false )
{
  const std::string& path = input.path;
  const std::string spec = path + ":" + input.variables;
  if( input.variables.find_first_of( "[]" ) != std::string::npos )
    die( "variable options are not supported: " + spec + " (a varmap value is one variable name)" );
  const std::string& vars = input.variables;
  std::vector<std::string> names;
  if( vars == "ncohort" )
  {
    count_ = true;
    names.push_back( "standage" );
  }
  else if( vars == "lulc" )
  {
    for( size_t i = 0; i < sizeof( LULC_FIELDS ) / sizeof( LULC_FIELDS[0] ); ++i )
      names.push_back( LULC_FIELDS[i].name );
  }
  else
  {
    names = split( vars, '+' );
  }
  if( label_ == "STORM" && names.size() != 2 )
    die( "storm input requires tropical storm and hurricane return intervals" );
  fields_.resize( names.size() );
  for( size_t i = 0; i < names.size(); ++i )
  {
    fields_[i].name = names[i];
    fields_[i].optional = false;
    fields_[i].dflt = 0;
    if( vars == "lulc" )
    {
      fields_[i].optional = LULC_FIELDS[i].optional;
      fields_[i].dflt = LULC_FIELDS[i].dflt;
    }
    Field& field = fields_[i];
    const std::string& fpath = ( input.files.size() == names.size() ) ? input.files[i] : path;
    open_field( field, fpath, names[i] );
    if( label_ == "STORM" )
    {
      const std::string field_name = path + ":" + names[i];
      if( field.axes.size() != 2 || field.ilon < 0 || field.ilat < 0 )
        die( field_name + " must have only lon and lat dimensions" );
      const Field& first = fields_.front();
      if( field.axes[field.ilon].dimid != first.axes[first.ilon].dimid ||
          field.axes[field.ilat].dimid != first.axes[first.ilat].dimid )
        die( "storm and hurricane return intervals must share lon and lat dimensions" );
      const std::string units = lower( field.units );
      if( !units.empty() && units != "year" && units != "years" &&
          units != "yr" && units != "yrs" && units != "a" )
        die( field_name + " must use return intervals in years" );
    }
  }

  // geometry comes from the first present field
  size_t first = 0;
  while( first < fields_.size() && !fields_[first].present )
    ++first;
  if( first == fields_.size() )
    die( "none of the variables of " + spec + " is in the file" );
  const Field& f0 = fields_[first];
  if( f0.ilon < 0 || f0.ilat < 0 )
    die( path + ":" + f0.name + " has no recognisable lon and lat dimensions" );
  lon_.resize( f0.axes[f0.ilon].len );
  lat_.resize( f0.axes[f0.ilat].len );
  int vid;
  check( nc_inq_varid( f0.ncid, f0.axes[f0.ilon].name.c_str(), &vid ),
         "no coordinate variable " + f0.axes[f0.ilon].name );
  check( nc_get_var_double( f0.ncid, vid, &lon_[0] ), "reading longitude" );
  tem_check::geoAxis( path, f0.axes[f0.ilon].name, "longitude", attr_text( f0.ncid, vid, "units" ), lon_ );
  check( nc_inq_varid( f0.ncid, f0.axes[f0.ilat].name.c_str(), &vid ),
         "no coordinate variable " + f0.axes[f0.ilat].name );
  check( nc_get_var_double( f0.ncid, vid, &lat_[0] ), "reading latitude" );
  tem_check::geoAxis( path, f0.axes[f0.ilat].name, "latitude", attr_text( f0.ncid, vid, "units" ), lat_ );
  for( size_t i = 0; i < lon_.size(); ++i )
    lon_[i] = wrap180( lon_[i] );

  // variables in other files must be on the same grid and time axis as the first
  for( size_t fi = first + 1; fi < fields_.size(); ++fi )
  {
    const Field& g = fields_[fi];
    if( !g.present || g.ncid == f0.ncid )
      continue;
    const std::string gp = input.files.size() == names.size() ? input.files[fi] : path;
    if( g.ilon < 0 || g.ilat < 0 || g.axes[g.ilon].len != f0.axes[f0.ilon].len || g.axes[g.ilat].len != f0.axes[f0.ilat].len )
      die( gp + ":" + g.name + " is not on the grid of " + path + ":" + f0.name );
    std::vector<double> x( g.axes[g.ilon].len ), y( g.axes[g.ilat].len );
    int cv;
    check( nc_inq_varid( g.ncid, g.axes[g.ilon].name.c_str(), &cv ), "no coordinate variable " + g.axes[g.ilon].name );
    check( nc_get_var_double( g.ncid, cv, &x[0] ), "reading longitude" );
    check( nc_inq_varid( g.ncid, g.axes[g.ilat].name.c_str(), &cv ), "no coordinate variable " + g.axes[g.ilat].name );
    check( nc_get_var_double( g.ncid, cv, &y[0] ), "reading latitude" );
    for( size_t k = 0; k < x.size(); ++k )
      if( std::fabs( wrap180( x[k] ) - lon_[k] ) > 1e-6 )
        die( gp + ":" + g.name + " has other longitudes than " + path + ":" + f0.name );
    for( size_t k = 0; k < y.size(); ++k )
      if( std::fabs( y[k] - lat_[k] ) > 1e-6 )
        die( gp + ":" + g.name + " has other latitudes than " + path + ":" + f0.name );
    if( ( g.itime >= 0 ) != ( f0.itime >= 0 ) || ( g.itime >= 0 && g.axes[g.itime].len != f0.axes[f0.itime].len ) )
      die( gp + ":" + g.name + " has another time axis length than " + path + ":" + f0.name );
  }

  // time axis
  has_time_ = f0.itime >= 0;
  time_.monthly = false;
  time_.native = false;
  time_.climatology = false;
  if( has_time_ )
  {
    const Axis& ta = f0.axes[f0.itime];
    int tv;
    time_.calendar = "standard";
    const bool month_dim = lower( ta.name ) == "month";
    if( month_dim )
    {
      if( ta.len != 12 )
        die( "a climatology needs 12 steps" );
      time_.climatology = true;
      time_.monthly = true;
    }
    else
    {
      check( nc_inq_varid( f0.ncid, ta.name.c_str(), &tv ), "no coordinate variable " + ta.name );
      std::string cal = attr_text( f0.ncid, tv, "calendar" );
      if( cal.empty() )
        cal = "standard";
      tem_time::CFBase tb = tem_time::parseCFUnits( attr_text( f0.ncid, tv, "units" ), cal );
      time_.calendar = tb.cal;
      std::vector<double> tvals( ta.len );
      check( nc_get_var_double( f0.ncid, tv, &tvals[0] ), "reading time" );
      const std::string bnds = attr_text( f0.ncid, tv, "bounds" );
      int bv;
      if( !bnds.empty() && nc_inq_varid( f0.ncid, bnds.c_str(), &bv ) == NC_NOERR )
      {
        std::vector<double> b( 2 * ta.len );
        if( nc_get_var_double( f0.ncid, bv, &b[0] ) == NC_NOERR )
          for( size_t i = 0; i < ta.len; ++i )
            tvals[i] = 0.5 * ( b[2 * i] + b[2 * i + 1] );
      }
      // As in FMS time_interp, the time axis must increase, or records would be matched to the wrong steps.
      for( size_t i = 1; i < ta.len; ++i )
        if( !( tvals[i] > tvals[i - 1] ) )
        {
          std::ostringstream msg;
          msg << "the time axis '" << ta.name << "' does not increase at record " << i << " (" << tvals[i - 1] << " then " << tvals[i] << ")";
          die( msg.str() );
        }
      tem_check::timeSpacing( path, tvals, tem_time::cfUnitSeconds( tb ), getTimestep45().getSTEPSECONDS(), getTimestep45().isMONTHLY() );
      for( size_t i = 0; i < ta.len; ++i )
      {
        long y;
        int m;
        tem_time::decodeCF( tb, tvals[i], &y, &m );
        time_.year.push_back( y );
        time_.month.push_back( m );
        std::vector<int>& v = time_.step[y];
        if( v.empty() )
          time_.years.push_back( y );
        v.push_back( static_cast<int>( i ) );
      }
      std::sort( time_.years.begin(), time_.years.end() );
      const size_t nyr = time_.years.size();
      const size_t per = time_.step[time_.years[0]].size();
      // The model year has 365 days (noleap), as the calendar of the FMS time_manager is fixed by the run.
      if( per >= 360 && "360_day" == time_.calendar )
        die( path + ": the calendar is 360_day, the model year has 365 days: resample the file to a 365_day (noleap) calendar" );
      for( size_t k = 0; k < nyr; ++k )
        if( "standard" == time_.calendar && time_.step[time_.years[k]].size() == 366 )
          die( path + ": year " + std::to_string( time_.years[k] ) + " has 366 daily records (29 February); the model year has 365 days: "
               "remove 29 February (cdo del29feb) or write the file with the noleap calendar" );
      for( size_t k = 0; k + 1 < nyr; ++k )
      {
        if( time_.step[time_.years[k]].size() != per )
          die( path + ": year " + std::to_string( time_.years[k] ) +
               " has a different number of time steps" );
      }
      // the last year may hold fewer records: a run cut short by run_length
      if( time_.step[time_.years[nyr - 1]].size() > per )
        die( path + ": the last year has more time steps than the first" );
      std::set<int> months; // distinct calendar months of the first year
      for( size_t j = 0; j < per; ++j )
        months.insert( time_.month[time_.step[time_.years[0]][j]] );
      const size_t nrun = static_cast<size_t>( steptable().n );
      if( per == 12 && months.size() == 12 )
      {
        time_.monthly = true;
        for( size_t k = 0; k < nyr; ++k )
        { // order by month (a complete year only)
          std::vector<int>& v = time_.step[time_.years[k]];
          if( v.size() != 12 )
            continue;
          std::vector<int> byMonth( 12, -1 );
          for( size_t j = 0; j < 12; ++j )
            byMonth[time_.month[v[j]] - 1] = v[j];
          for( size_t j = 0; j < 12; ++j )
            if( byMonth[j] < 0 )
              die( path + ": a month is missing in year " + std::to_string( time_.years[k] ) );
          v = byMonth;
        }
      }
      else if( per == nrun || ( nyr == 1 && per > 1 && per < nrun ) )
      { // one record per model step; a single year may stop early
        time_.monthly = true;
        time_.native = true;
      }
      else if( per != 1 )
      {
        die( path + ": " + std::to_string( per ) +
             " time steps per year; the run has " + std::to_string( steptable().n ) +
             " steps per year, and monthly (12) or annual (1) data are supported" );
      }
    }
  }

  // extra axis to iterate: the first extra axis of the fields that is not fixed with <dim>=<i>
  for( size_t fi = 0; fi < fields_.size(); ++fi )
  {
    Field& f = fields_[fi];
    if( !f.present )
      continue;
    f.select.assign( f.axes.size(), 0 );
    for( size_t a = 0; a < f.axes.size(); ++a )
    {
      if( f.axes[a].role != Extra )
        continue;
      if( !iterate_ && f.iextra < 0 )
      {
        f.iextra = static_cast<int>( a );
      }
    }
  }
  for( size_t fi = 0; fi < fields_.size(); ++fi )
  {
    if( fields_[fi].present && fields_[fi].iextra >= 0 && !iterate_ )
    {
      iterate_ = true;
      nextra_ = fields_[fi].axes[fields_[fi].iextra].len;
    }
  }
  if( count_ )
    grid().cohort_count = nextra_;
}

void Stream::open_field( Field& f, const std::string& path, const std::string& var )
{
  f.ncid = ncid_of( path );
  f.present = nc_inq_varid( f.ncid, var.c_str(), &f.varid ) == NC_NOERR;
  f.ilon = f.ilat = f.itime = f.iextra = -1;
  f.scale = 1;
  f.offset = 0;
  f.has_fill = false;
  f.fill = 0;
  if( !f.present )
  {
    if( f.optional )
      return;
    die( path + " has no variable '" + var + "'" );
  }
  if( !open_ragged( f, path, var ) )
  {
  int ndims;
  int dimids[NC_MAX_VAR_DIMS];
  check( nc_inq_varndims( f.ncid, f.varid, &ndims ), "inquiring " + var );
  check( nc_inq_vardimid( f.ncid, f.varid, dimids ), "inquiring " + var );
  for( int a = 0; a < ndims; ++a )
  {
    char nm[NC_MAX_NAME + 1];
    size_t len;
    check( nc_inq_dim( f.ncid, dimids[a], nm, &len ), "inquiring dimension of " + var );
    Axis ax;
    ax.dimid = dimids[a];
    ax.name = nm;
    ax.len = len;
    ax.role = classify( f.ncid, dimids[a], nm );
    if( ax.role == Lon && f.ilon < 0 )
      f.ilon = a;
    else if( ax.role == Lat && f.ilat < 0 )
      f.ilat = a;
    else if( ax.role == Time && f.itime < 0 )
      f.itime = a;
    else
      ax.role = Extra;
    f.axes.push_back( ax );
  }
  }
  f.units = attr_text( f.ncid, f.varid, "units" );
  double v;
  if( attr_double( f.ncid, f.varid, "scale_factor", &v ) )
    f.scale = v;
  if( attr_double( f.ncid, f.varid, "add_offset", &v ) )
    f.offset = v;
  if( attr_double( f.ncid, f.varid, "_FillValue", &v ) ||
      attr_double( f.ncid, f.varid, "missing_value", &v ) )
  {
    f.has_fill = true;
    f.fill = v;
  }
}

// A data variable on the sample dimension of an ncohort count variable is a ragged array. Its axes are those of the
// count variable (so the grid, the time axis and the cell test work as for a dense cube) plus a cohort axis as long
// as the largest count, which exists only as an index; no value is stored for a cohort that is absent.
bool Stream::open_ragged( Field& f, const std::string& path, const std::string& var )
{
  int nd = 0, dimid = -1, cvid = -1;
  check( nc_inq_varndims( f.ncid, f.varid, &nd ), "inquiring " + var );
  if( 1 != nd || nc_inq_vardimid( f.ncid, f.varid, &dimid ) != NC_NOERR ||
      nc_inq_varid( f.ncid, "ncohort", &cvid ) != NC_NOERR || cvid == f.varid )
    return false;
  const std::string sample = attr_text( f.ncid, cvid, "sample_dimension" );
  char dn[NC_MAX_NAME + 1];
  size_t dl = 0;
  check( nc_inq_dim( f.ncid, dimid, dn, &dl ), "inquiring dimension of " + var );
  if( sample.empty() || sample != dn )
    return false;
  if( !rag_.active )
  { // first ragged variable of the file: read the layout of the count variable
    rag_.active = true;
    rag_.ncid = f.ncid;
    rag_.cntvid = cvid;
    int cnd = 0;
    int cdims[NC_MAX_VAR_DIMS];
    check( nc_inq_varndims( f.ncid, cvid, &cnd ), "inquiring ncohort" );
    check( nc_inq_vardimid( f.ncid, cvid, cdims ), "inquiring ncohort" );
    for( int a = 0; a < cnd; ++a )
    {
      char nm[NC_MAX_NAME + 1];
      size_t len;
      check( nc_inq_dim( f.ncid, cdims[a], nm, &len ), "inquiring dimension of ncohort" );
      rag_.names.push_back( nm );
      rag_.lens.push_back( len );
      const Role r = classify( f.ncid, cdims[a], nm );
      if( r == Lon && rag_.ilon < 0 ) rag_.ilon = a;
      else if( r == Lat && rag_.ilat < 0 ) rag_.ilat = a;
      else if( r == Time && rag_.itime < 0 ) rag_.itime = a;
      else die( path + ": the dimensions of ncohort must be time, lat and lon" );
    }
    if( rag_.ilon < 0 || rag_.ilat < 0 )
      die( path + ": ncohort needs lon and lat dimensions" );
    // the largest number of cohorts of any cell and year, from the attribute or one pass over the counts
    double mx = 0;
    if( attr_double( f.ncid, cvid, "max_cohort", &mx ) && mx >= 1 )
      rag_.maxc = static_cast<size_t>( mx );
    else
    {
      size_t nl = rag_.lens[rag_.ilon], per = 1;
      for( size_t a = 0; a < rag_.lens.size(); ++a ) if( static_cast<int>( a ) != rag_.ilon ) per *= rag_.lens[a];
      std::vector<size_t> st( cnd, 0 ), ct( rag_.lens );
      ct[rag_.ilon] = 1;
      std::vector<int> col( per );
      for( size_t i = 0; i < nl; ++i )
      {
        st[rag_.ilon] = i;
        check( nc_get_vara_int( f.ncid, cvid, &st[0], &ct[0], &col[0] ), "reading ncohort" );
        for( size_t k = 0; k < per; ++k ) rag_.maxc = std::max( rag_.maxc, static_cast<size_t>( std::max( col[k], 0 ) ) );
      }
    }
    // first record of each longitude column
    const size_t nl = rag_.lens[rag_.ilon];
    size_t per = 1;
    for( size_t a = 0; a < rag_.lens.size(); ++a ) if( static_cast<int>( a ) != rag_.ilon ) per *= rag_.lens[a];
    std::vector<size_t> st( cnd, 0 ), ct( rag_.lens );
    ct[rag_.ilon] = 1;
    std::vector<int> col( per );
    rag_.colstart.assign( nl + 1, 0 );
    for( size_t i = 0; i < nl; ++i )
    {
      st[rag_.ilon] = i;
      check( nc_get_vara_int( f.ncid, cvid, &st[0], &ct[0], &col[0] ), "reading ncohort" );
      size_t n = 0;
      for( size_t k = 0; k < per; ++k ) n += static_cast<size_t>( std::max( col[k], 0 ) );
      rag_.colstart[i + 1] = rag_.colstart[i] + n;
    }
    if( rag_.colstart[nl] != dl )
      die( path + ": the counts of ncohort add to " + std::to_string( rag_.colstart[nl] ) + " records, but dimension " +
           sample + " has " + std::to_string( dl ) );
  }
  else if( f.ncid != rag_.ncid )
    die( path + ": the variables of a ragged land cover must be in one file" );
  if( dl != rag_.colstart.back() )
    die( path + ": " + var + " does not have the sample dimension of ncohort" );
  for( size_t a = 0; a < rag_.names.size(); ++a )
  {
    Axis ax;
    ax.dimid = -1;
    ax.name = rag_.names[a];
    ax.len = rag_.lens[a];
    ax.role = static_cast<int>( a ) == rag_.ilon ? Lon : static_cast<int>( a ) == rag_.ilat ? Lat : Time;
    f.axes.push_back( ax );
  }
  f.ilon = rag_.ilon;
  f.ilat = rag_.ilat;
  f.itime = rag_.itime;
  Axis cx;
  cx.dimid = -1;
  cx.name = "cohort";
  cx.len = rag_.maxc;
  cx.role = Extra;
  f.axes.push_back( cx );
  f.ragged = true;
  return true;
}

// number of cohorts and first record of every cell and year of one longitude column, from ncohort
void Stream::load_ragged_counts( int native_lon )
{
  const size_t nt = rag_.itime >= 0 ? rag_.lens[rag_.itime] : 1, nlat = rag_.lens[rag_.ilat];
  std::vector<size_t> st( rag_.lens.size(), 0 ), ct( rag_.lens );
  st[rag_.ilon] = static_cast<size_t>( native_lon );
  ct[rag_.ilon] = 1;
  size_t per = 1;
  for( size_t a = 0; a < rag_.lens.size(); ++a ) if( static_cast<int>( a ) != rag_.ilon ) per *= rag_.lens[a];
  std::vector<int> col( per );
  check( nc_get_vara_int( rag_.ncid, rag_.cntvid, &st[0], &ct[0], &col[0] ), "reading ncohort" );
  rag_.cnt.assign( nt * nlat, 0 );
  for( size_t t = 0; t < nt; ++t )
    for( size_t j = 0; j < nlat; ++j )
    { // index in file order of the dimensions other than lon
      size_t idx = 0;
      for( size_t a = 0; a < rag_.lens.size(); ++a )
      {
        if( static_cast<int>( a ) == rag_.ilon ) continue;
        const size_t i = static_cast<int>( a ) == rag_.ilat ? j : t;
        idx = idx * rag_.lens[a] + i;
      }
      rag_.cnt[t * nlat + j] = std::max( col[idx], 0 );
      if( static_cast<size_t>( rag_.cnt[t * nlat + j] ) > rag_.maxc )
        die( "ncohort holds " + std::to_string( rag_.cnt[t * nlat + j] ) + " cohorts in a cell, more than its attribute max_cohort (" +
             std::to_string( rag_.maxc ) + "); the output cohort dimension is taken from max_cohort, so correct the attribute or delete it" );
    }
  // records of a column: by latitude, then time, then cohort
  rag_.off.assign( nt * nlat, 0 );
  size_t run = 0;
  for( size_t j = 0; j < nlat; ++j )
    for( size_t t = 0; t < nt; ++t )
    {
      rag_.off[t * nlat + j] = run;
      run += static_cast<size_t>( rag_.cnt[t * nlat + j] );
    }
}

void Stream::prepare()
{
  if( prepared_ )
    return;
  Grid& g = grid();
  // same points, hence the same resolution, as the first input
  lonmap_ = tem_check::matchAxis( label_, "the first NetCDF input", "longitude", lon_, g.lon, g.dlon );
  latmap_ = tem_check::matchAxis( label_, "the first NetCDF input", "latitude", lat_, g.lat, g.dlat );
  if( has_time_ && !time_.climatology && g_run_steps > 0 && !( time_.years.size() == 1 && transient_years() > 1 ) )
  { // every time-dependent input starts in the same year: the records are read in step with the run
    g_start.check( label_, time_.years[0] );
    if( g_run_startyear != -999999 && time_.years[0] != g_run_startyear )
      tem_check::fail( label_ + ": the time axis starts in " + std::to_string( time_.years[0] ) + ", but clmstartyr is " +
                       std::to_string( g_run_startyear ) + ": the records are read from the start of the file, so cut the file at "
                       "the first year of the run (cdo seldate or ncks) or set clmstartyr" );
    // the time axis must cover the run: records >= steps, calendar months or years, as the file is sampled
    const StepTable& st = steptable();
    const size_t nrec = time_.year.size();
    const bool annual = time_.step[time_.years[0]].size() == 1;
    size_t expect;
    std::string kind;
    if( annual )
    {
      expect = static_cast<size_t>( transient_years() );
      kind = "one record per year";
    }
    else if( time_.native || ( st.n == 12 && time_.monthly ) )
    {
      expect = static_cast<size_t>( g_run_steps );
      kind = "one record per model step";
    }
    else
    {
      const int last = ( g_end_steps > 0 && g_end_steps < st.n ) ? g_end_steps : st.n;
      const int mlast = ( last >= st.n ) ? 12 : std::min( 12, st.month[last - 1] + 1 );
      expect = static_cast<size_t>( 12 * ( transient_years() - 1 ) + mlast );
      kind = "one record per calendar month";
    }
    auto ym = [&]( const long y, const int m ) { char b[24]; std::snprintf( b, sizeof b, "%04ld-%02d", y, m + 1 ); return std::string( b ); };
    std::string filerange, needrange;
    if( annual || kind == "one record per calendar month" || ( time_.monthly && st.n == 12 ) )
    {
      filerange = ym( time_.year.front(), time_.month.front() - 1 ) + " to " + ym( time_.year.back(), time_.month.back() - 1 );
      const size_t lastrec = expect > 0 ? expect - 1 : 0;
      needrange = annual ? ( "in year " + std::to_string( time_.year.front() + static_cast<long>( lastrec ) ) )
                         : ( "in " + ym( time_.year.front() + static_cast<long>( lastrec / 12 ), static_cast<int>( lastrec % 12 ) ) );
    }
    tem_check::timeCoversRun( label_, nrec, expect, kind,
                              "run_length = " + std::to_string( g_run_length ) + " " + g_run_units + " is " +
                              std::to_string( g_run_steps ) + " steps of " + std::to_string( st.n ) + " per year",
                              filerange, needrange );
  }
  prepared_ = true;
}

// Cache of the input: a tile of every field with all time, all extra and all lat steps and several longitudes, read with
// one hyperslab call. A cell is served from the tile, so a column of the file is read once whatever its chunking.
// The width of the tile follows from the memory budget (io_buffer_mb, default 256 MiB per tile, num_io_buffers tiles) and is
// a multiple of the longitude extent of the chunks of the variables, with the tile starting on a chunk boundary: every
// chunk is then decoded once, however the file was written. Cells are dealt in longitude order (buildGrid), so a rank
// moves along the tiles. A ragged land cover is stored by longitude column and is read one column at a time.
int Stream::tile_width( int native_lon, int* tile0 ) const
{
  const int nlon = static_cast<int>( lon_.size() );
  if( rag_.active )
  {
    *tile0 = native_lon;
    return 1;
  }
  size_t colbytes = 0; // one longitude of every field
  size_t chunk = 1;    // largest longitude extent of the chunks
  for( size_t fi = 0; fi < fields_.size(); ++fi )
  {
    const Field& f = fields_[fi];
    if( !f.present )
      continue;
    size_t n = 1;
    for( size_t a = 0; a < f.axes.size(); ++a )
      if( static_cast<int>( a ) == f.ilat || static_cast<int>( a ) == f.itime || static_cast<int>( a ) == f.iextra )
        n *= f.axes[a].len;
    colbytes += n * sizeof( double );
    int storage = NC_CONTIGUOUS;
    size_t chunks[NC_MAX_VAR_DIMS];
    if( f.ilon >= 0 && nc_inq_var_chunking( f.ncid, f.varid, &storage, chunks ) == NC_NOERR && storage == NC_CHUNKED )
      chunk = std::max( chunk, chunks[f.ilon] );
  }
  const netcdf_io::NetcdfSettings& set = netcdf_io::netcdf_io_settings();
  size_t w = std::max<size_t>( 1, ( set.io_buffer_mb << 20 ) / std::max<size_t>( 1, colbytes ) );
  w = std::max( chunk, w / chunk * chunk );
  if( set.num_io_buffers < 0 )
    w = static_cast<size_t>( nlon ); // the whole input in one tile
  w = std::min<size_t>( w, static_cast<size_t>( nlon ) );
  *tile0 = static_cast<int>( static_cast<size_t>( native_lon ) / w * w );
  return static_cast<int>( std::min<size_t>( w, static_cast<size_t>( nlon - *tile0 ) ) );
}

// the tile that holds native_lon, for each field
void Stream::load_column( int native_lon )
{
  cur_lon_ = native_lon;
  for( size_t t = 0; t < tiles_.size(); ++t )
    if( native_lon >= tiles_[t].first && native_lon < tiles_[t].first + tiles_[t].width )
    {
      cur_tile_ = t;
      return;
    }
  int first = 0;
  const int width = tile_width( native_lon, &first );
  const int nbuf = rag_.active || netcdf_io::netcdf_io_settings().num_io_buffers < 0
                       ? 1
                       : std::max( 1, netcdf_io::netcdf_io_settings().num_io_buffers );
  const int ib = netcdf_io::find_buf_index( first, ibuf_ ); // (a tile that is narrower at the edge of the grid)
  size_t slot;
  if( ib >= 0 )
    slot = static_cast<size_t>( ib );
  else if( tiles_.size() < static_cast<size_t>( nbuf ) )
  {
    slot = tiles_.size();
    tiles_.push_back( Tile() );
    ibuf_.push_back( first );
  }
  else
  { // round robin, as FMS replaces its record buffers
    slot = next_buf_++ % static_cast<size_t>( nbuf );
    ibuf_[slot] = first;
  }
  Tile& tile = tiles_[slot];
  tile.first = first;
  tile.width = width;
  tile.data.assign( fields_.size(), std::vector<double>() );
  cur_tile_ = slot;
  if( rag_.active )
    load_ragged_counts( native_lon );
  for( size_t fi = 0; fi < fields_.size(); ++fi )
  {
    Field& f = fields_[fi];
    if( !f.present )
      continue;
    if( f.ragged )
    { // one contiguous slab holds the column
      const size_t lo = rag_.colstart[static_cast<size_t>( native_lon )], n = rag_.colstart[static_cast<size_t>( native_lon ) + 1] - lo;
      tile.data[fi].assign( n, 0.0 );
      if( n > 0 )
        check( netcdf_io::netcdf_read_data( f.ncid, f.varid, &lo, &n, &tile.data[fi][0], n, false ), "reading " + f.name );
      continue;
    }
    const size_t nd = f.axes.size();
    std::vector<size_t> start( nd, 0 ), count( nd, 1 );
    size_t total = 1;
    for( size_t a = 0; a < nd; ++a )
    {
      if( static_cast<int>( a ) == f.ilon )
      {
        start[a] = first;
        count[a] = width;
      }
      else if( static_cast<int>( a ) == f.ilat || static_cast<int>( a ) == f.itime ||
               static_cast<int>( a ) == f.iextra )
      {
        count[a] = f.axes[a].len;
      }
      else
      {
        start[a] = f.select[a];
        count[a] = 1;
      }
      total *= count[a];
    }
    tile.data[fi].resize( total );
    check( netcdf_io::netcdf_read_data( f.ncid, f.varid, &start[0], &count[0], &tile.data[fi][0], total, collective_ ),
           "reading " + f.name );
  }
}

// element [t][e][lat] of the cached column, whatever the dimension order of the file
double Stream::raw( size_t fi, size_t t, size_t e, int native_lat ) const
{
  const Field& f = fields_[fi];
  if( f.ragged )
  {
    const size_t nlat = rag_.lens[rag_.ilat], k = ( rag_.itime >= 0 ? t : 0 ) * nlat + static_cast<size_t>( native_lat );
    if( e >= static_cast<size_t>( rag_.cnt[k] ) )
      return std::nan( "" );
    return tiles_[cur_tile_].data[fi][rag_.off[k] + e];
  }
  size_t idx = 0;
  for( size_t a = 0; a < f.axes.size(); ++a )
  {
    size_t i = 0, n = 1;
    if( static_cast<int>( a ) == f.ilon )
    {
      i = cur_lon_ - tiles_[cur_tile_].first;
      n = tiles_[cur_tile_].width;
    }
    else if( static_cast<int>( a ) == f.ilat )
    {
      i = native_lat;
      n = f.axes[a].len;
    }
    else if( static_cast<int>( a ) == f.itime )
    {
      i = t;
      n = f.axes[a].len;
    }
    else if( static_cast<int>( a ) == f.iextra )
    {
      i = e;
      n = f.axes[a].len;
    }
    idx = idx * n + i;
  }
  return tiles_[cur_tile_].data[fi][idx];
}

double Stream::value( size_t fi, size_t t, size_t e, int native_lat, int year, int month,
                      double stepdays ) const
{
  const Field& f = fields_[fi];
  if( !f.present )
    return f.dflt;
  double v = raw( fi, t, e, native_lat );
  if( std::isnan( v ) || ( f.has_fill && v == f.fill ) )
    return MISSING;
  v = v * f.scale + f.offset;
  double mul, add;
  const double dim = stepdays > 0.0 ? stepdays
                     : ( month >= 1 && month <= 12 )
                         ? tem_time::daysInMonth( year > 0 ? year : 2001, month, time_.calendar )
                         : 30;
  if( !convert( f.units, target_, label_ == "DTAIRRNG", dim, &mul, &add ) )
  {
    die( label_ + ": units '" + f.units + "' of " + f.name + " cannot be converted to " + target_ +
         "; fix the units attribute of the variable in the file" );
  }
  return v * mul + add;
}

bool Stream::valid_at( int gi_lon, int gi_lat )
{
  prepare();
  const int nl = lonmap_[gi_lon], nt = latmap_[gi_lat];
  load_column( nl );
  for( size_t fi = 0; fi < fields_.size(); ++fi )
  {
    const Field& f = fields_[fi];
    if( !f.present )
      continue;
    // every time step must have data, or the model would meet missing values during the run
    const size_t nsteps = ( f.itime >= 0 ) ? f.axes[f.itime].len : 1;
    for( size_t t = 0; t < nsteps; ++t )
    {
      const double v = raw( fi, t, 0, nt );
      if( std::isnan( v ) || ( f.has_fill && v == f.fill ) )
        return false;
      if( label_ == "MASK" && v <= 0.0 )
        return false;
    }
  }
  return true;
}

void Stream::buildGrid()
{
  Grid& g = grid();
  if( !g.built )
  {
    // The cell list is fixed at the first read, when every input has been opened.
    g.lon = lons();
    g.lat = lats();
    std::sort( g.lon.begin(), g.lon.end() );
    std::sort( g.lat.begin(), g.lat.end() );
    g.dlon = g.lon.size() > 1 ? g.lon[1] - g.lon[0] : 1.0;
    g.dlat = g.lat.size() > 1 ? g.lat[1] - g.lat[0] : 1.0;
    const int ncid = fields_[0].ncid;
    if( g.lon.size() == 1 )
      nc_get_att_double( ncid, NC_GLOBAL, "geospatial_lon_resolution", &g.dlon );
    if( g.lat.size() == 1 )
      nc_get_att_double( ncid, NC_GLOBAL, "geospatial_lat_resolution", &g.dlat );
    for( size_t s = 0; s < streams().size(); ++s )
      streams()[s]->prepare();
    // The cells that run: those with land cover (the land-cover input) and a positive mask (the optional mask
    // input), and every other input must have data there (tem_check::Domain).
    const size_t nlon = g.lon.size(), nlat = g.lat.size();
    tem_check::Domain dom( nlon, nlat );
    std::vector<char> indomain( nlon * nlat, 1 );
    for( int pass = 0; pass < 2; ++pass )
    { // land cover and mask first: the other inputs are read only where a cell is in the domain
      for( size_t s = 0; s < streams().size(); ++s )
      {
        const std::string& l = streams()[s]->label();
        const bool selector = ( l == "MXCOHRTS" || l == "LULCCHRT" || l == "MASK" );
        if( selector != ( 0 == pass ) )
          continue;
        streams()[s]->set_collective( true ); // every process reads the same tiles in the same order
        std::vector<char> valid( nlon * nlat, 0 );
        for( size_t i = 0; i < nlon; ++i ) // longitude outermost: load_column holds one longitude column
          for( size_t j = 0; j < nlat; ++j )
          {
            const size_t k = i + j * nlon;
            if( !selector && !indomain[k] )
              continue;
            valid[k] = streams()[s]->valid_at( static_cast<int>( i ), static_cast<int>( j ) ) ? 1 : 0;
          }
        streams()[s]->set_collective( false );
        if( selector )
        {
          for( size_t k = 0; k < valid.size(); ++k )
            indomain[k] = indomain[k] && valid[k];
          if( l == "MASK" )
            dom.setMask( valid );
          else
            dom.setLand( valid );
        }
        else
          dom.addInput( l, valid );
      }
    }
    std::vector<size_t> run = dom.cells( g.lon, g.lat );
    // longitude-major, so that the cells of a process move along the tiles of the input cache (load_column)
    std::stable_sort( run.begin(), run.end(), [nlon]( size_t a, size_t b ) {
      return a % nlon != b % nlon ? a % nlon < b % nlon : a / nlon < b / nlon;
    } );
    for( size_t k = 0; k < run.size(); ++k )
    {
      Grid::Cell c = { static_cast<int>( run[k] % nlon ), static_cast<int>( run[k] / nlon ) };
      g.allcells.push_back( c );
      if( part_rank() >= 0 && ( k % part_size() ) == static_cast<size_t>( part_rank() ) )
        g.cells.push_back( c );
    }
    tem::log::root() << "grid: " << dom.summary( run.size() ) << std::endl;
    g.built = true;
  }
}

int Stream::next( Record& rec )
{
  buildGrid();
  Grid& g = grid();
  prepare();

  size_t nyears = ( has_time_ && !time_.climatology ) ? time_.years.size() : 1;
  if( nyears > 1 && transient_years() > 0 && static_cast<size_t>( transient_years() ) < nyears )
    nyears = static_cast<size_t>( transient_years() );  // a longer file: the run reads its first years
  size_t guard = 0;
  for( ;; )
  { // skip extra indices that are missing for this cell (cohorts that do not exist)
    if( cell_ >= g.cells.size() )
      return -1;
    if( ++guard > 1000000 )
      die( "internal error: record search did not end" );
    const Grid::Cell c = g.cells[cell_];
    const int nl = lonmap_[c.ilon], nt = latmap_[c.ilat];
    load_column( nl );
    const size_t e = ei_;
    const size_t y = yi_;
    // advance the cursor for the next call: extra index fastest, then year, then cell
    if( ++ei_ >= ( count_ ? 1 : nextra_ ) )
    {
      ei_ = 0;
      if( ++yi_ >= nyears )
      {
        yi_ = 0;
        ++cell_;
      }
    }

    if( iterate_ && !count_ && !fields_.empty() )
    { // a cohort exists where its first field has data
      // the cohorts of a cell can differ from year to year: test the year being read, not the first one
      const size_t pt = ( has_time_ && !time_.climatology ) ? static_cast<size_t>( time_.step[time_.years[y]][0] ) : 0;
      const double probe = fields_[0].present ? raw( 0, pt, e, nt ) : 0.0;
      const Field& f0 = fields_[0];
      if( std::isnan( probe ) || ( f0.has_fill && probe == f0.fill ) )
        continue;
    }

    const double lat = g.lat[c.ilat];
    rec.lon = static_cast<float>( g.lon[c.ilon] );
    rec.lat = static_cast<float>( lat );
    const double r2 = 6371.0 * 6371.0;
    const double dl = g.dlon * PI / 180.0;
    const double s1 = std::sin( ( lat - 0.5 * g.dlat ) * PI / 180.0 ),
                 s2 = std::sin( ( lat + 0.5 * g.dlat ) * PI / 180.0 );
    rec.area = static_cast<long>( std::fabs( r2 * dl * ( s2 - s1 ) ) + 0.5 );
    rec.region = region_name();
    rec.varname = label_;
    rec.index = ( iterate_ && !count_ ) ? static_cast<int>( e ) + 1 : 0;
    rec.v.clear();
    const long year = ( has_time_ && !time_.climatology ) ? time_.years[y] : fixed_year_;
    rec.year = static_cast<int>( year );

    if( has_time_ && time_.monthly )
    {
      const std::vector<int>* steps = time_.climatology ? 0 : &time_.step[year];
      rec.total = 0;
      rec.max = -1e300;
      rec.min = 1e300;
      bool bad = false;
      const StepTable& st = steptable();
      rec.mon.assign( st.n, 0.0 );
      for( int m = 0; m < st.n; ++m )
      {
        // monthly data on a run with other steps: each step takes the month it starts in
        const int fm = time_.native ? m : ( st.n == 12 ? m : st.month[m] );
        // a run cut short has fewer records in its last year; the steps that are not run reuse the ones present
        const size_t t = steps ? static_cast<size_t>( ( *steps )[static_cast<size_t>( fm ) % steps->size()] ) : static_cast<size_t>( fm );
        const double v = ( st.n == 12 )
                             ? value( 0, t, e, nt, static_cast<int>( year ), m + 1 )
                             : value( 0, t, e, nt, static_cast<int>( year ), fm + 1, st.days[m] );
        if( v <= MISSING + 1 )
          bad = true;
        rec.mon[m] = v;
        rec.total += v;
        rec.max = std::max( rec.max, v );
        rec.min = std::min( rec.min, v );
      }
      if( bad )
      {
        rec.total = rec.max = rec.min = MISSING;
        rec.ave = MISSING;
      }
      else
        rec.ave = rec.total / st.n;
    }
    else
    {
      const size_t t =
          ( has_time_ && !time_.climatology ) ? static_cast<size_t>( time_.step[year][0] ) : 0;
      if( count_ )
      { // cohorts of the cell: indices where standage has data
        double n = 0;
        for( size_t k = 0; k < nextra_; ++k )
        {
          const double x = raw( 0, t, k, nt );
          if( !std::isnan( x ) && !( fields_[0].has_fill && x == fields_[0].fill ) )
            ++n;
        }
        rec.v.assign( 2, n );
      }
      else
        for( size_t fi = 0; fi < fields_.size(); ++fi )
          rec.v.push_back( value( fi, t, e, nt, static_cast<int>( year ), 0 ) );
    }
    return 1;
  }
}

// ---- public functions --------------------------------------------------------------------

int cohortCount()
{
  return static_cast<int>( std::max<size_t>( 1, grid().cohort_count ) );
}

void setSteps( int n, const double* days, const int* months )
{
  if( n < 1 )
    die( "steps per year " + std::to_string( n ) + " is not positive" );
  StepTable& t = steptable();
  t.n = n;
  t.days.assign( n, 0.0 );
  t.month.assign( n, 0 );
  for( int i = 0; i < n; ++i )
  {
    t.days[i] = days[i];
    t.month[i] = months[i];
  }
}

int steps()
{
  return steptable().n;
}

bool find( const std::string& spec, const std::vector<std::string>& patterns, std::string& file )
{
  std::string v = spec;
  if( v == "lulc" || v == "ncohort" )
    v = LULC_FIELDS[0].name;
  for( size_t i = 0; i < patterns.size(); ++i )
  {
    glob_t g;
    if( glob( patterns[i].c_str(), GLOB_TILDE, NULL, &g ) != 0 )
    {
      globfree( &g );
      continue;
    }
    const bool wildcard = patterns[i].find_first_of( "*?[" ) != std::string::npos;
    for( size_t k = 0; k < g.gl_pathc; ++k )
    {
      int vid;
      const int ncid = ncid_of( g.gl_pathv[k], wildcard );
      if( ncid >= 0 && nc_inq_varid( ncid, v.c_str(), &vid ) == NC_NOERR )
      {
        file = g.gl_pathv[k];
        globfree( &g );
        return true;
      }
    }
    globfree( &g );
  }
  return false;
}

bool series( const Input& input, std::vector<int>& years, std::vector<double>& values )
{
  const std::string spec = input.path + ":" + input.variables;
  const int ncid = ncid_of( input.path );
  int vid, ndims, dimid;
  check( nc_inq_varid( ncid, input.variables.c_str(), &vid ), "no variable " + spec );
  check( nc_inq_varndims( ncid, vid, &ndims ), "reading " + spec );
  if( ndims != 1 )
    die( spec + " must have the single dimension time" );
  check( nc_inq_vardimid( ncid, vid, &dimid ), "reading " + spec );
  char dname[NC_MAX_NAME + 1];
  size_t n;
  check( nc_inq_dim( ncid, dimid, dname, &n ), "reading " + spec );
  if( n == 0 )
    die( spec + " has an empty time axis" );
  int tv;
  check( nc_inq_varid( ncid, dname, &tv ), std::string( "no coordinate variable " ) + dname );
  std::string cal = attr_text( ncid, tv, "calendar" );
  if( cal.empty() )
    cal = "standard";
  const tem_time::CFBase tb = tem_time::parseCFUnits( attr_text( ncid, tv, "units" ), cal );
  std::vector<double> t( n ), v( n );
  check( nc_get_var_double( ncid, tv, &t[0] ), "reading time of " + spec );
  check( nc_get_var_double( ncid, vid, &v[0] ), "reading " + spec );
  std::map<int, std::pair<double, int>> sum; // annual mean of sub-annual values
  for( size_t i = 0; i < n; ++i )
  {
    long y;
    int m;
    tem_time::decodeCF( tb, t[i], &y, &m );
    sum[static_cast<int>( y )].first += v[i];
    sum[static_cast<int>( y )].second += 1;
  }
  years.clear();
  values.clear();
  for( std::map<int, std::pair<double, int>>::iterator it = sum.begin(); it != sum.end(); ++it )
  {
    years.push_back( it->first );
    values.push_back( it->second.first / it->second.second );
  }
  return !years.empty();
}

Stream* open( const Input& input, const std::string& label )
{
  streams().push_back( std::make_unique<Stream>( input, label ) );
  return streams().back().get();
}

int next( Stream* stream, Record& rec )
{
  return stream->next( rec );
}

void setTransientYears( int n )
{
  g_transient_years = n;
}

void setRunLength( long n, const std::string& units, long steps )
{
  g_run_length = n;
  g_run_units = units;
  g_run_steps = steps;
}

// The last transient year may run fewer steps than a full year: only those are written.
void setRunStart( int year )
{
  g_run_startyear = year;
}

void setRunEnd( int lastyear, int steps )
{
  g_end_year = lastyear;
  g_end_steps = steps;
}

void checkAligned()
{
  for( const auto& stream : streams() )
    if( stream->midCell() )
      die( stream->label() + ": the file has more time steps per cell than the run reads (run_length and the time axis of the file disagree)" );
}

bool hasTime( Stream* stream )
{
  return stream->timed();
}

bool atEnd( Stream* stream )
{
  stream->buildGrid();
  return stream->done();
}

void closeInputs()
{
  streams().clear();
  for( const auto& entry : open_files() )
  {
    netcdf_io::NetcdfFile_t file;
    file.ncid = entry.second;
    check( netcdf_io::close_file( file ), "closing " + entry.first );
  }
  open_files().clear();
}

// ---- output ------------------------------------------------------------------------------

namespace
{

struct OutConfig
{
  bool enabled;
  std::string file;    // path of the output file, or the stem of the numbered files
  std::string basefile; // the same
  int freqmonths;      // months per output file; 0 writes one file
  std::vector<std::pair<std::string, bool> > declared; // variables the run will write, with a cohort axis or not: defined when the file is created
  OutConfig() : enabled( false ), freqmonths( 0 )
  {
  }
};

OutConfig& outconfig()
{
  static OutConfig c;
  return c;
}

// One output file: a period of freqmonths months starting at month index startmonth (year*12)
struct Output
{
  int ncid;
  int startmonth;
  int dtime, dcoh, dlat, dlon, dbnds, vtime, vbnds, vcoh;
  size_t ncoh = 0; // length of the cohort dimension written so far; it grows when a higher cohort index is written
  int vncoh = -1;  // ncohort(time, lat, lon): number of cohorts of the cell in the year
  std::map<std::string, int> vars;
  std::map<std::string, bool> hascoh;       // variable name -> has a cohort axis
  std::set<int> timedone;                   // years whose time and time_bnds are written
  std::map<int, std::vector<int> > ncount;  // year -> number of cohorts written so far for each cell (lat x lon)
  Output()
      : ncid( -1 ), startmonth( 0 ), dtime( -1 ), dcoh( -1 ), dlat( -1 ), dlon( -1 ), dbnds( -1 ), vtime( -1 ),
        vbnds( -1 ), vcoh( -1 )
  {
  }
};

struct OutState
{
  int firstyear;
  std::map<int, Output> files; // period -> file (rank 0 only: it writes the output of all processes)
  size_t skipped = 0;           // records of years before firstyear, not written
  int skipmin = INT_MAX, skipmax = INT_MIN;
  std::map<std::pair<std::string, int>, std::vector<unsigned long long> > seen; // (variable, year) -> bits of the (cohort, lat, lon) written
  OutState() : firstyear( -1 )
  {
  }
  ~OutState()
  {
    closeAll();
  }
  void closeAll()
  {
    for( std::map<int, Output>::iterator it = files.begin(); it != files.end(); ++it )
      nc_close( it->second.ncid );
    files.clear();
  }
};

OutState& outstate()
{
  static OutState o;
  return o;
}

void ocheck( int status, const std::string& what )
{
  if( status != NC_NOERR )
  {
    std::cerr << std::endl
              << "NetCDF output: " << what << ": " << nc_strerror( status ) << std::endl;
    std::exit( -1 );
  }
}

void putText( int ncid, int varid, const char* name, const std::string& text )
{
  nc_put_att_text( ncid, varid, name, text.size(), text.c_str() );
}

// Index of the axis value x. The cells come from the axes, so x is an axis value (up to the rounding of a float); anything else is an error
size_t axisIndex( const std::vector<double>& v, double x, const char* what )
{
  const bool up = v.size() < 2 || v.front() < v.back();
  std::vector<double>::const_iterator it = std::lower_bound( v.begin(), v.end(), x, [up]( double a, double b ) { return up ? a < b : a > b; } );
  size_t i = it == v.end() ? v.size() - 1 : static_cast<size_t>( it - v.begin() );
  if( i > 0 && std::fabs( v[i - 1] - x ) < std::fabs( v[i] - x ) )
    --i;
  const size_t k = i + 1 < v.size() ? i : i - 1;
  if( v.size() > 1 && std::fabs( v[i] - x ) > 0.01 * std::fabs( v[k + 1] - v[k] ) )
    die( std::string( "output " ) + what + " " + std::to_string( x ) + " is not a value of the axis" );
  return i;
}

// days since 1970-01-01 (noleap) at the start of month m (0-based) of year y
double epochDays( int y, int m )
{
  double d = ( y - 1970 ) * 365.0;
  for( int i = 0; i < m; ++i )
    d += tem_time::MDAYS[i];
  return d;
}

std::string yyyymm( int month )
{
  char b[16];
  std::snprintf( b, sizeof( b ), "%04d%02d", month / 12, month % 12 + 1 );
  return b;
}

std::string periodName( const std::string& file, int startmonth, int nmonths )
{
  if( nmonths == 0 )
    return file;
  const std::filesystem::path path( file );
  const std::string name = path.stem().string() + "_" + yyyymm( startmonth ) + "-" +
                           yyyymm( startmonth + nmonths - 1 ) + path.extension().string();
  return ( path.parent_path() / name ).string();
}

void defineVar( Output& o, const std::string& name, bool cohort ); // defines a variable in define mode (below)

Output& openPeriod( int period )
{
  OutState& st = outstate();
  std::map<int, Output>::iterator it = st.files.find( period );
  if( it != st.files.end() )
    return it->second;
  OutConfig& c = outconfig();
  Grid& g = grid();
  if( !g.built )
  {
    std::cerr << std::endl
              << "NetCDF output needs a NetCDF input that defines the grid" << std::endl;
    std::exit( -1 );
  }
  Output& o = st.files[period];
  o.startmonth = st.firstyear * 12 + period * c.freqmonths;
  const std::string path = periodName( c.basefile, o.startmonth, c.freqmonths );
  ocheck( nc_create( path.c_str(), NC_CLOBBER | NC_NETCDF4, &o.ncid ), "cannot create " + path );
  ocheck( nc_def_dim( o.ncid, "time", NC_UNLIMITED, &o.dtime ), "time dimension" );
  // unlimited: the length starts at the largest count of the land-cover input and grows if a higher cohort index is written
  ocheck( nc_def_dim( o.ncid, "cohort", NC_UNLIMITED, &o.dcoh ), "cohort dimension" );
  ocheck( nc_def_dim( o.ncid, "lat", g.lat.size(), &o.dlat ), "lat dimension" );
  ocheck( nc_def_dim( o.ncid, "lon", g.lon.size(), &o.dlon ), "lon dimension" );
  ocheck( nc_def_dim( o.ncid, "bnds", 2, &o.dbnds ), "bnds dimension" );
  int vlat, vlon, vmask;
  const int dll[2] = { o.dlat, o.dlon };
  const int dtb[2] = { o.dtime, o.dbnds };
  ocheck( nc_def_var( o.ncid, "lat", NC_FLOAT, 1, &o.dlat, &vlat ), "lat variable" );
  ocheck( nc_def_var( o.ncid, "lon", NC_FLOAT, 1, &o.dlon, &vlon ), "lon variable" );
  ocheck( nc_def_var( o.ncid, "cohort", NC_INT64, 1, &o.dcoh, &o.vcoh ), "cohort variable" );
  putText( o.ncid, o.vcoh, "long_name", "cohort index within the cell, 1 to the number of cohorts" );
  putText( o.ncid, o.vcoh, "comment",
           "cohorts of a cell are the land cover classes with data in the lulc input, in the order of its cohort axis; "
           "the vegetation type of each cohort and cell is in currentveg and potveg (flag_meanings list the names), agstate is 0 natural, 1 agriculture" );
  ocheck( nc_def_var( o.ncid, "time", NC_DOUBLE, 1, &o.dtime, &o.vtime ), "time variable" );
  ocheck( nc_def_var( o.ncid, "time_bnds", NC_DOUBLE, 2, dtb, &o.vbnds ), "time_bnds variable" );
  ocheck( nc_def_var( o.ncid, "mask", NC_INT64, 2, dll, &vmask ), "mask variable" );
  { // ncohort(time, lat, lon): cohorts 1 to ncohort exist in the cell and year (0 where there is none)
    const int dtll[3] = { o.dtime, o.dlat, o.dlon };
    ocheck( nc_def_var( o.ncid, "ncohort", NC_INT, 3, dtll, &o.vncoh ), "ncohort variable" );
    const int zero = 0;
    nc_def_var_fill( o.ncid, o.vncoh, 0, &zero );
    putText( o.ncid, o.vncoh, "long_name", "number of cohorts in the cell and year; the cohort indices 1 to ncohort have data" );
    if( netcdf_io::netcdf_io_settings().output_chunk_lonlat > 0 )
    {
      const size_t ll = static_cast<size_t>( netcdf_io::netcdf_io_settings().output_chunk_lonlat );
      const size_t cc[3] = { static_cast<size_t>( steptable().n ), std::min( g.lat.size(), ll ), std::min( g.lon.size(), ll ) };
      nc_def_var_chunking( o.ncid, o.vncoh, NC_CHUNKED, cc );
    }
    netcdf_io::netcdf_add_variable( o.ncid, o.vncoh );
  }
  for( size_t k = 0; k < c.declared.size(); ++k ) // the variables of the run are defined now, once; any other is added when it arrives
    defineVar( o, c.declared[k].first, c.declared[k].second );
  putText( o.ncid, vlat, "units", "degrees_north" );
  putText( o.ncid, vlat, "standard_name", "latitude" );
  putText( o.ncid, vlon, "units", "degrees_east" );
  putText( o.ncid, vlon, "standard_name", "longitude" );
  putText( o.ncid, o.vtime, "units", "days since 1970-01-01 00:00:00" );
  putText( o.ncid, o.vtime, "calendar", "noleap" );
  putText( o.ncid, o.vtime, "standard_name", "time" );
  putText( o.ncid, o.vtime, "axis", "T" );
  putText( o.ncid, o.vtime, "bounds", "time_bnds" );
  putText( o.ncid, vmask, "long_name",
           "1 where TEM ran the cell, 0 where inputs were missing or masked" );
  putText( o.ncid, NC_GLOBAL, "title", "TEM output" );
  putText( o.ncid, NC_GLOBAL, "source", "XTEM" );
  putText( o.ncid, NC_GLOBAL, "region", region_name() );
  putText( o.ncid, NC_GLOBAL, "Conventions", "CF-1.8" );
  ocheck( netcdf_io::netcdf_end_def( o.ncid ), "end of definitions" );
  ocheck( nc_put_var_double( o.ncid, vlat, &g.lat[0] ), "writing lat" );
  ocheck( nc_put_var_double( o.ncid, vlon, &g.lon[0] ), "writing lon" );
  std::vector<long long> cohorts( g.cohort_count );
  for( size_t k = 0; k < cohorts.size(); ++k )
    cohorts[k] = static_cast<long long>( k + 1 );
  const size_t n0 = cohorts.size(), z0 = 0;
  ocheck( nc_put_vara_longlong( o.ncid, o.vcoh, &z0, &n0, &cohorts[0] ), "writing cohort" );
  o.ncoh = n0;
  std::vector<signed char> mask( g.lat.size() * g.lon.size(), 0 );
  for( size_t k = 0; k < g.allcells.size(); ++k )
    mask[static_cast<size_t>( g.allcells[k].ilat ) * g.lon.size() + g.allcells[k].ilon] = 1;
  ocheck( nc_put_var_schar( o.ncid, vmask, &mask[0] ), "writing mask" );
  return o;
}

} // namespace

namespace
{
void writeRecord( const std::string& var, double lon, double lat, int cohort, int year, const double* mon, int n );
}

void configureOutput( const std::string& file, int newfilefreq, const std::string& newfilefrequnits,
                      const std::string& region )
{
  OutConfig& c = outconfig();
  c.enabled = !file.empty() && file != "none";
  c.file = file;
  c.basefile = file;
  region_name() = region;
  registerBuiltinOutputs();
  if( !c.enabled )
    return;
  // Only rank 0 writes. The cells arrive in the order of the longitude-major deal, so the chunks (all steps of a year, one cohort,
  // 16 x 16 cells) that are being filled are a band of a few longitude columns. Cached, each chunk is compressed once, when it
  // is evicted, not at every cell written into it (a variable has its own cache of this size).
  ocheck( nc_set_chunk_cache( 128u << 20, 10007, 0.75f ), "setting chunk cache" );
  tem::diag::init( writeRecord );
  if( newfilefreq < 0 ||
      ( newfilefreq > 0 && newfilefrequnits != "year" && newfilefrequnits != "month" ) )
    die( "output_new_file_freq needs a count >= 0 and output_new_file_freq_units year or month" );
  c.freqmonths = ( newfilefrequnits == "year" ) ? 12 * newfilefreq : newfilefreq;
  if( c.freqmonths % 12 != 0 )
    die( "output_new_file_freq must be whole years: a year is never split between two files" );
}

void setOutputStart( int year )
{
  outstate().firstyear = year;
}

void declareOutput( const std::vector<std::string>& names, bool cohort_axis )
{
  for( size_t k = 0; k < names.size(); ++k )
    outconfig().declared.push_back( std::make_pair( names[k], cohort_axis ) );
}

void setPartition( int rank, int size )
{
  part_rank() = rank;
  part_size() = size;
}

bool outputEnabled()
{
  return outconfig().enabled;
}

// TEM vegetation types of params/gdstem/tcomm45_lulc.yaml, for the flag attributes
const int VEG_TYPES[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56 };
const char* const VEG_NAMES =
    "Ice Alpine_Tundra_Polar_Desert Moist_Wet_Tundra Boreal_Forest Forested_Boreal_Wetlands Boreal_Woodlands Non_forested_Boreal_Wetlands Mixed_Temperate_Forests Temperate_Coniferous_Forests Temperate_Deciduous_Forests Temperate_Forested_Wetlands Tall_Grasslands Short_Grasslands Tropical_Savannas Arid_Shrublands Tropical_Evergreen_Forests Tropical_Forested_Wetlands Tropical_Deciduous_Forests Xeromorphic_Forests_and_Woodlands Tropical_Forested_Floodplains Deserts Tropical_Non_forested_Wetlands Tropical_Non_forested_Floodplains Temperate_Non_forested_Wetlands Temperate_Forested_Floodplains Temperate_Non_forested_Floodplain Wet_Savannas Salt_Marsh Mangroves Tidal_Freshwater_Marshes Temperate_Savannas Reserved Temperate_Broadleaved_Evergreen_Forests Reserved2 Mediterranean_Shrublands Reserved3 Reserved4 Reserved5 Reserved6 Reserved7 Reserved8 Reserved9 Reserved10 Reserved11 Reserved12 Suburban Rodale_Pasture Turflawn Vegetable_Farm Crops Pasture Maize Wheat Rice Soybean Potato";

void putCohortFlags( int ncid, int vid, const std::string& name )
{
  if( name == "currentveg" || name == "potveg" )
  {
    nc_put_att_int( ncid, vid, "flag_values", NC_INT, sizeof( VEG_TYPES ) / sizeof( VEG_TYPES[0] ), VEG_TYPES );
    putText( ncid, vid, "flag_meanings", VEG_NAMES );
  }
  else if( name == "agstate" )
  {
    const int ag[2] = { 0, 1 };
    nc_put_att_int( ncid, vid, "flag_values", NC_INT, 2, ag );
    putText( ncid, vid, "flag_meanings", "natural_vegetation agriculture" );
  }
}

void outputPut( const std::string& var, double lon, double lat, int cohort, int year,
                const double* mon )
{
  if( !outputEnabled() )
    return;
  tem::diag::send_data( var, lon, lat, cohort, year, mon, steptable().n );
}

namespace
{

// Defines an output variable (define mode). Climate variables (no cohort) have no cohort axis.
void defineVar( Output& o, const std::string& name, bool cohort )
{
  Grid& g = grid();
  const int dims[4] = { o.dtime, o.dcoh, o.dlat, o.dlon };
  const int cdims[3] = { o.dtime, o.dlat, o.dlon };
  int vid;
  const OutputVar* reg = findOutput( name );
  const nc_type xtype = !reg ? NC_FLOAT : reg->kind == Float64 ? NC_DOUBLE : reg->kind == Int32 ? NC_INT : reg->kind == Int64 ? NC_INT64 : NC_FLOAT;
  ocheck( nc_def_var( o.ncid, name.c_str(), xtype, cohort ? 4 : 3, cohort ? dims : cdims, &vid ), "defining " + name );
  const double fill = MISSING;
  nc_put_att_double( o.ncid, vid, "_FillValue", xtype, 1, &fill );
  if( reg )
  {
    putText( o.ncid, vid, "long_name", reg->long_name );
    std::string un = reg->units;
    const size_t pos = un.find( "month-1" );
    if( pos != std::string::npos && steptable().n != 12 )
      un.replace( pos, 7, steptable().n >= 365 ? "day-1" : "step-1" ); // rates are per model step
    putText( o.ncid, vid, "units", un );
  }
  putCohortFlags( o.ncid, vid, name );
  // As in FMS (chunksizes are optional, otherwise NetCDF chooses), but the default chunk of an unlimited variable is a whole map per step, which a
  // cell-by-cell writer rewrites for every cell. The chunk is the series of a cell in a lon-lat tile (FMS: the writer's share of x and y, the whole other axes).
  if( netcdf_io::netcdf_io_settings().output_chunk_lonlat > 0 )
  {
    const size_t nst = static_cast<size_t>( steptable().n );
    const size_t ll = static_cast<size_t>( netcdf_io::netcdf_io_settings().output_chunk_lonlat );
    const size_t chunk[4] = { nst, 1, std::min( g.lat.size(), ll ), std::min( g.lon.size(), ll ) };
    const size_t cchunk[3] = { nst, chunk[2], chunk[3] };
    nc_def_var_chunking( o.ncid, vid, NC_CHUNKED, cohort ? chunk : cchunk );
  }
  netcdf_io::netcdf_add_variable( o.ncid, vid );
  o.vars[name] = vid;
  o.hascoh[name] = cohort;
}

// The sink of the diag manager: runs on rank 0 (or in a serial run) for the data of all processes.
// The sink of the diag manager (see above): one cohort and year of one variable in one cell
void writeRecord( const std::string& var, double lon, double lat, int cohort, int year, const double* mon, int )
{
  OutState& st = outstate();
  OutConfig& c = outconfig();
  Grid& g = grid();
  if( st.firstyear < 0 )
    st.firstyear = year; // setOutputStart was not called
  if( year < st.firstyear )
  { // as in FMS, nothing is written before the start of the file; the equilibration of the model labels its records with year 0
    ++st.skipped;
    st.skipmin = std::min( st.skipmin, year );
    st.skipmax = std::max( st.skipmax, year );
    return;
  }
  const size_t ilat = axisIndex( g.lat, lat, "latitude" ), ilon = axisIndex( g.lon, lon, "longitude" );
  const StepTable& stp = steptable();
  const size_t nst = static_cast<size_t>( stp.n );
  const size_t nw = ( year == g_end_year && g_end_steps > 0 && g_end_steps < stp.n ) ? static_cast<size_t>( g_end_steps ) : nst;
  { // a (variable, year, cohort, cell) is written once
    std::vector<unsigned long long>& bits = st.seen[std::make_pair( var, year )];
    const size_t bit = ( static_cast<size_t>( std::max( cohort, 1 ) - 1 ) * g.lat.size() + ilat ) * g.lon.size() + ilon;
    if( bits.size() <= bit / 64 )
      bits.resize( bit / 64 + 1, 0ull );
    if( bits[bit / 64] >> ( bit % 64 ) & 1ull )
      die( "output " + var + " is written twice for lon " + std::to_string( lon ) + " lat " + std::to_string( lat ) + " cohort " +
           std::to_string( cohort ) + " year " + std::to_string( year ) );
    bits[bit / 64] |= 1ull << ( bit % 64 );
  }
  std::vector<double> buf( nst );
  for( size_t m = 0; m < nst; ++m )
  {
    if( !std::isfinite( mon[m] ) )
      die( "output " + var + " is " + ( std::isnan( mon[m] ) ? "NaN" : "infinite" ) + " at lon " + std::to_string( lon ) +
           " lat " + std::to_string( lat ) + " cohort " + std::to_string( cohort ) + " year " + std::to_string( year ) +
           " step " + std::to_string( m + 1 ) + " of " + std::to_string( nst ) );
    buf[m] = mon[m] <= MISSING + 1 ? MISSING : mon[m];
  }
  // A file holds whole years (configureOutput), so all steps of the year are in one file; its period follows from the year
  const int period = c.freqmonths == 0 ? 0 : static_cast<int>( static_cast<long long>( year - st.firstyear ) * 12 / c.freqmonths );
  const size_t nseg = nw, t0 = static_cast<size_t>( year - st.firstyear - period * c.freqmonths / 12 ) * nst; // time index in the file
  Output& o = openPeriod( period );
  std::map<std::string, int>::iterator it = o.vars.find( var );
  if( it == o.vars.end() )
  { // not declared by the run: defined when the first record arrives
    ocheck( nc_redef( o.ncid ), "re-entering define mode" );
    defineVar( o, var, cohort > 0 );
    ocheck( netcdf_io::netcdf_end_def( o.ncid ), "end of definitions" );
    it = o.vars.find( var );
  }
  else if( o.hascoh[var] != ( cohort > 0 ) )
    die( "output " + var + " is declared " + ( o.hascoh[var] ? "with" : "without" ) + " a cohort axis but is written " + ( cohort > 0 ? "with" : "without" ) + " one" );
  if( cohort > 0 && static_cast<size_t>( cohort ) > o.ncoh )
  { // a cohort index beyond the input maximum: the unlimited dimension grows, and so does its coordinate
    std::vector<long long> more;
    for( size_t k = o.ncoh; k < static_cast<size_t>( cohort ); ++k )
      more.push_back( static_cast<long long>( k + 1 ) );
    const size_t at = o.ncoh, n = more.size();
    ocheck( nc_put_vara_longlong( o.ncid, o.vcoh, &at, &n, &more[0] ), "extending the cohort coordinate" );
    o.ncoh = static_cast<size_t>( cohort );
  }
  const size_t start[4] = { t0, static_cast<size_t>( std::max( cohort, 1 ) - 1 ), ilat, ilon }, count[4] = { nseg, 1, 1, 1 };
  const size_t cstart[3] = { t0, ilat, ilon }, ccount[3] = { nseg, 1, 1 };
  ocheck( nc_put_vara_double( o.ncid, it->second, ( cohort > 0 ) ? start : cstart, ( cohort > 0 ) ? count : ccount, &buf[0] ),
          "writing " + var );
  if( cohort > 0 )
  { // ncohort: the highest cohort index written for the cell and year
    std::vector<int>& cnt = o.ncount[year];
    if( cnt.empty() )
      cnt.assign( g.lat.size() * g.lon.size(), 0 );
    int& n = cnt[ilat * g.lon.size() + ilon];
    if( n < cohort )
    {
      n = cohort;
      const std::vector<int> v( nseg, cohort );
      ocheck( nc_put_vara_int( o.ncid, o.vncoh, cstart, ccount, &v[0] ), "writing ncohort" );
    }
  }
  if( o.timedone.insert( year ).second )
  { // time and time_bnds of the steps are written with the first record that has them
    std::vector<double> tv( nseg ), tb( 2 * nseg );
    double day = epochDays( year, 0 );
    for( size_t m = 0; m < nseg; ++m )
    {
      tb[2 * m] = day;
      tb[2 * m + 1] = day + stp.days[m];
      tv[m] = day + 0.5 * stp.days[m];
      day += stp.days[m];
    }
    const size_t tstart[1] = { t0 }, tcount[1] = { nseg }, bstart[2] = { t0, 0 }, bcount[2] = { nseg, 2 };
    ocheck( nc_put_vara_double( o.ncid, o.vtime, tstart, tcount, &tv[0] ), "writing time" );
    ocheck( nc_put_vara_double( o.ncid, o.vbnds, bstart, bcount, &tb[0] ), "writing time_bnds" );
  }
}

} // namespace

void outputFlush()
{
  tem::diag::end(); // rank 0 writes what is still on its way from the other ranks (output and TEM state)
}

void outputClose()
{
  tem::diag::end(); // no effect if outputFlush has been called
  if( !outputEnabled() )
    return;
  if( outstate().skipped > 0 )
    std::cerr << "NetCDF output: " << outstate().skipped << " records of the years " << outstate().skipmin << " to " << outstate().skipmax
              << " were not written: the files start in " << outstate().firstyear << " (equilibration output has no calendar year)" << std::endl;
  outstate().closeAll();
}

} // namespace nc
} // namespace tem
