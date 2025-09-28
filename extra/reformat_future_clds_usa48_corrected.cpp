#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>
#include <unistd.h>
using namespace std;

        float xlonp,ylatp;
	double lon[3381],lat[3381],yrsum,yrmax,yrmin,mon[12][265][3381];
        double totp,minp,maxp,meanp,monp[12];
	int i,year,grid,m,j,iareap,yearp,yr;
	char file[50],varname[40],contnent[40];

int main ()
{	


       FILE* cruarea;
        cruarea = fopen("/m2/partition2/tem/CRU4.04/cru_ts4.04.1901lf.cld.dat_usa48.csv", "r");
        if (!cruarea){
                cerr << "cru area file could not be opened\n";
                exit(1);
        }

        sprintf(file,"clds_future_cesm_year_corrected.usa48");
        ifstream ffut;
        ffut.open(file, ios::in);
        ffut.precision(2);
        ffut.setf(ios::showpoint);


        if (!ffut){
                cerr << "future input file could not be opened\n";
                exit(1);
        }


	sprintf(file,"clds_ssp370_2015_2099_tem_corrected.usa48");
	ofstream fout; 
	fout.open(file, ios::out);
	fout.precision(1);
	fout.setf(ios::showpoint);
	fout.flags(ios::fixed);
	
	
    for(grid=0; grid<3381; grid++)
    {
     cout << "grid = " << grid << endl;

       fscanf(cruarea, "%f,%f, %s ,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf, %s",&xlonp, &ylatp, varname, &iareap, &yearp, &totp, &maxp, &meanp, &minp, monp+0, monp+1, monp+2, monp+3, monp+4, monp+5,monp+6, monp+7, monp+8, monp+9, monp+10, monp+11, contnent);

    for(m=2015; m<2101; m++)
    {
ffut >> lon[grid] >> lat[grid] >> year >> mon[0][m-2015][grid] >> mon[1][m-2015][grid] >> mon[2][m-2015][grid]
     >> mon[3][m-2015][grid] >> mon[4][m-2015][grid] >> mon[5][m-2015][grid] >> mon[6][m-2015][grid]
     >> mon[7][m-2015][grid] >> mon[8][m-2015][grid] >> mon[9][m-2015][grid]
     >> mon[10][m-2015][grid] >> mon[11][m-2015][grid];

        yrsum = 0.0;
        yrmax = -99999.9;
        yrmin = 99999.9;
        for(i=0; i<12; i++) {
             yrsum = yrsum + mon[i][m-2015][grid];
             if(yrmax < mon[i][m-2015][grid]) {yrmax = mon[i][m-2015][grid];}
             if(yrmin > mon[i][m-2015][grid]) {yrmin = mon[i][m-2015][grid];}
         }

/*       cout << "final = " << grid << " " << lon[grid] << " " << lat[grid] << endl;
       if(yrsum < 0.0) { 
         cout << "negative value = " << yrsum << " " << m << endl;
         sleep(5);
       }
       if(yrsum == 0.0) { 
         cout << "zero annual precip = " << yrsum << " " << m << endl;
         sleep(5);
       } */
       if(m < 2100)
       {
       fout << setprecision(1) << lon[grid] << "," << lat[grid] << ", " << "CLDINESS" << " ," <<
            iareap << "," << m << "," << setprecision(1) << yrsum << "," << yrmax <<
            "," << setprecision(2) << yrsum/12 << "," << setprecision(1) <<
            yrmin << "," << mon[0][m-2015][grid] << "," <<
            mon[1][m-2015][grid] << "," << mon[2][m-2015][grid] << "," << mon[3][m-2015][grid] << "," <<
                        mon[4][m-2015][grid] <<
            "," << mon[5][m-2015][grid] << "," << mon[6][m-2015][grid] << "," << mon[7][m-2015][grid] <<
            "," << mon[8][m-2015][grid] << "," << mon[9][m-2015][grid] << "," << mon[10][m-2015][grid] <<
            "," << mon[11][m-2015][grid] << ", " << "NAMERICA" << endl;
       }
}
}

    ffut.close();
    fout.close();

};
