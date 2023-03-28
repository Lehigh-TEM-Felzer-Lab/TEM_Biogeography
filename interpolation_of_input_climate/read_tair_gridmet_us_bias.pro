PRO read_tair_gridmet_us_bias


  close, 1
  close, 2

  openw, 1, "/share/partition2/climate/lehigh_river/gridmet/gridmet_bias_us.txt"
  openr,2,'/home/bsf208/tem/lonlat/lonlathurttlf_usa48.txt'
  tlon = fltarr(3381)
  tlat  = fltarr(3381)

  for i=0L,3380L do begin

  readf,2,xlon,xlat
    tlon(i) = xlon
    tlat(i) = xlat
;    print,"lon,lat = ",xlon,xlat
  endfor
  close,2

val = fltarr(3381,12,36)
tair = fltarr(1386,585,12)

for iyr = 1979,2014  do begin
;for iyr = 1979,1979  do begin
  yrar = iyr - 1979
  yr = STRING(iyr)
  yr = StrTrim(yr,2)
  filestart = STRING('/share/partition2/climate/lehigh_river/gridmet/tmmx_')
  fileend = STRING('.nc')
  filename = filestart + yr + fileend
  fileid = ncdf_open(filename)
;  fileid = ncdf_open("/share/partition2/climate/lehigh_river/gridmet/pr_1979.nc")
  fileid1=ncdf_varid(fileid,"air_temperature")
  fileid2=ncdf_varid(fileid,"lon")
  fileid3=ncdf_varid(fileid,"lat")
  fileid4=ncdf_varid(fileid,"day")
  print, "fileid = ", iyr,fileid1,fileid2,fileid3,fileid4
  ncdf_varget,fileid,fileid1,air_temperature
  tmax = air_temperature
  ncdf_varget,fileid,fileid2,lon
  ncdf_varget,fileid,fileid3,lat
  ncdf_varget,fileid,fileid4,day
  ncdf_close,fileid
  result1=n_elements(tmax)
  result2=n_elements(lon)
  result3=n_elements(lat)
  result4=n_elements(day)

  print, "n_elements = ", result1,result2,result3,result4

  filestart = STRING('/share/partition2/climate/lehigh_river/gridmet/tmmn_')
  fileend = STRING('.nc')
  filename = filestart + yr + fileend
  fileid = ncdf_open(filename)
;  fileid = ncdf_open("/share/partition2/climate/lehigh_river/gridmet/pr_1979.nc")
  fileid1=ncdf_varid(fileid,"air_temperature")
  fileid2=ncdf_varid(fileid,"lon")
  fileid3=ncdf_varid(fileid,"lat")
  fileid4=ncdf_varid(fileid,"day")
  print, "fileid = ", iyr,fileid1,fileid2,fileid3,fileid4
  ncdf_varget,fileid,fileid1,air_temperature
  tmin = air_temperature
  ncdf_varget,fileid,fileid2,lon
  ncdf_varget,fileid,fileid3,lat
  ncdf_varget,fileid,fileid4,day
  ncdf_close,fileid
  result1=n_elements(tmin)
  result2=n_elements(lon)
  result3=n_elements(lat)
  result4=n_elements(day)

  print, "n_elements = ", result1,result2,result3,result4



  air_temperature = ((0.1*tmax+220.0)+(0.1*tmin+210.0))/2.0 - 273.15

  if((iyr mod 4) ne 0) then begin
    dayval = 365
  endif else begin
    dayval = 366
  endelse

for i = 0, 1385 do begin
for j = 0, 584 do begin

 if(dayval eq 365) then begin
  tair(i,j,0) = mean(air_temperature(i,j,0:30))
  tair(i,j,1) = mean(air_temperature(i,j,31:58))
  tair(i,j,2) = mean(air_temperature(i,j,59:89))
  tair(i,j,3) = mean(air_temperature(i,j,90:119))
  tair(i,j,4) = mean(air_temperature(i,j,120:150))
  tair(i,j,5) = mean(air_temperature(i,j,151:180))
  tair(i,j,6) = mean(air_temperature(i,j,181:211))
  tair(i,j,7) = mean(air_temperature(i,j,212:242))
  tair(i,j,8) = mean(air_temperature(i,j,243:272))
  tair(i,j,9) = mean(air_temperature(i,j,273:303))
  tair(i,j,10) = mean(air_temperature(i,j,304:333))
  tair(i,j,11) = mean(air_temperature(i,j,334:364))
