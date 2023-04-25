PRO dtr




  close, 1
  close, 2

  comma = ","
 close, 1
    close, 2

    comma = ","

    ; Define the folder_names array
    folder_names = ['bcc_csm1_1', 'bcc_csm1_1_m', 'bnu_esm', 'canesm2', 'ccsm4', 'cnrm_cm5', 'csiro_mk3_6_0', $
                    'gfdl_esm2g', 'gfdl_esm2m', 'hadgem2_cc365', 'hadgem2_es365', 'inmcm4', 'ipsl_cm5a_lr', $
                    'ipsl_cm5a_mr', 'ipsl_cm5b_lr', 'miroc5', 'miroc_esm', 'miroc_esm_chem', 'mri_cgcm3', $
                    'noresm1_m']

    ; Get the number of elements in the array
    n_elements = N_ELEMENTS(folder_names)
    print, 'Number of elements in the array:', n_elements

    ; Loop over the elements of the array
    for model_idx = 0, n_elements - 1 DO BEGIN
        ; Access the current element
        model_code = folder_names[model_idx]
        print, 'Model code:', model_code

        ; Update file_path with the current_folder_name
        file_path = '/m8/kodero/raw_data/MACA_DATA/' + model_code + '/tasmax_' + model_code + '_rcp85_2006_2099.nc'
        print, 'File:', file_path


        fileid = ncdf_open(file_path)
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


        file_path = '/m8/kodero/raw_data/MACA_DATA/' + model_code + '/tasmin_' + model_code + '_rcp85_2006_2099.nc'
        print, 'File:', file_path

        fileid = ncdf_open(file_path)
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



        openw, 1,width=1000, "/m8/kodero/raw_data/MACA_DATA/" + model_code + "/dtr.raw"


        macalon = fltarr(251955)
        macalat = fltarr(251955)


        l=0
        for y = 0,494 do begin
        ;  print,"y = ",y
        for x = 0,508 do begin
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

        dtr = tmax -  tmin

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
        ;print, "diag = ",latval,n_elements(dtr)," ",dtr(0,21,0)
        if(dtr(lonval,latval,t+mon) gt -9999 and lonval lt 509 and latval lt 495) then begin
        val(i,mon,yrar) = dtr(lonval,latval,t+mon)
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

        dtr_sum = STRING(sum, FORMAT='(F8.2)')
        dtr_sum = StrTrim(dtr_sum,2)
        dtr_sum =dtr_sum + comma
        dtr_avg = STRING(avg, FORMAT='(F7.2)')
        dtr_avg = StrTrim(dtr_avg,2)
        dtr_avg =dtr_avg + comma
        dtr_maxval = STRING(maxval, FORMAT='(F7.2)')
        dtr_maxval = StrTrim(dtr_maxval,2)
        dtr_maxval =dtr_maxval + comma
        dtr_minval = STRING(minval, FORMAT='(F7.2)')
        dtr_minval = StrTrim(dtr_minval,2)
        dtr_minval =dtr_minval + comma
        slon = STRING(tlon(l), FORMAT='(F9.3)')
        slon = StrTrim(slon,2)
        slon = slon + comma
        ;  print,"lon = ",macalon(l),l,slon
        slat = STRING(tlat(l), FORMAT='(F8.3)')
        slat = StrTrim(slat,2)
        slat = slat + comma
        syear = StrTrim(t+2006,2)
        syear = syear + comma
        dtr_jan = STRING(val(l,0,t), FORMAT='(F7.2)')
        dtr_jan = StrTrim(dtr_jan,2)
        dtr_jan =dtr_jan + comma
        dtr_feb = STRING(val(l,1,t), FORMAT='(F7.2)')
        dtr_feb = StrTrim(dtr_feb,2)
        dtr_feb =dtr_feb + comma
        dtr_mar = STRING(val(l,2,t), FORMAT='(F7.2)')
        dtr_mar = StrTrim(dtr_mar,2)
        dtr_mar =dtr_mar + comma
        dtr_apr = STRING(val(l,3,t), FORMAT='(F7.2)')
        dtr_apr = StrTrim(dtr_apr,2)
        dtr_apr =dtr_apr + comma
        dtr_may = STRING(val(l,4,t), FORMAT='(F7.2)')
        dtr_may = StrTrim(dtr_may,2)
        dtr_may =dtr_may + comma
        dtr_jun = STRING(val(l,5,t), FORMAT='(F7.2)')
        dtr_jun = StrTrim(dtr_jun,2)
        dtr_jun =dtr_jun + comma
        dtr_jul = STRING(val(l,6,t), FORMAT='(F7.2)')
        dtr_jul = StrTrim(dtr_jul,2)
        dtr_jul =dtr_jul + comma
        dtr_aug = STRING(val(l,7,t), FORMAT='(F7.2)')
        dtr_aug = StrTrim(dtr_aug,2)
        dtr_aug =dtr_aug + comma
        dtr_sep = STRING(val(l,8,t), FORMAT='(F7.2)')
        dtr_sep = StrTrim(dtr_sep,2)
        dtr_sep =dtr_sep + comma
        dtr_oct = STRING(val(l,9,t), FORMAT='(F7.2)')
        dtr_oct = StrTrim(dtr_oct,2)
        dtr_oct =dtr_oct + comma
        dtr_nov = STRING(val(l,10,t), FORMAT='(F7.2)')
        dtr_nov = StrTrim(dtr_nov,2)
        dtr_nov =dtr_nov + comma
        dtr_dec = STRING(val(l,11,t), FORMAT='(F7.2)')
        dtr_dec = StrTrim(dtr_dec,2)
        dtr_dec =dtr_dec + comma


        if(t+2006 ge 2015) then begin
        prtstrg = slon+slat+" DTAIRRNG , AREA , "+syear+dtr_sum+dtr_maxval+dtr_avg+dtr_minval+dtr_jan+dtr_feb+dtr_mar+dtr_apr+dtr_may+dtr_jun+dtr_jul+dtr_aug+dtr_sep+dtr_oct+dtr_nov+dtr_dec+" WEST "

        printf,1,prtstrg
        endif else begin
        endelse

        endfor ;year loop
        endfor  ;grid loop

        close, 1

endfor

end
