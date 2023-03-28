PRO read_maca_vpr_west


  close, 1
  close, 2

  comma = ","

  fileid = ncdf_open("/m8/kodero/raw_data/MACA_DATA/austarlia/agg_macav2metdata_huss_CSIRO-Mk3-6-0_r1i1p1_rcp85_2006_2099_CONUS_monthly.nc")
  fileid1=ncdf_varid(fileid,"specific_humidity")
  fileid2=ncdf_varid(fileid,"lon")
  fileid3=ncdf_varid(fileid,"lat")
  fileid4=ncdf_varid(fileid,"time")
  print, "fileid = ", fileid1,fileid2,fileid3,fileid4
  ncdf_varget,fileid,fileid1, specific_humidity
  ncdf_varget,fileid,fileid2,lon
  ncdf_varget,fileid,fileid3,lat
  ncdf_varget,fileid,fileid4,time
  ncdf_close,fileid
  result1=n_elements(specific_humidity)
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


   openw, 1,width=1000, "/m8/kodero/runs/australia/climate/vpr_CSIRO-Mk3-6-0_rcp85_2006_2099.csv"

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
   if(specific_humidity(lonval,latval,t+mon) gt -9999 and lonval lt 595 and latval lt 584) then begin
   val(i,mon,yrar) = specific_humidity(lonval,latval,t+mon) * 1010 / (0.622+0.328*specific_humidity(lonval,latval,t+mon) )
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

  vpr_sum = STRING(sum, FORMAT='(F8.2)')
  vpr_sum = StrTrim(vpr_sum,2)
  vpr_sum = vpr_sum + comma
  vpr_avg = STRING(avg, FORMAT='(F7.2)')
  vpr_avg = StrTrim(vpr_avg,2)
  vpr_avg = vpr_avg + comma
  vpr_maxval = STRING(maxval, FORMAT='(F7.2)')
  vpr_maxval = StrTrim(vpr_maxval,2)
  vpr_maxval = vpr_maxval + comma
  vpr_minval = STRING(minval, FORMAT='(F7.2)')
  vpr_minval = StrTrim(vpr_minval,2)
  vpr_minval = vpr_minval + comma
  slon = STRING(tlon(l), FORMAT='(F9.3)')
  slon = StrTrim(slon,2)
  slon = slon + comma
  slat = STRING(tlat(l), FORMAT='(F8.3)')
  slat = StrTrim(slat,2)
  slat = slat + comma
  syear = StrTrim(t+2006,2)
  syear = syear + comma
  vpr_jan = STRING(val(l,0,t), FORMAT='(F7.2)')
  vpr_jan = StrTrim(vpr_jan,2)
  vpr_jan = vpr_jan + comma
  vpr_feb = STRING(val(l,1,t), FORMAT='(F7.2)')
  vpr_feb = StrTrim(vpr_feb,2)
  vpr_feb = vpr_feb + comma
  vpr_mar = STRING(val(l,2,t), FORMAT='(F7.2)')
  vpr_mar = StrTrim(vpr_mar,2)
  vpr_mar = vpr_mar + comma
  vpr_apr = STRING(val(l,3,t), FORMAT='(F7.2)')
  vpr_apr = StrTrim(vpr_apr,2)
  vpr_apr = vpr_apr + comma
  vpr_may = STRING(val(l,4,t), FORMAT='(F7.2)')
  vpr_may = StrTrim(vpr_may,2)
  vpr_may = vpr_may + comma
  vpr_jun = STRING(val(l,5,t), FORMAT='(F7.2)')
  vpr_jun = StrTrim(vpr_jun,2)
  vpr_jun = vpr_jun + comma
  vpr_jul = STRING(val(l,6,t), FORMAT='(F7.2)')
  vpr_jul = StrTrim(vpr_jul,2)
  vpr_jul = vpr_jul + comma
  vpr_aug = STRING(val(l,7,t), FORMAT='(F7.2)')
  vpr_aug = StrTrim(vpr_aug,2)
  vpr_aug = vpr_aug + comma
  vpr_sep = STRING(val(l,8,t), FORMAT='(F7.2)')
  vpr_sep = StrTrim(vpr_sep,2)
  vpr_sep = vpr_sep + comma
  vpr_oct = STRING(val(l,9,t), FORMAT='(F7.2)')
  vpr_oct = StrTrim(vpr_oct,2)
  vpr_oct = vpr_oct + comma
  vpr_nov = STRING(val(l,10,t), FORMAT='(F7.2)')
  vpr_nov = StrTrim(vpr_nov,2)
  vpr_nov = vpr_nov + comma
  vpr_dec = STRING(val(l,11,t), FORMAT='(F7.2)')
  vpr_dec = StrTrim(vpr_dec,2)
  vpr_dec = vpr_dec + comma

if(t+2006 ge 2015) then begin
    prtstrg = slon+slat+" VPRPRESS , AREA ,"+syear+vpr_sum+vpr_maxval+vpr_avg+vpr_minval+vpr_jan+vpr_feb+vpr_mar+vpr_apr+vpr_may+vpr_jun+vpr_jul+vpr_aug+vpr_sep+vpr_oct+vpr_nov+vpr_dec+" WEST "

    printf,1,prtstrg
endif else begin
endelse

  endfor ;year loop
endfor  ;grid loop

  close, 1


end
