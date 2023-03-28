PRO read_maca_tair_west


  close, 1
  close, 2

  comma = ","

  fileid = ncdf_open("/m8/kodero/raw_data/MACA_DATA/austarlia/agg_macav2metdata_tasmax_CSIRO-Mk3-6-0_r1i1p1_rcp85_2006_2099_CONUS_monthly.nc")
  fileid1=ncdf_varid(fileid,"air_temperature")
  fileid2=ncdf_varid(fileid,"lon")
  fileid3=ncdf_varid(fileid,"lat")
  fileid4=ncdf_varid(fileid,"time")
  print, "fileid = ", fileid1,fileid2,fileid3,fileid4
  ncdf_varget,fileid,fileid1,air_temperature
  tmax = air_temperature
  ncdf_varget,fileid,fileid2,lon
  ncdf_varget,fileid,fileid3,lat
  ncdf_varget,fileid,fileid4,time
  ncdf_close,fileid
  result1=n_elements(tmax)
  result2=n_elements(lon)
  result3=n_elements(lat)
  result4=n_elements(time)

  print, "n_elements = ", result1,result2,result3,result4

  fileid = ncdf_open("/m8/kodero/raw_data/MACA_DATA/austarlia/agg_macav2metdata_tasmin_CSIRO-Mk3-6-0_r1i1p1_rcp85_2006_2099_CONUS_monthly.nc")
  fileid1=ncdf_varid(fileid,"air_temperature")
  fileid2=ncdf_varid(fileid,"lon")
  fileid3=ncdf_varid(fileid,"lat")
  fileid4=ncdf_varid(fileid,"time")
  print, "fileid = ", fileid1,fileid2,fileid3,fileid4
  ncdf_varget,fileid,fileid1,air_temperature
  tmin = air_temperature
  ncdf_varget,fileid,fileid2,lon
  ncdf_varget,fileid,fileid3,lat
  ncdf_varget,fileid,fileid4,time
  ncdf_close,fileid
  result1=n_elements(tmin)
  result2=n_elements(lon)
  result3=n_elements(lat)
  result4=n_elements(time)

  print, "n_elements = ", result1,result2,result3,result4



openr,2,'grids.csv'

tlon = fltarr(374)
tlat  = fltarr(374)



for i=0L,373L do begin

readf,2,xlon,xlat
    tlon(i) = xlon
    tlat(i) = xlat
;    print,"lon,lat = ",xlon,xlat
endfor
close,2


   openw, 1,width=1000, "/m8/kodero/runs/australia/climate/tair_CSIRO-Mk3-6-0_rcp85_2006_2099.csv"


macalon = fltarr(347,480)
macalat = fltarr(347,480)

  
 l=0
  for y = 0,583 do begin
;  print,"y = ",y
  for x = 0,594 do begin
     if(lon(x) gt 180) then begin
       macalon(l) = lon(x)-360.0
;       print,"macalon 1 = ",macalon(l),l
     endif else begin
       macalon(l) = lon(x)
;       print,"macalon 2 = ",macalon(l),l
     endelse
   macalat(l) = lat(y)

   l=l+1L

   
  endfor
  endfor

print,"finished loop"

  year = 2005
  yrar = -1

;grids,month,year
val = fltarr(374,12,94)


for t = 0,1116,12 do begin



     year = year + 1
     yrar = yrar + 1
     print,"year = ",year,t

  tair = ((tmax +  tmin)/2)-273

for i=0L,373L do begin
;print,"i = ",i
  lonval =  ROUND((tlon(i)- (min(macalon)))/0.041666666666666+1)-1
;   latval = ROUND((max(macalat)-tlat(i))/0.041666666666666+1)-1
   latval = ROUND((tlat(i)- (min(macalat)))/0.041666666666666+1)-1
 ;print,"lonval, latval = ",lonval, latval,tlon(i),tlat(i),lon(lonval)-360,lat(latval),air_temperature(lonval,latval,0)
;   wait,1

;
;  
;
;time = 1128;



   for mon = 0,11 do begin

   val(i,mon,yrar) = 0.0
 ;print, "diag = ",latval,n_elements(tair)," ",tair(0,21,0)
   if(tair(lonval,latval,t+mon) gt -9999 and lonval lt 595 and latval lt 584) then begin
      val(i,mon,yrar) = tair(lonval,latval,t+mon)
   endif else begin
