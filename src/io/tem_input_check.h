/* **************************************************************
TEM_INPUT_CHECK.H - the checks that every NetCDF input matches the
                    others: grid, resolution, time axis and cells

The first input opened sets the grid (lon, lat) and the first year;
every other input must have the same points (so the same resolution),
a time axis whose spacing is the model step, a calendar month or a
year, and as many records as the run needs; the cells run are the
cells with land cover (and a positive mask when one is given) that
have data in every input. A failed check stops the run with the file
and the values that differ.

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241110 Created: The checks of tem_netcdf_io.cpp collected in one place, with the time spacing in seconds
         checked against the step of the run and a summary of the cells excluded and the reason.
20241111 Changed: no regridding in the model (model_resolution, regrid_method and regrid_dir removed); inputs must
         be on the grid of the land-cover file, and errors are reported once through tem_log.h.
**************************************************************** */

#ifndef TEM_INPUT_CHECK_H
#define TEM_INPUT_CHECK_H

#include <string>
#include <vector>

namespace tem_check
{
  // Stops the run with the message
  [[noreturn]] void fail( const std::string& msg );

  // Mean spacing of a coordinate (degrees); 0 for a single point
  double spacing( std::vector<double> coordinate );

  // Index in the native axis of every point of the reference axis, within 0.1 % of the cell width. Stops the run
  //   when the sizes or the spacing differ or a point is missing (axis: "longitude" or "latitude").
  std::vector<int> matchAxis( const std::string& label, const std::string& reflabel, const std::string& axis,
                              const std::vector<double>& native, const std::vector<double>& reference,
                              const double& cellwidth );

  // A lon or lat axis must be a regular grid in degrees, as the grid of a FMS/SHiELD input file must be recognised
  //   as the model grid (cartesian_axis X and Y): units in degrees (or none), values in range, strictly increasing
  //   or decreasing, evenly spaced within 1 % of the cell width. axis is "longitude" or "latitude".
  void geoAxis( const std::string& label, const std::string& axisname, const std::string& axis,
                const std::string& units, const std::vector<double>& values );

  // The spacing of the time axis (values in units of unitseconds) must be the model step, a calendar month or a
  //   year; returns which ("step", "month" or "year"). Spacings that are not regular stop the run.
  std::string timeSpacing( const std::string& label, const std::vector<double>& values, const double& unitseconds,
                           const long long& stepseconds, const bool& monthlystep );

  // The time axis has at least the records the run needs
  //   The message gives the range of the file and the range of the run, as time_interp of FMS does
  //   ("time ... is after range of list ...").
  void timeCoversRun( const std::string& label, const size_t& have, const size_t& need, const std::string& kind,
                      const std::string& runtext, const std::string& filerange = "", const std::string& needrange = "" );

  // All time-dependent inputs start in the same year
  class StartYear
  {
    public:
      StartYear() : year_( -999999 ) {}
      void check( const std::string& label, const long& year );
      void reset() { year_ = -999999; label_.clear(); }
    private:
      long year_;
      std::string label_;
  };

  // The cells that run. Each input gives, for every cell (lon fastest in index i + j * nlon), whether it has
  //   data. land-cover and mask only decide whether a cell is part of the domain; any other input without data
  //   in a domain cell is an error. summary() describes the cells in a line for the log.
  class Domain
  {
    public:
      Domain( const size_t& nlon, const size_t& nlat ) : nlon_( nlon ), nlat_( nlat ), haveland_( false ),
        havemask_( false ), land_( nlon * nlat, 1 ), mask_( nlon * nlat, 1 ) {}
      void setLand( const std::vector<char>& valid ) { haveland_ = true; for( size_t k = 0; k < land_.size(); ++k ) land_[k] = land_[k] && valid[k]; }
      void setMask( const std::vector<char>& valid ) { havemask_ = true; for( size_t k = 0; k < mask_.size(); ++k ) mask_[k] = mask_[k] && valid[k]; }
      void addInput( const std::string& label, const std::vector<char>& valid );
      // i + j * nlon of the cells that run; stops the run when an input lacks data in a domain cell or no cell runs
      std::vector<size_t> cells( const std::vector<double>& lon, const std::vector<double>& lat ) const;
      std::string summary( const size_t& ncells ) const;
    private:
      size_t nlon_, nlat_;
      bool haveland_, havemask_;
      std::vector<char> land_, mask_;
      std::vector<std::string> labels_;
      std::vector<std::vector<char> > valid_;
  };

  // lon and lat of a grid, in the order of the file
  struct Grid
  {
    std::vector<double> lon, lat;
    std::string lonname, latname;
  };

  // lon and lat axes of a NetCDF file (of the first variable that has both), checked with geoAxis
  Grid readGrid( const std::string& path );

  // True when the file has the lon and lat points of the target (the order may differ; 0.1 % of the cell width)
  bool sameGrid( const Grid& a, const Grid& b );
}

#endif
