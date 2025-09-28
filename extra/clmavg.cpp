#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>
#include <string>
#include <cstdlib>

using namespace std;

double  sumclm[12][3381], refclm[12];
int k,yr,gisend,i,j;
char file[100];


#include "tclmdat45.h"


int main ()
{
	
	Clmdat45 clmdat;

	FILE* ifile;

//	ifile = fopen("/share/partition2/tem/IDL/millennial/dtr_millennial_hurtt.usa48", "r");
	ifile = fopen("/share/partition2/tem/IDL/millennial/vpr_millennial_hurtt.usa48", "r");

	if (!ifile){
		cerr << "climate file could not be opened\n";
		exit(1);
	}

	ofstream fout; 
//    fout.open("/share/partition2/tem/IDL/millennial/aot40_millennial_avg.usa48", ios::out);
    fout.open("/share/partition2/tem/IDL/millennial/vpr_millennial_avg_1901_1930.usa48", ios::out);
	fout.precision(2);
    fout.setf(ios::showpoint);
	fout.flags(ios::fixed);




	for( i = 0; i < 3381; i++ ) {

		cout << "grid = " << i << endl;

	   for(j=0; j < 12; j++) {
		   sumclm[j][i] = 0.0;
	   }

	for(yr=1700; yr<2012; yr++) {


	   gisend = clmdat.getdel(ifile);

//	   if(clmdat.year >= 1700 && clmdat.year < 1730)
	   if(clmdat.year >= 1901 && clmdat.year < 1931)
	   {
	   for(j=0; j < 12; j++) {
	     sumclm[j][i] = sumclm[j][i] + clmdat.mon[j];
//		 cout << o3dat.mon[j] << endl;
	   }
	   }
	}  // end of year loop

	for(j=0; j < 12; j++) {
	sumclm[j][i] = sumclm[j][i]/30.;
	refclm[j] = sumclm[j][i];
	}
	clmdat.outdel(fout,clmdat.col,clmdat.row,clmdat.varname, 
		clmdat.carea, clmdat.year-311, refclm,
		clmdat.contnent);

	}  // end of grid loop

	fclose( ifile );
	fout.close();

};
