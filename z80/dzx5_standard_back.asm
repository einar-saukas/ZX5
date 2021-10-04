; -----------------------------------------------------------------------------
; ZX5 decoder by Einar Saukas
; "Standard" version (88 bytes only) - BACKWARDS VARIANT
; -----------------------------------------------------------------------------
; Parameters:
;   HL: last source address (compressed data)
;   DE: last destination address (decompressing)
; -----------------------------------------------------------------------------

dzx5_standard_back:
        ld      bc, 1                   ; preserve default offset 1
        push    bc
        ld      a, $80
dzx5sb_literals:
        call    dzx5sb_elias            ; obtain length
        lddr                            ; copy literals
        inc     c
        add     a, a                    ; copy from last offset or new offset?
        jr      c, dzx5sb_other_offset
dzx5sb_last_offset:
        call    dzx5sb_elias            ; obtain length
dzx5sb_copy:
        ex      (sp), hl                ; preserve source, restore offset
        push    hl                      ; preserve offset
        add     hl, de                  ; calculate destination - offset
        lddr                            ; copy from offset
        inc     c
        pop     hl                      ; restore offset
        ex      (sp), hl                ; preserve offset, restore source
        add     a, a                    ; copy from literals or new offset?
        jr      nc, dzx5sb_literals
dzx5sb_other_offset:
        add     a, a
        jr      nz, dzx5sb_other_offset_skip
        ld      a, (hl)                 ; load another group of 8 bits
        dec     hl
        rla
dzx5sb_other_offset_skip:
        exx
        jr      nc, dzx5sb_prev_offset
dzx5sb_new_offset:
        ex      de, hl                  ; copy 2nd last offset to 3rd last offset
        pop     hl                      ; copy last offset to 2nd last offset
        ld      b, a                    ; preserve next bit
        add     a, a
        exx
        call    dzx5sb_elias            ; obtain offset MSB
        dec     b
        ret     z                       ; check end marker
        dec     c                       ; adjust for positive offset
        ld      b, c
        ld      c, (hl)                 ; obtain offset LSB
        dec     hl
        inc     bc
        push    bc                      ; preserve new offset
        ld      bc, 1                   ; obtain length
        exx
        dec     b                       ; restore preserved bit
        exx
        call    m, dzx5sb_elias_backtrack
        inc     bc
        jr      dzx5sb_copy
dzx5sb_prev_offset:
        add     a, a
        jr      nc, dzx5sb_second_offset
        ex      de, hl
dzx5sb_second_offset:
        ex      (sp), hl
        exx
        jr      dzx5sb_last_offset
dzx5sb_elias_backtrack:
        add     a, a
        rl      c
        rl      b
dzx5sb_elias:
        add     a, a                    ; inverted interlaced Elias gamma coding
        jr      nz, dzx5sb_elias_skip
        ld      a, (hl)                 ; load another group of 8 bits
        dec     hl
        rla
dzx5sb_elias_skip:
        jr      c, dzx5sb_elias_backtrack
        ret
; -----------------------------------------------------------------------------
