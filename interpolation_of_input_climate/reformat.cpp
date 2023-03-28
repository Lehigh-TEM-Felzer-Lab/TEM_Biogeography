#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>
using namespace std;

        float xlonp,ylatp;
	double lon[3399],lat[3399],yrsum,yrmax,yrmin,mon[12][312][3399];
        double totp,minp,maxp,meanp,monp[12];
	int i,year,grid,m,j,iareap,yearp,yr;
	char file[50],varname[40],contnent[40];

int main ()
{	

        FILE* crutair;
        crutair = fopen("/m2/partition2/tem/CRU3.2/USA48/cru32prec1700_2011r.usa48", "r");
        if (!crutair){
                cerr << "cru input file could not be opened\n";
                exit(1);
        }


	sprintf(file,"prec_millennial_mpi_year.usa48");
	ifstream fmil;
	fmil.open(file, ios::in);
	fmil.precision(2);
	fmil.setf(ios::showpoint);


	if (!fmil){
		cerr << "millenial input file could not be opened\n";
		exit(1);
	}

        sprintf(file,"prec_historical_mpi_year.usa48");
        ifstream fhis;
        fhis.open(file, ios::in);
        fhis.precision(2);
        fhis.setf(ios::showpoint);


        if (!fhis){
                cerr << "historical input file could not be opened\n";
                exit(1);
        }


	sprintf(file,"prec_millennial.usa48");
	ofstream fout; 
	fout.open(file, ios::out);
	fout.precision(1);
	fout.setf(ios::showpoint);
	fout.flags(ios::fixed);
	
	
    for(grid=0; grid<3399; grid++)
    {
     cout << "grid = " << grid << endl;

    for(m=1700; m<2012; m++)
    {

        fscanf(crutair, "%f,%f, %s ,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf, %s",&xlonp, &ylatp, varname, &iareap, &yearp, &totp, &maxp, &meanp, &minp, monp+0, monp+1, monp+2, monp+3, monp+4, monp+5,monp+6, monp+7, monp+8, monp+9, monp+10, monp+11, contnent);
	
    if(m < 1850)
    {
fmil >> lon[grid] >> lat[grid] >> year >> mon[0][m-1700][grid] >> mon[1][m-1700][grid] >> mon[2][m-1700][grid]
     >> mon[3][m-1700][grid] >> mon[4][m-1700][grid] >> mon[5][m-1700][grid] >> mon[6][m-1700][grid]
     >> mon[7][m-1700][grid] >> mon[8][m-1700][grid] >> mon[9][m-1700][grid]
     >> mon[10][m-1700][grid] >> mon[11][m-1700][grid];
    }

    if(m >= 1850 && m < 1900)
    {
fhis >> lon[grid] >> lat[grid] >> year >> mon[0][m-1700][grid] >> mon[1][m-1700][grid] >> mon[2][m-1700][grid]
     >> mon[3][m-1700][grid] >> mon[4][m-1700][grid] >> mon[5][m-1700][grid] >> mon[6][m-1700][grid]
     >> mon[7][m-1700][grid] >> mon[8][m-1700][grid] >> mon[9][m-1700][grid]
     >> mon[10][m-1700][grid] >> mon[11][m-1700][grid];

    }

    else if (m >= 1900)
    {
    mon[0][m-1700][grid] = monp[0];
    mon[1][m-1700][grid] = monp[1];
    mon[2][m-1700][grid] = monp[2];
    mon[3][m-1700][grid] = monp[3];
    mon[4][m-1700][grid] = monp[4];
    mon[5][m-1700][grid] = monp[5];
    mon[6][m-1700][grid] = monp[6];
    mon[7][m-1700][grid] = monp[7];
    mon[8][m-1700][grid] = monp[8];
    mon[9][m-1700][grid] = monp[9];
    mon[10][m-1700][grid] = monp[10];
    mon[11][m-1700][grid] = monp[11];
   }

        yrsum = 0.0;
        yrmax = -99999.9;
        yrmin = 99999.9;
        for(i=0; i<12; i++) {
             yrsum = yrsum + mon[i][m-1700][grid];
             if(yrmax < mon[i][m-1700][grid]) {yrmax = mon[i][m-1700][grid];}
             if(yrmin > mon[i][m-1700][grid]) {yrmin = mon[i][m-1700][grid];}
         }
       fout << setprecision(1) << lon[grid] << "," << lat[grid] << ", " << "VPRPRESS" << " ," <<
            iareap << "," << m << "," << setprecision(1) << yrsum << "," << yrmax <<
            "," << setprecision(2) << yrsum/12 << "," << setprecision(1) <<
            yrmin << "," << mon[0][m-1700][grid] << "," <<
            mon[1][m-1700][grid] << "," << mon[2][m-1700][grid] << "," << mon[3][m-1700][grid] << "," <<
                        mon[4][m-1700][grid] <<
            "," << mon[5][m-1700][grid] << "," << mon[6][m-1700][grid] << "," << mon[7][m-1700][grid] <<
            "," << mon[8][m-1700][grid] << "," << mon[9][m-1700][grid] << "," << mon[10][m-1700][grid] <<
            "," << mon[11][m-1700][grid] << ", " << "NAMERICA" << endl;
}
}

    fmil.close();
    fhis.close();
    fclose( crutair );
    fout.close();

};
