PRO read_maca_wind_west


  close, 1
  close, 2

  comma = ","

  fileid = ncdf_open("/m8/kodero/raw_data/MACA_DATA/austarlia/agg_macav2metdata_was_CSIRO-Mk3-6-0_r1i1p1_rcp85_2006_2099_CONUS_monthly.nc")
  fileid1=ncdf_varid(fileid,"wind_speed")
  fileid2=ncdf_varid(fileid,"lon")
  fileid3=ncdf_varid(fileid,"lat")
  fileid4=ncdf_varid(fileid,"time")
  print, "fileid = ", fileid1,fileid2,fileid3,fileid4
  ncdf_varget,fileid,fileid1, wind_speed
  ncdf_varget,fileid,fileid2,lon
  ncdf_varget,fileid,fileid3,lat
  ncdf_varget,fileid,fileid4,time
  ncdf_close,fileid
  result1=n_elements(wind_speed)
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


   openw, 1,width=1000, "was.csv"

   ;lat =y
;lon =x
;lat = 580;
    ;lon = 640;
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

val = fltarr(374,12,94)


for t = 0,1116,12 do begin


     year = year + 1
     yrar = yrar + 1
     print,"year = ",year,t
for i=0L,373L do begin

   lonval =  ROUND((tlon(i)- (min(macalon)))/0.041666666666666+1)-1

   latval = ROUND((tlat(i)- (min(macalat)))/0.041666666666666+1)-1

;   wait,1


;  
;
   for mon = 0,11 do begin

   val(i,mon,yrar) = 0.0
   if(wind_speed(lonval,latval,t+mon) gt -9999 andlonval lt 595 and latval lt 584) then begin
      val(i,mon,yrar) = wind_speed(lonval,latval,t+mon)
   endif else begin
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

  wind_sum = STRING(sum, FORMAT='(F8.2)')
  wind_sum = StrTrim(wind_sum,2)
  wind_sum = wind_sum + comma
  wind_avg = STRING(avg, FORMAT='(F7.2)')
  wind_avg = StrTrim(wind_avg,2)
  wind_avg = wind_avg + comma
  wind_maxval = STRING(maxval, FORMAT='(F7.2)')
  wind_maxval = StrTrim(wind_maxval,2)
  wind_maxval = wind_maxval + comma
  wind_minval = STRING(minval, FORMAT='(F7.2)')
  wind_minval = StrTrim(wind_minval,2)
  wind_minval = wind_minval + comma
  slon = STRING(tlon(l), FORMAT='(F9.3)')
  slon = StrTrim(slon,2)
  slon = slon + comma
  slat = STRING(tlat(l), FORMAT='(F8.3)')
  slat = StrTrim(slat,2)
  slat = slat + comma
  syear = StrTrim(t+2006,2)
  syear = syear + comma
  wind_jan = STRING(val(l,0,t), FORMAT='(F7.2)')
  wind_jan = StrTrim(wind_jan,2)
  wind_jan = wind_jan + comma
  wind_feb = STRING(val(l,1,t), FORMAT='(F7.2)')
  wind_feb = StrTrim(wind_feb,2)
  wind_feb = wind_feb + comma
  wind_mar = STRING(val(l,2,t), FORMAT='(F7.2)')
  wind_mar = StrTrim(wind_mar,2)
  wind_mar = wind_mar + comma
  wind_apr = STRING(val(l,3,t), FORMAT='(F7.2)')
  wind_apr = StrTrim(wind_apr,2)
  wind_apr = wind_apr + comma
  wind_may = STRING(val(l,4,t), FORMAT='(F7.2)')
  wind_may = StrTrim(wind_may,2)
  wind_may = wind_may + comma
  wind_jun = STRING(val(l,5,t), FORMAT='(F7.2)')
  wind_jun = StrTrim(wind_jun,2)
  wind_jun = wind_jun + comma
  wind_jul = STRING(val(l,6,t), FORMAT='(F7.2)')
  wind_jul = StrTrim(wind_jul,2)
  wind_jul = wind_jul + comma
  wind_aug = STRING(val(l,7,t), FORMAT='(F7.2)')
  wind_aug = StrTrim(wind_aug,2)
  wind_aug = wind_aug + comma
  wind_sep = STRING(val(l,8,t), FORMAT='(F7.2)')
  wind_sep = StrTrim(wind_sep,2)
  wind_sep = wind_sep + comma
  wind_oct = STRING(val(l,9,t), FORMAT='(F7.2)')
  wind_oct = StrTrim(wind_oct,2)
  wind_oct = wind_oct + comma
  wind_nov = STRING(val(l,10,t), FORMAT='(F7.2)')
  wind_nov = StrTrim(wind_nov,2)
  wind_nov = wind_nov + comma
  wind_dec = STRING(val(l,11,t), FORMAT='(F7.2)')
  wind_dec = StrTrim(wind_dec,2)
  wind_dec = wind_dec + comma

if(t+2006 ge 2015) then begin
    prtstrg = slon+slat+" WIND , AREA ,"+syear+wind_sum+wind_maxval+wind_avg+wind_minval+wind_jan+wind_feb+wind_mar+wind_apr+wind_may+wind_jun+wind_jul+wind_aug+wind_sep+wind_oct+wind_nov+wind_dec+" WEST "

    printf,1,prtstrg
endif else begin
endelse

  endfor ;year loop
endfor  ;grid loop

  close, 1


end