; if(air_temperature(lonval,latval,t+mon) gt -9999
; and lonval lt 59 and latval lt 50) then begin
 ;     val(i,mon,yrar) = air_temperature(lonval,latval,t+mon)
  ; endif else begin
   endelse


  endfor  ;month loop
 endfor ; grid loop
endfor  ;year loop`


  for l = 0L,373L do begin

  for t = 0,93 do begin

  sum = total(val(l,*,t)) 
  avg = sum/12.0
  maxval = max(val(l,*,t))
  minval = min(val(l,*,t)) 

 tair_sum = STRING(sum, FORMAT='(F8.2)')
 tair_sum = StrTrim(tair_sum,2)
 tair_sum =tair_sum + comma
 tair_avg = STRING(avg, FORMAT='(F7.2)')
 tair_avg = StrTrim(tair_avg,2)
 tair_avg =tair_avg + comma
 tair_maxval = STRING(maxval, FORMAT='(F7.2)')
 tair_maxval = StrTrim(tair_maxval,2)
 tair_maxval =tair_maxval + comma
 tair_minval = STRING(minval, FORMAT='(F7.2)')
 tair_minval = StrTrim(tair_minval,2)
 tair_minval =tair_minval + comma
  slon = STRING(tlon(l), FORMAT='(F9.3)')
  slon = StrTrim(slon,2)
  slon = slon + comma
;  print,"lon = ",macalon(l),l,slon
  slat = STRING(tlat(l), FORMAT='(F8.3)')
  slat = StrTrim(slat,2)
  slat = slat + comma
  syear = StrTrim(t+2006,2)
  syear = syear + comma
 tair_jan = STRING(val(l,0,t), FORMAT='(F7.2)')
 tair_jan = StrTrim(tair_jan,2)
 tair_jan =tair_jan + comma
 tair_feb = STRING(val(l,1,t), FORMAT='(F7.2)')
 tair_feb = StrTrim(tair_feb,2)
 tair_feb =tair_feb + comma
 tair_mar = STRING(val(l,2,t), FORMAT='(F7.2)')
 tair_mar = StrTrim(tair_mar,2)
 tair_mar =tair_mar + comma
 tair_apr = STRING(val(l,3,t), FORMAT='(F7.2)')
 tair_apr = StrTrim(tair_apr,2)
 tair_apr =tair_apr + comma
 tair_may = STRING(val(l,4,t), FORMAT='(F7.2)')
 tair_may = StrTrim(tair_may,2)
 tair_may =tair_may + comma
 tair_jun = STRING(val(l,5,t), FORMAT='(F7.2)')
 tair_jun = StrTrim(tair_jun,2)
 tair_jun =tair_jun + comma
 tair_jul = STRING(val(l,6,t), FORMAT='(F7.2)')
 tair_jul = StrTrim(tair_jul,2)
 tair_jul =tair_jul + comma
 tair_aug = STRING(val(l,7,t), FORMAT='(F7.2)')
 tair_aug = StrTrim(tair_aug,2)
 tair_aug =tair_aug + comma
 tair_sep = STRING(val(l,8,t), FORMAT='(F7.2)')
 tair_sep = StrTrim(tair_sep,2)
 tair_sep =tair_sep + comma
 tair_oct = STRING(val(l,9,t), FORMAT='(F7.2)')
 tair_oct = StrTrim(tair_oct,2)
 tair_oct =tair_oct + comma
 tair_nov = STRING(val(l,10,t), FORMAT='(F7.2)')
 tair_nov = StrTrim(tair_nov,2)
 tair_nov =tair_nov + comma
 tair_dec = STRING(val(l,11,t), FORMAT='(F7.2)')
 tair_dec = StrTrim(tair_dec,2)
 tair_dec =tair_dec + comma


if(t+2006 ge 2015) then begin
    prtstrg = slon+slat+" TAIR , AREA , "+syear+tair_sum+tair_maxval+tair_avg+tair_minval+tair_jan+tair_feb+tair_mar+tair_apr+tair_may+tair_jun+tair_jul+tair_aug+tair_sep+tair_oct+tair_nov+tair_dec+" WEST "

    printf,1,prtstrg
endif else begin
endelse

  endfor ;year loop
endfor  ;grid loop

  close, 1



end
