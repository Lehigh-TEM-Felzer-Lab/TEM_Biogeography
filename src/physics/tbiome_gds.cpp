/* *************************************************************
****************************************************************
TBIOME_GDS.CPP - object describing general characteristics of
                   vegetation mosaic used in the Terrestrial
	           Ecosystem Model (TEM)

Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241109 Changed: Parameters are read with tem::yaml::Reader (tem_yaml.h) instead of ProcessXML, because the
         parameter files are YAML.
20241109 Fixed: Numbers are parsed strictly: a missing key or a value that is not a number stops the run, with
         the file and key named (atoi and atof read such values as 0).

****************************************************************
************************************************************* */

#include<iostream>

  using std::cout;
  using std::ios;
  using std::cerr;
  using std::endl;

#include<fstream>

  using std::ifstream;
  using std::ofstream;

#include<cstdlib>

  using std::exit;
  using std::atof;
  using std::atoi;

#include<string>

  using std::string;

#include<sstream>

  using std::ostringstream;


#include "tbiome_gds.h"


/* *************************************************************
************************************************************* */

Biome45::Biome45( void )
{
  temveg = -99;

};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

int Biome45::getCommunityType( const int& tveg )
{
  int mez;
  int communtype;

  mez = tveg - 1;
  if( mez < 0 || mez >= NUMVEG )
  {
    communtype = 1;
  }
  else { communtype = subtype[mez][0]; }

  return communtype;
};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

int Biome45::getVegMosaic( const int& tveg )
{
  int mez;
  int maxtype;

  mez = tveg - 1;
  if( mez < 0 || mez >= NUMVEG )
  {
    maxtype = 1;
  }
  else { maxtype = numtype[mez]; }

  return maxtype;
};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

double Biome45::getVegSubarea( const int& tveg,
                               const int& dtype,
                               const float& carea )
{
  int mez;
  double sarea;

  mez = tveg - 1;
  if( mez < 0 || mez >= NUMVEG )
  {
    sarea = (double) carea;
  }
  else { sarea = (double) carea * pcttype[mez][dtype] * 0.01; }

  return sarea;
};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

int Biome45::getVegSubtype( const int& tveg, const int& dtype )
{
  int mez;
  int vegtype;

  mez = tveg - 1;
  if( mez < 0 || mez >= NUMVEG )
  {
    vegtype = 1;
  }
  else { vegtype = subtype[mez][dtype]; }

  return vegtype;
};

/* *************************************************************
************************************************************* */


/* *************************************************************
************************************************************* */

void Biome45::getvtype( const string& ecd )
{
  int dv;
  int dtype;
  int vegtype;
  int ez;

  ostringstream tempString;

  tem::yaml::Reader infile( ecd );


  // The number of community types is the number of entries in the file. It may not exceed NUMVEG.
  const int nentries = static_cast<int>( infile.entryCount( "temveg" ) );
  if( nentries > NUMVEG )
  {
    cerr << endl << ecd << " has " << nentries << " temveg entries but NUMVEG is " << NUMVEG << endl;
    exit( -1 );
  }

  for( dv = 0; dv < nentries; ++dv )
  {

    vegtype = infile.nextTemveg();

    if( vegtype > NUMVEG )
    {
      cerr << endl << "TEMVEG type " << vegtype << endl;
      cerr << " cannot be greater than " << NUMVEG;
      cerr << " in communityECD" << endl;
      exit( -1 );
    }

    ez = vegtype - 1;

    numtype[ez] = infile.getInt( "numtype" );

    for( dtype = 0; dtype < NUMMSAC; ++dtype )
    {
      tempString.str( "" );
      tempString << "subtype" << (dtype+1);
      subtype[ez][dtype] = infile.getInt( tempString.str() );

      tempString.str( "" );
      tempString << "pcttype" << (dtype+1);
      pcttype[ez][dtype] = infile.getDouble( tempString.str() );
    }

    infile.endEntry();
  }


  if( 0 == dv )
  {
    cerr << endl << " No community types found in communityECD " << ecd << endl;
    exit( -1 );
  }


};


