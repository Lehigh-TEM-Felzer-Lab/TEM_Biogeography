/* *************************************************************
disturbdat.CPP - object to read and write the structure of
                   land use/land cover data from/to files

Modifications:

20060422 - DWK created by modifying lulcdat437.cpp
20060422 - DWK changed include from lulcdat437.h to lulcdat44.h
20060422 - DWK changed Lulcdata43:: to Lulcdata44::
20060422 - DWK added int disturbflag and int disturbmonth to
           functions
20070105 - TWC renamed to lulcdat45

************************************************************* */

#include <cstdio>

using std::FILE;
using std::fscanf;

#include <iostream>

using std::endl;
using std::ios;

#include <fstream>

using std::ifstream;
using std::ofstream;

#include <iomanip>

using std::setprecision;

#include <string>

using std::string;

#include "disturbdat_inout.h"

Disturbdat::Disturbdat(void)
{

    disturbend = 1;
    lagpos = -99;
    curpos = 0;
};

/* **************************************************************
                    Public Functions
************************************************************** */

/* *************************************************************
************************************************************* */

/* *************************************************************
************************************************************* */

int Disturbdat::getdel(FILE *infile)
{
    char tmpvarname[40];

    disturbend = fscanf(infile, "%f,%f, %s ,%lf", &col, &row, tmpvarname, &retint);

    varname = tmpvarname;

    return disturbend;
};

/* *************************************************************
************************************************************* */
void Disturbdat::outdel(ofstream &ofile, const float &col, const float &row, const string &varname, const float &retint)

{
    ofile.setf(ios::fixed, ios::floatfield);
    ofile.setf(ios::showpoint);
    ofile.precision(2);

    ofile << col << ",";
    ofile << row << ", ";
    ofile << varname << " ,";
    ofile << setprecision(4) << retint;
    ofile << endl;
};
