; -----------------------------------------------------------------------------
; ZX5 decoder by Einar Saukas
; "Turbo" version (157 bytes, 16% faster) - BACKWARDS VARIANT
; Converted to backwards variant by Geir Bjerke
; -----------------------------------------------------------------------------
; Parameters:
;   HL: last source address (compressed data)
;   DE: last destination address (decompressing)
; -----------------------------------------------------------------------------

dzx5_turbo_back:
        ld      bc, 1                   ; preserve default offset 1
        ld      (dzx5tb_last_offset+1), bc
        ld      a, $80
        jr      dzx5tb_literals
dzx5tb_other_offset:
        add     a, a                    ; copy from previous offset or new offset?
        jr      nz, dzx5tb_other_offset_skip
        ld      a, (hl)                 ; load another group of 8 bits
        dec     hl
        rla
dzx5tb_other_offset_skip:
        exx
        jr      nc, dzx5tb_prev_offset
dzx5tb_new_offset:
        ex      de, hl                  ; copy 2nd last offset to 3rd last offset
        ld      hl, (dzx5tb_last_offset+1) ; copy last offset to 2nd last offset
        ld      b, a                    ; preserve next bit
        add     a, a
        exx
        inc     c                       ; prepare offset
        add     a, a
        jp      nz, dzx5tb_new_offset_skip
        ld      a, (hl)                 ; load another group of 8 bits
        dec     hl
        rla
dzx5tb_new_offset_skip:
        call    c, dzx5tb_elias         ; obtain offset MSB
        dec     b
        ret     z                       ; check end marker
        dec     c
        ld      b, c
        ld      c, (hl)                 ; obtain offset LSB
        dec     hl
        inc     bc
        ld      (dzx5tb_last_offset+1), bc ; preserve new offset
        ld      bc, 1                   ; obtain length
        exx
        dec     b                       ; restore preserved bit
        exx
        call    m, dzx5tb_elias
        inc     bc
dzx5tb_copy:
        push    hl                      ; preserve source
dzx5tb_last_offset:
        ld      hl, 0                   ; restore offset
        add     hl, de                  ; calculate destination + offset
        lddr                            ; copy from offset
        pop     hl                      ; restore source
        add     a, a                    ; copy from literals or another offset?
        jr      c, dzx5tb_other_offset
        inc     c                       ; obtain length
dzx5tb_literals:
        add     a, a
        jp      nz, dzx5tb_literals_skip
        ld      a, (hl)                 ; load another group of 8 bits
        dec     hl
        rla
dzx5tb_literals_skip:
        call    c, dzx5tb_elias
        lddr                            ; copy literals
        add     a, a                    ; copy from last offset or another offset?
        jr      c, dzx5tb_other_offset
dzx5tb_reuse_offset:
        inc     c                       ; obtain length
        add     a, a
        jp      nz, dzx5tb_last_offset_skip
        ld      a, (hl)                 ; load another group of 8 bits
        dec     hl
        rla
dzx5tb_last_offset_skip:
        call    c, dzx5tb_elias
        jp      dzx5tb_copy
dzx5tb_prev_offset:
        add     a, a                    ; copy from 2nd offset or 3rd offset?
        jr      nc, dzx5tb_second_offset
        ex      de, hl
dzx5tb_second_offset:
        ld      bc, (dzx5tb_last_offset+1)
        ld      (dzx5tb_last_offset+1), hl
        ld      h, b
        ld      l, c
        exx
        jp      dzx5tb_reuse_offset
dzx5tb_elias:
        add     a, a                    ; inverted interlaced Elias gamma coding
        rl      c
        add     a, a
        ret     nc
        jp      nz, dzx5tb_elias
        ld      a, (hl)                 ; load another group of 8 bits
        dec     hl
        rla
        ret     nc
        add     a, a
        rl      c
        add     a, a
        ret     nc
        add     a, a
        rl      c
        add     a, a
        ret     nc
        add     a, a
        rl      c
        add     a, a
dzx5tb_elias_loop:
        ret     nc
        add     a, a
        rl      c
        rl      b
        add     a, a
        jr      nz, dzx5tb_elias_loop
        ld      a, (hl)
        dec     hl
        rla
        jr      dzx5tb_elias_loop
; -----------------------------------------------------------------------------
