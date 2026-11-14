// tem_netcdf_io.h
// NetCDF input and output for TEM.
//
// Each input is a NetCDF dataset and one variable name (tem_invars.cpp). File formats are detected by
// libnetcdf. Cells are visited by ascending longitude, then latitude, and only cells with data in
// all opened streams are used.
//
// Variables may have dimensions (lat, lon), (time, lat, lon), or any of these with one extra
// dimension such as a land-cover cohort, in any order; dimensions are recognised by name or CF
// attributes. Longitude may be 0..360 or -180..180 and latitude may run either way.
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
// 20241109 Updated: the log lists the inputs that lack data in skipped cells that other inputs have.
// 20241110 Changed: Step arrays are vectors of the steps of the year (no fixed capacity).
// 20241110 Changed: setRunStart(year) and the checks of tem_input_check.

#ifndef TEM_NETCDF_IO_H
#define TEM_NETCDF_IO_H

#include <cstddef>
#include <string>
#include <vector>

#include "temconsts_gds.hpp"

namespace tem
{
namespace nc
{

// Steps per year of the run (the model's run_dt). days[i] is the length of step i in days and
// months[i] the calendar month (0-11) its start falls in. Call once, before the first record.
void setSteps( int n, const double* days, const int* months );
int steps();

struct Record
{
  float lon;                   // longitude, degrees east in -180..180
  float lat;                   // latitude, degrees north
  long area;                   // cell area, km2
  int year;                    // -999 for a long-term mean, or a field without time
  int index;                   // 1-based position along the extra dimension (cohort), else 0
  double total, max, ave, min; // statistics of mon (monthly fields)
  std::vector<double> mon;     // values of the steps of the year (steps() of them), in the units TEM expects
  std::vector<double> v;       // one value per variable of the stream (fields that are not monthly)
  std::string region;
  std::string varname; // label given at open, for example TAIR
};

struct Input
{
  std::string path;      // the file of the variables; with files set, the file of the first variable
  std::string variables; // names joined by +
  // One file per variable (same order as variables) when the variables of the input are in different files; empty when all
  // are in path
  std::vector<std::string> files;
};

class Stream;

// Find the first dataset containing the requested variable in a list of paths or globs.
bool find( const std::string& spec, const std::vector<std::string>& patterns, std::string& file );

// Open a gridded input. The label determines the units expected by TEM.
Stream* open( const Input& input, const std::string& label );

// Read a one-dimensional time series as annual means.
bool series( const Input& input, std::vector<int>& years, std::vector<double>& values );

// Cohort dimension of the land-cover input (1 when it has none); known once the inputs are open.
int cohortCount();

// Returns 1 for a record or -1 at the end. A dataset error is fatal.
int next( Stream* stream, Record& rec );

// Number of years of the transient run; a gridded input with a time axis shorter than the run (and longer than 1 year) is an error.
void setTransientYears( int n );
// run_length, run_length_units and the steps they make; used in the message when an input has another length.
void setRunLength( long n, const std::string& units, long steps );
// Last transient year (calendar) and how many of its steps are run; later steps are not written.
void setRunEnd( int lastyear, int steps );

// First year of the run (clmstartyr): the time axis of every time-dependent input must start in it, because the
// records are read in order from the start of the file
void setRunStart( int year );

// Stops with an error if a transient input is positioned inside a cell, which happens when its time axis is longer
// than the years read per cell. Call after the inputs of a cell have been read.
void checkAligned();

// True when the stream has delivered every cell of this process (false before the first record).
// True when the input has a time axis (a series of years or steps, not a 12 month climatology).
bool hasTime( Stream* stream );

bool atEnd( Stream* stream );

void closeInputs();

// ---- output --------------------------------------------------------------------------
// Files are (time, cohort, lat, lon) on the grid of the inputs, one variable per TEM output
// variable, plus mask (cell run or not). time is days since 1970-01-01 (noleap)
// with time_bnds. newfilefreq months or years per file (0: one file named file); long_name,
// units and storage type per variable come from the registry (tem_outvars.h; the
// built-in variables are in tem_outvars.cpp). Climate variables (cohort 0) are (time, lat, lon).
void configureOutput( const std::string& file, int newfilefreq, const std::string& newfilefrequnits,
                      const std::string& region );
// The variables the run will write, with (cohort_axis) or without a cohort axis. They are defined when a file is created, once; a variable
// that is not declared is added when its first record arrives. Call before the first record.
// First year of the output files, as FMS starts its files at the base time of the run: records of earlier years are not written (and counted
// in a note at the end). Call before the first record; without it the first year is that of the first record.
void setOutputStart( int year );

void declareOutput( const std::vector<std::string>& names, bool cohort_axis );

// Deal the cells to size processes in turn: this process runs every size-th cell from rank (rank -1: this process runs no
// cells). Call before the first record is read and before configureOutput. The output of all processes goes to rank 0, which
// writes the one file per period (diag_manager.h).
void setPartition( int rank, int size );

bool outputEnabled();
void outputPut( const std::string& var, double lon, double lat, int cohort, int year,
                const double* mon );
// Collective, right after the last cell of every process and before any barrier: rank 0 writes the data of the other ranks until
// each has sent its last batch (diag_manager.h).
void outputFlush();
// Rank 0 closes the files (calls outputFlush if that has not been done).
void outputClose();

} // namespace nc
} // namespace tem

#endif
