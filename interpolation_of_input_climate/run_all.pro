PRO run_all
    COMPILE_OPT IDL2

    ; Compile and run each program
    RESOLVE_ROUTINE, 'dtr', /COMPILE_FULL_FILE
    dtr

    RESOLVE_ROUTINE, 'nirr', /COMPILE_FULL_FILE
    nirr

    RESOLVE_ROUTINE, 'prec', /COMPILE_FULL_FILE
    prec

    RESOLVE_ROUTINE, 'tair', /COMPILE_FULL_FILE
    tair
   
    RESOLVE_ROUTINE, 'vpr', /COMPILE_FULL_FILE
    vpr

    RESOLVE_ROUTINE, 'was', /COMPILE_FULL_FILE
    was

END