endif else begin
  tair(i,j,0) = mean(air_temperature(i,j,0:30))
  tair(i,j,1) = mean(air_temperature(i,j,31:59))
  tair(i,j,2) = mean(air_temperature(i,j,60:90))
  tair(i,j,3) = mean(air_temperature(i,j,91:120))
  tair(i,j,4) = mean(air_temperature(i,j,121:151))
  tair(i,j,5) = mean(air_temperature(i,j,152:181))
  tair(i,j,6) = mean(air_temperature(i,j,182:212))
  tair(i,j,7) = mean(air_temperature(i,j,213:243))
  tair(i,j,8) = mean(air_temperature(i,j,244:273))
  tair(i,j,9) = mean(air_temperature(i,j,274:304))
  tair(i,j,10) = mean(air_temperature(i,j,305:334))
  tair(i,j,11) = mean(air_temperature(i,j,335:365))

endelse

endfor
endfor


for i=0L,3380L do begin

   l = 0

   lonval =  ROUND((tlon(i)- (-124.766666633333))/0.041666666666666+1)-1
   if(lonval lt 0) then lonval = 0
   latval = ROUND((49.4-tlat(i))/0.041666666666666+1)-1
   if(latval gt 584) then latval = 584
;   print, "lonval, latval = ",lonval,latval


   for mon = 0,11 do begin

;   print,"lonval, latval, mon = ", lonval,latval,mon
   l=0
   val(i,mon,yrar) = 0.0
   if(lonval lt 1386 and latval lt 585) then begin
   if(tair(lonval,latval,mon) lt 3218 ) then begin
      val(i,mon,yrar) = tair(lonval,latval,mon)
      l = 1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp1 = ",tair(lonval,latval,mon),val(i,mon,yrar),l
   endif else begin
   endelse
   endif else begin
   endelse
   for z = 1,6 do begin
   if(lonval+z lt 1386 and latval+z lt 585 and lonval-z gt 0 and latval -z gt 0) then begin
   if(tair(lonval,latval+z,mon) lt 3218 ) then begin
      val(i,mon,yrar) = tair(lonval,latval+z,mon)
      l = 1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp1.1 = ",tair(lonval,latval+z,mon),val(i,mon,yrar),l,z
   endif else begin
   endelse
   if(tair(lonval,latval-z,mon) lt 3218 ) then begin
      val(i,mon,yrar) = tair(lonval,latval-z,mon)
      l = 1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp1.2 = ",tair(lonval,latval-z,mon),val(i,mon,yrar),l,z
   endif else begin
   endelse
   if(tair(lonval+z,latval,mon) lt 3218 ) then begin
      val(i,mon,yrar) = tair(lonval+z,latval,mon)
      l = 1
;    if(i eq 3380 and mon eq 0 and yrar eq 0) then print,"temp1.3 = ",tair(lonval+z,latval,mon),val(i,mon,yrar),l,z
   endif else begin
   endelse
   if(tair(lonval-z,latval,mon) lt 3218 ) then begin
      val(i,mon,yrar) = tair(lonval-z,latval,mon)
      l = 1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp1.4 = ",tair(lonval-z,latval,mon),val(i,mon,yrar),l,z
   endif else begin
   endelse
   endif else begin
   endelse
 endfor 

   for x = 1,6 do begin
    for y = 1,6 do begin
 
  if(lonval+x lt 1386 and latval+y lt 585 and lonval-x gt 0 and latval -y gt 0) then begin
  if(tair(lonval+x,latval+y,mon) lt 3218) then begin
     val(i,mon,yrar) = val(i,mon,yrar) + tair(lonval+x,latval+y,mon)
     l=l+1
;     if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp2 = ",tair(lonval+x,latval+y,mon),val(i,mon,yrar),l,x,y
  endif else begin
  endelse
  if(tair(lonval-x,latval-y,mon) lt 3218) then begin
    val(i,mon,yrar) = val(i,mon,yrar) + tair(lonval-x,latval-y,mon)
    l=l+1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp3 = ",tair(lonval-x,latval-y,mon),val(i,mon,yrar),l,x,y
  endif else begin
  endelse
  if(tair(lonval+x,latval-y,mon) lt 3218) then begin
    val(i,mon,yrar) = val(i,mon,yrar) + tair(lonval+x,latval-y,mon)
    l=l+1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp4 = ",tair(lonval+x,latval-y,mon),val(i,mon,yrar),l,x,y
  endif else begin
  endelse
  if(tair(lonval-x,latval+y,mon) lt 3218) then begin
    val(i,mon,yrar) = val(i,mon,yrar) + tair(lonval-x,latval+y,mon)
    l=l+1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp5 = ",tair(lonval-x,latval+y,mon),val(i,mon,yrar),l,x,y
  endif else begin
  endelse

  endif else begin
  endelse
