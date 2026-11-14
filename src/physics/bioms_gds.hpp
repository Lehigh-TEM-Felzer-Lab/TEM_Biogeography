/* **************************************************************
*****************************************************************
BIOMS45.HPP - describes components comprising biomass
*****************************************************************
Version: XTEM version 4.5
20241110 Changed: version number 4.5 (only the input and output, configuration and time handling changed).
20241109 Changed: the names have the suffix 50; nothing else.

************************************************************** */
#ifndef BIOMS45_HPP
#define BIOMS45_HPP

class Biomass {

  public:

/* **************************************************************
		 Public Variables
************************************************************** */

     double carbon;       // (Units are grams /square meter)
     double nitrogen;     // (Units are grams /square meter)
     double allocc;       // (Units are grams /(square meter-month))
     double allocn;       //  "
     double allocic;       
     double allocin;       
     double ltrc;
     double ltrn;
     double rm;
     double nresorb;
     double rg;
     double cleaf;
     double csap;
     double cheart;
     double clabile;
     double nleaf;
     double nsap;
     double nheart;
     double nlabile;

};

#endif
