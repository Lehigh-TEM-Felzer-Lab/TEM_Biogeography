#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>
#include <unistd.h>
using namespace std;

	float xlonp,ylatp;
        double totp,minp,maxp,meanp,monp[12];
	int i,grid,m,j,iareap,yearp;
	char file[50],varname[40],contnent[40];

int main ()
{	

        FILE* tairhis;
        tairhis = fopen("/m2/partition2/tem/IDL/millennial/prec_millennial_1750_2014.usa48", "r");
        if (!tairhis){
                cerr << "historical input file could not be opened\n";
                exit(1);
        }

        FILE* tairfut;
        tairfut = fopen("/m2/partition2/tem/IDL/millennial/pr_ssp370_2015_2099_tem.usa48", "r");
        if (!tairfut){
                cerr << "future input file could not be opened\n";
                exit(1);
        }


	sprintf(file,"prec_ssp370_1750_2099_tem.usa48");
	ofstream fout; 
	fout.open(file, ios::out);
	fout.precision(1);
	fout.setf(ios::showpoint);
	fout.flags(ios::fixed);
	
	
    for(grid=0; grid<3381; grid++)
    {
     cout << "grid = " << grid << endl;

    for(m=1750; m<2015; m++)
    {
       fscanf(tairhis, "%f,%f, %s ,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf, %s",&xlonp, &ylatp, varname, &iareap, &yearp, &totp, &maxp, &meanp, &minp, monp+0, monp+1, monp+2, monp+3, monp+4, monp+5,monp+6, monp+7, monp+8, monp+9, monp+10, monp+11, contnent);

       fout << setprecision(1) << xlonp << "," << ylatp << ", " << "PREC" << " ," <<
            iareap << "," << yearp << "," << setprecision(1) << totp << "," << maxp <<
            "," << setprecision(2) << meanp << "," << setprecision(1) <<
            minp << "," << monp[0] << "," <<
            monp[1] << "," << monp[2] << "," << monp[3] << "," <<
                        monp[4] <<
            "," << monp[5] << "," << monp[6] << "," << monp[7] <<
            "," << monp[8] << "," << monp[9] << "," << monp[10] <<
            "," << monp[11] << ", " << "NAMERICA" << endl;

    }

    for(m=2015; m<2100; m++)
    {
       fscanf(tairfut, "%f,%f, %s ,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf, %s",&xlonp, &ylatp, varname, &iareap, &yearp, &totp, &maxp, &meanp, &minp, monp+0, monp+1, monp+2, monp+3, monp+4, monp+5,monp+6, monp+7, monp+8, monp+9, monp+10, monp+11, contnent);

       fout << setprecision(1) << xlonp << "," << ylatp << ", " << "PREC" << " ," <<
            iareap << "," << yearp << "," << setprecision(1) << totp << "," << maxp <<
            "," << setprecision(2) << meanp << "," << setprecision(1) <<
            minp << "," << monp[0] << "," <<
            monp[1] << "," << monp[2] << "," << monp[3] << "," <<
                        monp[4] <<
            "," << monp[5] << "," << monp[6] << "," << monp[7] <<
            "," << monp[8] << "," << monp[9] << "," << monp[10] <<
            "," << monp[11] << ", " << "NAMERICA" << endl;

    }
}

    fclose( tairhis );
    fclose( tairfut );
    fout.close();

};