;    if(i eq 0 ) then print,"values = ",lonval,latval,tair(lonval,latval,mon),tlon(i),min(macalon),tair(lonval+5,latval,mon),lon(0)-360.0
;    if(i eq 0 ) then print,"values = ",tair(0,latval,mon),tair(1,latval,mon),tair(2,latval,mon),tair(3,latval,mon),tair(4,latval,mon),tair(5,latval,mon)
    endfor  ;y loop
   endfor  ;x loop

if(val(i,mon,yrar) eq 0.0) then begin
   for z = 1,11 do begin
   if(lonval+z lt 1386 and latval+z lt 585 and lonval-z gt 0 and latval -z gt 0) then begin
   if(tair(lonval,latval+z,mon) lt 3218) then begin
      val(i,mon,yrar) = tair(lonval,latval+z,mon)
      l = 1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp1.1 = ",tair(lonval,latval+z,mon),val(i,mon,yrar),l,z
   endif else begin
   endelse
   if(tair(lonval,latval-z,mon) lt 3218) then begin
      val(i,mon,yrar) = tair(lonval,latval-z,mon)
      l = 1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp1.2 = ",tair(lonval,latval-z,mon),val(i,mon,yrar),l,z
   endif else begin
   endelse
   if(tair(lonval+z,latval,mon) lt 3218) then begin
      val(i,mon,yrar) = tair(lonval+z,latval,mon)
      l = 1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp1.3 = ",tair(lonval+z,latval,mon),val(i,mon,yrar),l,z
   endif else begin
   endelse
   if(tair(lonval-z,latval,mon) lt 3218) then begin
      val(i,mon,yrar) = tair(lonval-z,latval,mon)
      l = 1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp1.4 = ",tair(lonval-z,latval,mon),val(i,mon,yrar),l,z
   endif else begin
   endelse
   endif else begin
   endelse
 endfor

   for x = 1,11 do begin
    for y = 1,11 do begin
;  print,"x,y = ",x,y,i,mon
  if(lonval+x lt 1386 and latval+y lt 585 and lonval-x gt 0 and latval-y gt 0) then begin
  if(tair(lonval+x,latval+y,mon) lt 3218) then begin
     val(i,mon,yrar) = val(i,mon,yrar) + tair(lonval+x,latval+y,mon)
     l=l+1
;     if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp6 = ",tair(lonval+x,latval+y,mon),val(i,mon,yrar),l,x,y
  endif else begin
  endelse
  if(tair(lonval-x,latval-y,mon) lt 3218) then begin
     val(i,mon,yrar) = val(i,mon,yrar) + tair(lonval-x,latval-y,mon)
     l=l+1
;     if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp7 = ",tair(lonval-x,latval-y,mon),val(i,mon,yrar),l,x,y
  endif else begin
  endelse
  if(tair(lonval+x,latval-y,mon) lt 3218) then begin
    val(i,mon,yrar) = val(i,mon,yrar) + tair(lonval+x,latval-y,mon)
    l=l+1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp8 = ",tair(lonval+x,latval-y,mon),val(i,mon,yrar),l,x,y
  endif else begin
  endelse
  if(tair(lonval-x,latval+y,mon) lt 3218) then begin
    val(i,mon,yrar) = val(i,mon,yrar) + tair(lonval-x,latval+y,mon)
    l=l+1
;    if(i eq 3380 and mon eq 6 and yrar eq 0) then print,"temp9 = ",tair(lonval-x,latval+y,mon),val(i,mon,yrar),l,x,y
  endif else begin
  endelse

  endif else begin
  endelse
    endfor
  endfor
endif else begin
endelse
;   print,"l = ",l,val(i,mon),i,mon
   if(i eq 3380 and mon eq 6) then print,"temp = ", yrar, l, val(i,mon,yrar), val(i,mon,yrar)/l,tlon(i),tlat(i)
   val(i,mon,yrar) = (val(i,mon,yrar)/l)
;if(mon eq 6) then print,"values = ",val(i,mon,yrar),i,yrar,l
  endfor  ; mon loop
 endfor ; grid loop
endfor  ;year loop`

avg = fltarr(12)

for i=0L,3380L do begin
   for mon = 0,11 do begin

  avg(mon) = mean(val(i,mon,*))
;  if(i eq 3380) then print,"value = ",val(i,mon,0),i,mon,avg(mon)

   endfor

  printf,1,format = '(14f10.2)',tlon(i),tlat(i), avg(0),avg(1),avg(2),avg(3),avg(4),avg(5),avg(6),avg(7),avg(8),avg(9),avg(10),avg(11)

endfor


  close, 1

end
