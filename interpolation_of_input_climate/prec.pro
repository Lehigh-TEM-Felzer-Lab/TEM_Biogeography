PRO prec


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
        file_path = '/m8/kodero/raw_data/MACA_DATA/' + model_code + '/pr_' + model_code + '_rcp85_2006_2099.nc'
        print, 'File:', file_path

        fileid = ncdf_open(file_path)
        fileid1=ncdf_varid(fileid,"precipitation")
        fileid2=ncdf_varid(fileid,"lon")
        fileid3=ncdf_varid(fileid,"lat")
        fileid4=ncdf_varid(fileid,"time")
        print, "fileid = ", fileid1,fileid2,fileid3,fileid4
        ncdf_varget,fileid,fileid1, precipitation
        ncdf_varget,fileid,fileid2,lon
        ncdf_varget,fileid,fileid3,lat
        ncdf_varget,fileid,fileid4,time
        ncdf_close,fileid
        result1=n_elements(precipitation)
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



        openw, 1,width=1000, "/m8/kodero/raw_data/MACA_DATA/" + model_code + "/pr.raw"

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
        if(precipitation(lonval,latval,t+mon) gt -9999 and lonval lt 509 and latval lt 495) then begin
        val(i,mon,yrar) = precipitation(lonval,latval,t+mon)
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

        prec_sum = STRING(sum, FORMAT='(F8.2)')
        prec_sum = StrTrim(prec_sum,2)
        prec_sum = prec_sum + comma
        prec_avg = STRING(avg, FORMAT='(F7.2)')
        prec_avg = StrTrim(prec_avg,2)
        prec_avg = prec_avg + comma
        prec_maxval = STRING(maxval, FORMAT='(F7.2)')
        prec_maxval = StrTrim(prec_maxval,2)
        prec_maxval = prec_maxval + comma
        prec_minval = STRING(minval, FORMAT='(F7.2)')
        prec_minval = StrTrim(prec_minval,2)
        prec_minval = prec_minval + comma
        slon = STRING(tlon(l), FORMAT='(F9.3)')
        slon = StrTrim(slon,2)
        slon = slon + comma
        slat = STRING(tlat(l), FORMAT='(F8.3)')
        slat = StrTrim(slat,2)
        slat = slat + comma
        syear = StrTrim(t+2006,2)
        syear = syear + comma
        prec_jan = STRING(val(l,0,t), FORMAT='(F7.2)')
        prec_jan = StrTrim(prec_jan,2)
        prec_jan = prec_jan + comma
        prec_feb = STRING(val(l,1,t), FORMAT='(F7.2)')
        prec_feb = StrTrim(prec_feb,2)
        prec_feb = prec_feb + comma
        prec_mar = STRING(val(l,2,t), FORMAT='(F7.2)')
        prec_mar = StrTrim(prec_mar,2)
        prec_mar = prec_mar + comma
        prec_apr = STRING(val(l,3,t), FORMAT='(F7.2)')
        prec_apr = StrTrim(prec_apr,2)
        prec_apr = prec_apr + comma
        prec_may = STRING(val(l,4,t), FORMAT='(F7.2)')
        prec_may = StrTrim(prec_may,2)
        prec_may = prec_may + comma
        prec_jun = STRING(val(l,5,t), FORMAT='(F7.2)')
        prec_jun = StrTrim(prec_jun,2)
        prec_jun = prec_jun + comma
        prec_jul = STRING(val(l,6,t), FORMAT='(F7.2)')
        prec_jul = StrTrim(prec_jul,2)
        prec_jul = prec_jul + comma
        prec_aug = STRING(val(l,7,t), FORMAT='(F7.2)')
        prec_aug = StrTrim(prec_aug,2)
        prec_aug = prec_aug + comma
        prec_sep = STRING(val(l,8,t), FORMAT='(F7.2)')
        prec_sep = StrTrim(prec_sep,2)
        prec_sep = prec_sep + comma
        prec_oct = STRING(val(l,9,t), FORMAT='(F7.2)')
        prec_oct = StrTrim(prec_oct,2)
        prec_oct = prec_oct + comma
        prec_nov = STRING(val(l,10,t), FORMAT='(F7.2)')
        prec_nov = StrTrim(prec_nov,2)
        prec_nov = prec_nov + comma
        prec_dec = STRING(val(l,11,t), FORMAT='(F7.2)')
        prec_dec = StrTrim(prec_dec,2)
        prec_dec = prec_dec + comma

        if(t+2006 ge 2015) then begin
        prtstrg = slon+slat+" PREC , AREA ,"+syear+prec_sum+prec_maxval+prec_avg+prec_minval+prec_jan+prec_feb+prec_mar+prec_apr+prec_may+prec_jun+prec_jul+prec_aug+prec_sep+prec_oct+prec_nov+prec_dec+" WEST "

        printf,1,prtstrg
        endif else begin
        endelse

        endfor ;year loop
        endfor  ;grid loop

        close, 1
endfor

end
