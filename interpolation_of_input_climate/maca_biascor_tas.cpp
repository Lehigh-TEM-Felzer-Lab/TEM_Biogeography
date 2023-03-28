#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>

using namespace std;

	double yrsum,yrmax,yrmin,value[12],lon,lat;
        double crujan,crufeb,crumar,cruapr,crumay,crujun,crujul,cruaug,crusep,cruoct,crunov,crudec;
        double gmetjan,gmetfeb,gmetmar,gmetapr,gmetmay,gmetjun,gmetjul,gmetaug,gmetsep,gmetoct,gmetnov,gmetdec;
        double biasjan,biasfeb,biasmar,biasapr,biasmay,biasjun,biasjul,biasaug,biassep,biasoct,biasnov,biasdec;
	int i,year,grid,m,gisend;

#include "tclmdat45.h"

int main ()
{	

        Clmdat45 clmdat;

        FILE* crutair;
        crutair = fopen("/share/partition2/tem/IDL/millennial/tas_millennial_1750_2014.usa48", "r");
        if (!crutair){
                cerr << "original file could not be opened\n";
                exit(1);
        }



	ifstream fcru;
	fcru.open("/share/partition2/tem/CRU4.04/cru2tmpbl_2005_2014.usa48", ios::in);

	if (!fcru){
		cerr << "CRU baseline file could not be opened\n";
		exit(1);
	}

        ifstream fgmet;
        fgmet.open("/share/partition2/climate/maca_usa/ccsm4/maca2tmpbl.usa48", ios::in);


        if (!fgmet){
                cerr << "GRIDMET file could not be opened\n";
                exit(1);
        }


	ofstream fout; 
	fout.open("tas_macacor.usa48", ios::out);
	fout.precision(1);
	fout.setf(ios::showpoint);
	fout.flags(ios::fixed);
	
	
    for(grid=0; grid<3381; grid++)
    {
     cout << "grid = " << grid << endl;

      fcru >> lon >> lat >> crujan >> crufeb >> crumar >> cruapr >> crumay >> crujun >> crujul >> cruaug >> crusep >> cruoct >> crunov >> crudec;
      fgmet >> lon >> lat >> gmetjan >> gmetfeb >> gmetmar >> gmetapr >> gmetmay >> gmetjun >> gmetjul >> gmetaug >> gmetsep >> gmetoct >> gmetnov >> gmetdec;
      biasjan = gmetjan - crujan;
      biasfeb = gmetfeb - crufeb;
      biasmar = gmetmar - crumar;
      biasapr = gmetapr - cruapr;
      biasmay = gmetmay - crumay;
      biasjun = gmetjun - crujun;
      biasjul = gmetjul - crujul;
      biasaug = gmetaug - cruaug;
      biassep = gmetsep - crusep;
      biasoct = gmetoct - cruoct;
      biasnov = gmetnov - crunov;
      biasdec = gmetdec - crudec;

    for(m=1750; m<2015; m++)
    {
       gisend = clmdat.getdel(crutair);


       value[0] = clmdat.mon[0] + biasjan;
       value[1] = clmdat.mon[1] + biasfeb;
       value[2] = clmdat.mon[2] + biasmar;
       value[3] = clmdat.mon[3] + biasapr;
       value[4] = clmdat.mon[4] + biasmay;
       value[5] = clmdat.mon[5] + biasjun;
       value[6] = clmdat.mon[6] + biasjul;
       value[7] = clmdat.mon[7] + biasaug;
       value[8] = clmdat.mon[8] + biassep;
       value[9] = clmdat.mon[9] + biasoct;
       value[10] = clmdat.mon[10] + biasnov;
       value[11] = clmdat.mon[11] + biasdec;

        yrsum = 0.0;
        yrmax = -99999.9;
        yrmin = 99999.9;
        for(i=0; i<12; i++) {
             yrsum = yrsum + value[i];
             if(yrmax < value[i]) {yrmax = value[i];}
             if(yrmin > value[i]) {yrmin = value[i];}
         }

       fout << setprecision(1) << clmdat.col << "," << clmdat.row << ", " << "TAIR" << " ," <<
            clmdat.carea << "," << m << "," << setprecision(1) << yrsum << "," << yrmax <<
            "," << setprecision(2) << yrsum/12 << "," << setprecision(1) <<
            yrmin << "," << value[0] << "," <<
            value[1] << "," << value[2] << "," << value[3] << "," << value[4] << 
            "," << value[5] << "," << value[6] << "," << value[7] <<
            "," << value[8] << "," << value[9] << "," << value[10] <<
            "," << value[11] << ", " << "NAMERICA" << endl;
}
}

    fcru.close();
    fgmet.close();
    fclose( crutair );
    fout.close();

};
