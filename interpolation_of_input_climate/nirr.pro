PRO nirr
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
        file_path = '/m8/kodero/raw_data/MACA_DATA/' + model_code + '/rsds_' + model_code + '_rcp85_2006_2099.nc'
        print, 'File:', file_path


        fileid = ncdf_open(file_path)
        fileid1=ncdf_varid(fileid,"surface_downwelling_shortwave_flux_in_air")
        fileid2=ncdf_varid(fileid,"lon")
        fileid3=ncdf_varid(fileid,"lat")
        fileid4=ncdf_varid(fileid,"time")
        print, "fileid = ", fileid1,fileid2,fileid3,fileid4
        ncdf_varget,fileid,fileid1, surface_downwelling_shortwave_flux_in_air
        ncdf_varget,fileid,fileid2,lon
        ncdf_varget,fileid,fileid3,lat
        ncdf_varget,fileid,fileid4,time
        ncdf_close,fileid
        result1=n_elements(surface_downwelling_shortwave_flux_in_air)
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


        openw, 1,width=1000, "/m8/kodero/raw_data/MACA_DATA/" + model_code + "/nirr.raw"

        ;lat =y
        ;lon =x
        ;lat = 580;
        ;lon = 640;
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

        val = fltarr(374,12,94)

        ;  for t = 0,59 do begin
        ;for t = 0,48,12 do begin
        for t = 0,1116,12 do begin
        ;for t = 0,0 do begin

        year = year + 1
        yrar = yrar + 1
        print,"year = ",year,t
        for i=0L,373L do begin
        ;   print,"i = ",i
        lonval =  ROUND((tlon(i)- (min(macalon)))/0.041666666666666+1)-1
        ;   latval = ROUND((max(macalat)-tlat(i))/0.041666666666666+1)-1
        latval = ROUND((tlat(i)- (min(macalat)))/0.041666666666666+1)-1
        ;   print,"lonval, latval = ",lonval, latval,tlon(i),tlat(i),lon(lonval)-360,lat(latval),surface_downwelling_shortwave_flux_in_air(lonval,latval,0)
        ;   wait,1


        ;  
        ;
        for mon = 0,11 do begin

        val(i,mon,yrar) = 0.0
        if(surface_downwelling_shortwave_flux_in_air(lonval,latval,t+mon) gt -9999 and lonval lt 509 and latval lt 495) then begin
          val(i,mon,yrar) = surface_downwelling_shortwave_flux_in_air(lonval,latval,t+mon)
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

        nirr_sum = STRING(sum, FORMAT='(F8.2)')
        nirr_sum = StrTrim(nirr_sum,2)
        nirr_sum = nirr_sum + comma
        nirr_avg = STRING(avg, FORMAT='(F7.2)')
        nirr_avg = StrTrim(nirr_avg,2)
        nirr_avg = nirr_avg + comma
        nirr_maxval = STRING(maxval, FORMAT='(F7.2)')
        nirr_maxval = StrTrim(nirr_maxval,2)
        nirr_maxval = nirr_maxval + comma
        nirr_minval = STRING(minval, FORMAT='(F7.2)')
        nirr_minval = StrTrim(nirr_minval,2)
        nirr_minval = nirr_minval + comma
        slon = STRING(tlon(l), FORMAT='(F9.3)')
        slon = StrTrim(slon,2)
        slon = slon + comma
        ;  print,"lon = ",macalon(l),l,slon
        slat = STRING(tlat(l), FORMAT='(F8.3)')
        slat = StrTrim(slat,2)
        slat = slat + comma
        syear = StrTrim(t+2006,2)
        syear = syear + comma
        nirr_jan = STRING(val(l,0,t), FORMAT='(F7.2)')
        nirr_jan = StrTrim(nirr_jan,2)
        nirr_jan = nirr_jan + comma
        nirr_feb = STRING(val(l,1,t), FORMAT='(F7.2)')
        nirr_feb = StrTrim(nirr_feb,2)
        nirr_feb = nirr_feb + comma
        nirr_mar = STRING(val(l,2,t), FORMAT='(F7.2)')
        nirr_mar = StrTrim(nirr_mar,2)
        nirr_mar = nirr_mar + comma
        nirr_apr = STRING(val(l,3,t), FORMAT='(F7.2)')
        nirr_apr = StrTrim(nirr_apr,2)
        nirr_apr = nirr_apr + comma
        nirr_may = STRING(val(l,4,t), FORMAT='(F7.2)')
        nirr_may = StrTrim(nirr_may,2)
        nirr_may = nirr_may + comma
        nirr_jun = STRING(val(l,5,t), FORMAT='(F7.2)')
        nirr_jun = StrTrim(nirr_jun,2)
        nirr_jun = nirr_jun + comma
        nirr_jul = STRING(val(l,6,t), FORMAT='(F7.2)')
        nirr_jul = StrTrim(nirr_jul,2)
        nirr_jul = nirr_jul + comma
        nirr_aug = STRING(val(l,7,t), FORMAT='(F7.2)')
        nirr_aug = StrTrim(nirr_aug,2)
        nirr_aug = nirr_aug + comma
        nirr_sep = STRING(val(l,8,t), FORMAT='(F7.2)')
        nirr_sep = StrTrim(nirr_sep,2)
        nirr_sep = nirr_sep + comma
        nirr_oct = STRING(val(l,9,t), FORMAT='(F7.2)')
        nirr_oct = StrTrim(nirr_oct,2)
        nirr_oct = nirr_oct + comma
        nirr_nov = STRING(val(l,10,t), FORMAT='(F7.2)')
        nirr_nov = StrTrim(nirr_nov,2)
        nirr_nov = nirr_nov + comma
        nirr_dec = STRING(val(l,11,t), FORMAT='(F7.2)')
        nirr_dec = StrTrim(nirr_dec,2)
        nirr_dec = nirr_dec + comma
        ;  print,"nirr = ",ajan(l),l,nirr
        ;  svpr = STRING(vpr, FORMAT='(F6.3)')
        ;  svpr = STRING(vpr, FORMAT='(F8.4)')
        ;  svpr = StrTrim(svpr,2)
        ;  svpr = svpr + comma
        ;  nirr = STRING(surface_downwelling_shortwave_flux_in_air(x,y,t), FORMAT='(F7.3)')
        ;  nirr = StrTrim(nirr,2)
        ;  nirr = nirr + comma
        if(t+2006 ge 2015) then begin
        prtstrg = slon+slat+" NIRR , AREA ,"+syear+nirr_sum+nirr_maxval+nirr_avg+nirr_minval+nirr_jan+nirr_feb+nirr_mar+nirr_apr+nirr_may+nirr_jun+nirr_jul+nirr_aug+nirr_sep+nirr_oct+nirr_nov+nirr_dec+" WEST "
        ;    printf,1,slon,slat," nirr ,5,",syear,nirr_jan,nirr_feb,nirr_mar,nirr_apr,nirr_may,nirr_jun,nirr_jul,nirr_aug,nirr_sep,nirr_oct,nirr_nov,nirr_dec," USA48"
        printf,1,prtstrg
        endif else begin
        endelse

        endfor ;year loop
        endfor  ;grid loop




        close, 1

endfor
  
  end
