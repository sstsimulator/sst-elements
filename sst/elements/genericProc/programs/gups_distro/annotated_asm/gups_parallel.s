         # ident     gups_parallel$c
         # source file    "/home/eecs/brg/dis/stresscode/gups_parallel.c"
         # vcc 4.21.0 
         # -----------------------------------------
         .section    .data, 1, 0x00000003, 0, 8
         .globl      indices
         .size       indices, 4
         .align      3
indices:
         .align      0
         .space      4
         # -----------------------------------------
         .section    .data, 1, 0x00000003, 0, 8
         .globl      feedbackterms
         .size       feedbackterms, 136
         .align      3
feedbackterms:
         .align      0
         # offset = 0
         .word       0         # feedbackterms
         # offset = 4
         .word       0         # feedbackterms
         # offset = 8
         .word       0         # feedbackterms
         # offset = 12
         .word       0         # feedbackterms
         # offset = 16
         .word       9         # feedbackterms
         # offset = 20
         .word       18         # feedbackterms
         # offset = 24
         .word       33         # feedbackterms
         # offset = 28
         .word       65         # feedbackterms
         # offset = 32
         .word       142         # feedbackterms
         # offset = 36
         .word       264         # feedbackterms
         # offset = 40
         .word       516         # feedbackterms
         # offset = 44
         .word       1026         # feedbackterms
         # offset = 48
         .word       2089         # feedbackterms
         # offset = 52
         .word       4109         # feedbackterms
         # offset = 56
         .word       8213         # feedbackterms
         # offset = 60
         .word       16385         # feedbackterms
         # offset = 64
         .word       32790         # feedbackterms
         # offset = 68
         .word       65540         # feedbackterms
         # offset = 72
         .word       131091         # feedbackterms
         # offset = 76
         .word       262163         # feedbackterms
         # offset = 80
         .word       524292         # feedbackterms
         # offset = 84
         .word       1048578         # feedbackterms
         # offset = 88
         .word       2097153         # feedbackterms
         # offset = 92
         .word       4194320         # feedbackterms
         # offset = 96
         .word       8388621         # feedbackterms
         # offset = 100
         .word       16777220         # feedbackterms
         # offset = 104
         .word       33554467         # feedbackterms
         # offset = 108
         .word       67108883         # feedbackterms
         # offset = 112
         .word       134217732         # feedbackterms
         # offset = 116
         .word       268435458         # feedbackterms
         # offset = 120
         .word       536870953         # feedbackterms
         # offset = 124
         .word       1073741828         # feedbackterms
         # offset = 128
         .word       -2147483561         # feedbackterms
         .space      4
         # -----------------------------------------
         .section    .data, 1, 0x00000003, 0, 8
         .globl      lfsr_bits
         .size       lfsr_bits, 4
         .align      3
lfsr_bits:
         .align      0
         # offset = 0
         .word       0         # lfsr_bits
         # -----------------------------------------
         .section    .data, 1, 0x00000003, 0, 8
         .globl      lfsr_state
         .size       lfsr_state, 4
         .align      3
lfsr_state:
         .align      0
         # offset = 0
         .word       0         # lfsr_state
         # -----------------------------------------
         .section    .data,1, 0x00000003, 0, 8
         .align      3
__gstatic:
         .align      0
         # offset = 0
         .ascii      "Error: couldn't "  # _Fe_variable_0
         .ascii      "malloc space for"
         .ascii      " array of indice"
         .ascii      "s"
         .byte       0xa,00
         .space      5
         # offset = 56
         .ascii      "EX"  # _Fe_variable_1
         .byte       00
         .space      5
         # offset = 64
         .ascii      "gups_parallel.c"  # _Fe_variable_2
         .byte       00
         # offset = 80
         .ascii      "Warning: uint8 i"  # _Fe_variable_3
         .ascii      "s %i bits. Check"
         .ascii      " typedefs and re"
         .ascii      "compile."
         .byte       0xa,00
         .space      6
         # offset = 144
         .ascii      "Warning: uint16 "  # _Fe_variable_4
         .ascii      "is %i bits. Chec"
         .ascii      "k typedefs and r"
         .ascii      "ecompile."
         .byte       0xa,00
         .space      5
         # offset = 208
         .ascii      "Warning: uint32 "  # _Fe_variable_5
         .ascii      "is %i bits. Chec"
         .ascii      "k typedefs and r"
         .ascii      "ecompile."
         .byte       0xa,00
         .space      5
         # offset = 272
         .ascii      "Warning: uint64 "  # _Fe_variable_6
         .ascii      "is %i bits. Chec"
         .ascii      "k typedefs and r"
         .ascii      "ecompile."
         .byte       0xa,00
         .space      5
         # offset = 336
         .ascii      "%llu %f %llu"  # _Fe_variable_7
         .byte       00
         .space      3
         # offset = 352
         .ascii      "%llu updates, "  # _Fe_variable_8
         .byte       00
         .space      1
         # offset = 368
         .ascii      "%llu-bit-wide da"  # _Fe_variable_9
         .ascii      "ta, "
         .byte       00
         .space      3
         # offset = 392
         .ascii      "field of 2^%.2f "  # _Fe_variable_10
         .ascii      "(%llu) bytes."
         .byte       0xa,00
         .space      1
         # offset = 424
         .ascii      "Error: Failed to"  # _Fe_variable_11
         .ascii      " malloc %llu byt"
         .ascii      "es."
         .byte       0xa,00
         .space      3
         # offset = 464
         .ascii      "Selecting lfsr w"  # _Fe_variable_12
         .ascii      "idth %d."
         .byte       0xa,00
         .space      6
         # offset = 496
         .ascii      "Element size is "  # _Fe_variable_13
         .ascii      "%llu bytes."
         .byte       0xa,00
         .space      3
         # offset = 528
         .ascii      "Field is %llu da"  # _Fe_variable_14
         .ascii      "ta elements star"
         .ascii      "ting at 0x%08lx."
         .byte       0xa,00
         .space      6
         # offset = 584
         .ascii      "Calculating indi"  # _Fe_variable_15
         .ascii      "ces."
         .byte       0xa,00
         .space      2
         # offset = 608
         .ascii      "Timing."  # _Fe_variable_16
         .byte       0xa,00
         .space      7
         # offset = 624
         .ascii      "Elapsed time: %."  # _Fe_variable_17
         .ascii      "4f seconds."
         .byte       0xa,00
         .space      3
         # offset = 656
         .ascii      "GUPS = %.10f"  # _Fe_variable_18
         .byte       0xa,00
         .space      2
         # offset = 672
         .ascii      "$Revision: 1.75 "  # _Fe_variable_19
         .ascii      "$"
         .byte       00
         .space      6
         # offset = 696
         .ascii      "$Id: standards.h"  # _Fe_variable_20
         .ascii      ",v 1.19 1997/12/"
         .ascii      "15 21:30:25 jph "
         .ascii      "Exp $"
         .byte       00
         .space      2
         # offset = 752
         .ascii      "$Revision: 1.11 "  # _Fe_variable_21
         .ascii      "$"
         .byte       00
         .space      6
         # offset = 776
         .ascii      "$Revision: 1.11 "  # _Fe_variable_22
         .ascii      "$"
         .byte       00
         .space      6
         # offset = 800
         .ascii      "$Revision: 1.78 "  # _Fe_variable_23
         .ascii      "$"
         .byte       00
         .space      6
         # offset = 824
         .ascii      "$Revision: 1.1 $"  # _Fe_variable_24
         .byte       00
         .space      7
         # offset = 848
         .ascii      "$Revision: 1.17 "  # _Fe_variable_25
         .ascii      "$"
         .byte       00
         .space      6
         # offset = 872
         .ascii      "$Revision: 3.53 "  # _Fe_variable_26
         .ascii      "$"
         .byte       00
         .space      6
         # offset = 896
         .ascii      "$Revision: 1.1 $"  # _Fe_variable_27
         .byte       00
         .space      7
         # offset = 920
         .ascii      "$Revision: 3.138"  # _Fe_variable_28
         .ascii      " $"
         .byte       00
         .space      5
         # offset = 944
         .ascii      "$Revision: 1.1 $"  # _Fe_variable_29
         .byte       00
         .space      7
         # offset = 968
         .ascii      "$Revision: 1.17 "  # _Fe_variable_30
         .ascii      "$"
         .byte       00
         .space      6
         # offset = 992
         .ascii      "$Revision: 1.41 "  # _Fe_variable_31
         .ascii      "$"
         .byte       00
         .space      6
         # offset = 1016
         .ascii      "$Revision: 1.51 "  # _Fe_variable_32
         .ascii      "$"
         .byte       00
         .space      6
         # offset = 1040
         .ascii      "$Id: ptimers.h,v"  # _Fe_variable_33
         .ascii      " 1.4 1997/01/15 "
         .ascii      "04:44:41 jwag Ex"
         .ascii      "p $"
         .byte       00
         .space      4
         # -----------------------------------------
         # -----------------------------------------
         #     Routine fake_gettimeofday
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        fake_gettimeofday
fake_gettimeofday:
         .frame      $sp, 128, $31
         .mask       0xc0010000,-96
         dsubu       $sp,$sp,128         
         sd          $31,24($sp)
         sd          $fp,16($sp)
         sd          $16,8($sp)
         move        $fp,$sp
         move        $16,$4              
         dla         $25,time            	              #    13,     2
         move        $4,$0               	              #    13,     2
         jalr        $25                 	              #    13,     2
         sw          $2,0($16)           	              #    13,     2 0[(int) tp].B
         sw          $0,4($16)           	              #    14,     4 4[(int) @SPLTMR_tp_P0].B
         move        $2,$0               	              #    15,     5
         move        $sp,$fp
         ld          $31,24($sp)
         ld          $fp,16($sp)
         ld          $16,8($sp)
         daddu       $sp,$sp,128         	              #    15,     5
         jr          $31                 	              #    15,     5
         .end        fake_gettimeofday
         # -----------------------------------------
         #     Routine lfsr
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        lfsr
lfsr:
         .frame      $sp, 112, $31
         .mask       0x40000000,-96
         dsubu       $sp,$sp,112         
         sd          $fp,8($sp)
         move        $fp,$sp
         dla         $3,lfsr_state       	              #    37,     2
         lwu         $13,0($3)           	              #    37,     2 lfsr_state
         and         $2,$13,1            	              #    37,     3
         beq         $2,$0,$L11          	              #    37,     3
         dla         $15,lfsr_bits       	              #    39,     4
         lwu         $11,0($15)          	              #    39,     4 lfsr_bits
         dla         $3,feedbackterms    	              #    39,     5
         li          $9,4                	              #    39,     5
         addu        $10,$11,$0          	              #    39,     5
         mul         $8,$9,$10           	              #    39,     5
         addu        $2,$3,$8            	              #    39,     5
         lw          $7,0($2)            	              #    39,     5 0[ _&feedbackterms + 4*(int) @SPLTMR_lfsr_bits_P2].B
         dla         $4,lfsr_state       	              #    39,     7
         srl         $5,$13,1            	              #    39,     6
         and         $6,$7,4294967295    	              #    39,     6
         xor         $14,$5,$6           	              #    39,     6
         sw          $14,0($4)           	              #    39,     7 lfsr_state
         beqz        $0,$L13    
$L11:
         dla         $6,lfsr_state       	              #    43,     9
         srl         $14,$13,1           	              #    43,     8
         sw          $14,0($6)           	              #    43,     9 lfsr_state
$L13:
         move        $2,$14              	              #    46,    10
         move        $sp,$fp
         ld          $fp,8($sp)
         daddu       $sp,$sp,112         	              #    46,    10
         jr          $31                 	              #    46,    10
         .end        lfsr
         # -----------------------------------------
         #     Routine gups8
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        gups8
gups8:
         .frame      $sp, 144, $31
         .mask       0x40000000,-128
         dsubu       $sp,$sp,144         
         sd          $fp,8($sp)
         move        $fp,$sp
         ctc2        $4,vbase0           
         move        $13,$5              
         ble         $13,$0,$L22         	              #    67,     3
         dla         $2,indices          	              #    69,     4
         dli         $7,2                	              #    67,    11
         addu        $4,$13,$0           	              #    67,    10
         lwu         $6,0($2)            	              #    69,     4 indices
         ctc2        $7,vpw              	              #    67,    11
         move        $5,$6               	              #    67,    13
         ctc2        $5,vbase5           
         ctc2        $4,vl               	              #    67,    11
         vsatvl                          	              #    67,    11
         cfc2        $13,vl              	              #    67,    11
         move        $14,$4              	              #    67,    12
$L20:
         ctc2        $13,vl              	              #    69,    14
         vld.u.w     $vr1,vbase5,vinc0   	              #    69,    14 0[@SI_V5:@VL_V2#I@CS_0001_C0:1#W@VW_V6].B
         vldx.u.b    $vr5,$vr1,vbase0    	              #    69,    14 0[$LCS_field_0:@VL_V2:(int) 0[@SI_V5:@VL_V2#I@CS_0001_C0:1#W@VW_V6].B#W@VW_V6].B
         li          $3,1                	              #    69,    14
         mtc2        $3,$vs7             
         vadd.sv     $vr4,$vs7,$vr5      	              #    69,    14
         dli         $2,255              	              #    69,    14
         mtc2        $2,$vs6             	              #    69,    14
         vand.sv     $vr6,$vs6,$vr4      	              #    69,    14
         vld.u.w     $vr7,vbase5,vinc0   	              #    69,    14 0[@SI_V5:@VL_V2#I@CS_0001_C0:1#W@VW_V6].B
         li          $5,4                	              #    69,    21
         mul         $4,$5,$13           	              #    69,    21
         cfc2        $7,vbase5           
         addu        $6,$7,$4            	              #    67,    15
         ctc2        $6,vbase5           
         subu        $14,$14,$13         	              #    67,    16
         vstx.b      $vr6,$vr7,vbase0    	              #    69,    14 0[{$LCS_field_0:@VL_V2:(int) 0[@SI_V5:@VL_V2#I@CS_0001_C0:1#W@VW_V6].B#W@VW_V6}].B
         ctc2        $14,vl              	              #    67,    17
         vsatvl                          	              #    67,    17
         cfc2        $13,vl              	              #    67,    17
         bgt         $14,$0,$L20         	              #    67,    18
$L22:
         move        $sp,$fp
         ld          $fp,8($sp)
         daddu       $sp,$sp,144         	              #    71,    19
         jr          $31                 	              #    71,    19
         .end        gups8
         # -----------------------------------------
         #     Routine gups16
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        gups16
gups16:
         .frame      $sp, 144, $31
         .mask       0x40000000,-128
         dsubu       $sp,$sp,144         
         sd          $fp,8($sp)
         move        $fp,$sp
         ctc2        $4,vbase0           
         move        $13,$5              
         ble         $13,$0,$L31         	              #    84,     3
         dla         $2,indices          	              #    86,     4
         dli         $7,2                	              #    84,    11
         addu        $4,$13,$0           	              #    84,    10
         lwu         $6,0($2)            	              #    86,     4 indices
         ctc2        $7,vpw              	              #    84,    11
         move        $5,$6               	              #    84,    13
         ctc2        $5,vbase5           
         ctc2        $4,vl               	              #    84,    11
         vsatvl                          	              #    84,    11
         cfc2        $13,vl              	              #    84,    11
         move        $14,$4              	              #    84,    12
$L29:
         ctc2        $13,vl              	              #    86,    14
         vld.u.w     $vr3,vbase5,vinc0   	              #    86,    14 0[@SI_V12:@VL_V9#I@CS_0001_C1:1#W@VW_V13].B
         li          $8,2                	              #    86,    14
         mtc2        $8,$vs7             
         vmullo.sv   $vr2,$vs7,$vr3      	              #    86,    14
         vldx.u.h    $vr1,$vr2,vbase0    	              #    86,    14 0[$LCS_field_1:@VL_V9:2*(int) 0[@SI_V12:@VL_V9#I@CS_0001_C1:1#W@VW_V13].B#W@VW_V13].B
         li          $3,1                	              #    86,    14
         mtc2        $3,$vs4             
         vadd.sv     $vr4,$vs4,$vr1      	              #    86,    14
         vld.u.w     $vr5,vbase5,vinc0   	              #    86,    14 0[@SI_V12:@VL_V9#I@CS_0001_C1:1#W@VW_V13].B
         dli         $2,65535            	              #    86,    14
         mtc2        $2,$vs6             	              #    86,    14
         vmullo.sv   $vr7,$vs7,$vr5      	              #    86,    14
         vand.sv     $vr6,$vs6,$vr4      	              #    86,    14
         li          $5,4                	              #    86,    21
         mul         $4,$5,$13           	              #    86,    21
         cfc2        $7,vbase5           
         addu        $6,$7,$4            	              #    84,    15
         ctc2        $6,vbase5           
         subu        $14,$14,$13         	              #    84,    16
         vstx.h      $vr6,$vr7,vbase0    	              #    86,    14 0[{$LCS_field_1:@VL_V9:2*(int) 0[@SI_V12:@VL_V9#I@CS_0001_C1:1#W@VW_V13].B#W@VW_V13}].B
         ctc2        $14,vl              	              #    84,    17
         vsatvl                          	              #    84,    17
         cfc2        $13,vl              	              #    84,    17
         bgt         $14,$0,$L29         	              #    84,    18
$L31:
         move        $sp,$fp
         ld          $fp,8($sp)
         daddu       $sp,$sp,144         	              #    88,    19
         jr          $31                 	              #    88,    19
         .end        gups16
         # -----------------------------------------
         #     Routine gups32
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        gups32
gups32:
         .frame      $sp, 144, $31
         .mask       0x40000000,-128
         dsubu       $sp,$sp,144         
         sd          $fp,8($sp)
         move        $fp,$sp
         ctc2        $4,vbase0           
         move        $13,$5              
         ble         $13,$0,$L40         	              #   101,     3
         dla         $2,indices          	              #   103,     4
         dli         $7,2                	              #   101,    11
         addu        $4,$13,$0           	              #   101,    10
         lwu         $6,0($2)            	              #   103,     4 indices
         ctc2        $7,vpw              	              #   101,    11
         move        $5,$6               	              #   101,    13
         ctc2        $5,vbase5           
         ctc2        $4,vl               	              #   101,    11
         vsatvl                          	              #   101,    11
         cfc2        $13,vl              	              #   101,    11
         move        $14,$4              	              #   101,    12
$L38:
         ctc2        $13,vl              	              #   103,    14
         vld.u.w     $vr2,vbase5,vinc0   	              #   103,    14 0[@SI_V19:@VL_V16#I@CS_0001_C2:1#W@VW_V20].B
         li          $5,4                	              #   103,    21
         mtc2        $5,$vs7             
         vmullo.sv   $vr5,$vs7,$vr2      	              #   103,    14
         vld.u.w     $vr1,vbase5,vinc0   	              #   103,    14 0[@SI_V19:@VL_V16#I@CS_0001_C2:1#W@VW_V20].B
         li          $2,1                	              #   103,    14
         mtc2        $2,$vs6             
         vmullo.sv   $vr7,$vs7,$vr1      	              #   103,    14
         vldx.u.w    $vr4,$vr5,vbase0    	              #   103,    14 0[$LCS_field_2:@VL_V16:4*(int) 0[@SI_V19:@VL_V16#I@CS_0001_C2:1#W@VW_V20].B#W@VW_V20].B
         mul         $4,$5,$13           	              #   103,    21
         vadd.u.sv   $vr6,$vs6,$vr4      	              #   103,    14
         cfc2        $7,vbase5           
         addu        $6,$7,$4            	              #   101,    15
         ctc2        $6,vbase5           
         subu        $14,$14,$13         	              #   101,    16
         vstx.w      $vr6,$vr7,vbase0    	              #   103,    14 0[{$LCS_field_2:@VL_V16:4*(int) 0[@SI_V19:@VL_V16#I@CS_0001_C2:1#W@VW_V20].B#W@VW_V20}].B
         ctc2        $14,vl              	              #   101,    17
         vsatvl                          	              #   101,    17
         cfc2        $13,vl              	              #   101,    17
         bgt         $14,$0,$L38         	              #   101,    18
$L40:
         move        $sp,$fp
         ld          $fp,8($sp)
         daddu       $sp,$sp,144         	              #   105,    19
         jr          $31                 	              #   105,    19
         .end        gups32
         # -----------------------------------------
         #     Routine gups64
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        gups64
gups64:
         .frame      $sp, 144, $31
         .mask       0x40000000,-128
         dsubu       $sp,$sp,144         
         sd          $fp,8($sp)
         move        $fp,$sp
         ctc2        $4,vbase0           
         move        $13,$5              
         ble         $13,$0,$L49         	              #   118,     3
         dla         $2,indices          	              #   120,     4
         dli         $7,3                	              #   118,    11
         addu        $4,$13,$0           	              #   118,    10
         lwu         $6,0($2)            	              #   120,     4 indices
         ctc2        $7,vpw              	              #   118,    11
         move        $5,$6               	              #   118,    13
         ctc2        $5,vbase5           
         ctc2        $4,vl               	              #   118,    11
         vsatvl                          	              #   118,    11
         cfc2        $13,vl              	              #   118,    11
         move        $14,$4              	              #   118,    12
$L47:
         ctc2        $13,vl              	              #   120,    14
         vld.u.w     $vr10,vbase5,vinc0  	              #   120,    14 0[@SI_V26:@VL_V23#I@CS_0001_C3:1#W@VW_V21].B
         dli         $3,32               	              #   120,    14
         mtc2        $3,$vs5             	              #   120,    14
         vsll.vs     $vr9,$vr10,$vs5     	              #   120,    14
         vld.u.w     $vr3,vbase5,vinc0   	              #   120,    14 0[@SI_V26:@VL_V23#I@CS_0001_C3:1#W@VW_V21].B
         vsra.vs     $vr8,$vr9,$vs5      	              #   120,    14
         li          $8,8                	              #   120,    14
         mtc2        $8,$vs7             
         vmullo.sv   $vr2,$vs7,$vr8      	              #   120,    14
         mtc2        $3,$vs4             	              #   120,    14
         vsll.vs     $vr1,$vr3,$vs4      	              #   120,    14
         vldx.l      $vr4,$vr2,vbase0    	              #   120,    14 0[$LCS_field_3:@VL_V23:8*(int) 0[@SI_V26:@VL_V23#I@CS_0001_C3:1#W@VW_V21].B#W@VW_V21].B
         vsra.vs     $vr5,$vr1,$vs4      	              #   120,    14
         dli         $2,1                	              #   120,    14
         mtc2        $2,$vs6             
         vmullo.sv   $vr7,$vs7,$vr5      	              #   120,    14
         vadd.u.sv   $vr6,$vs6,$vr4      	              #   120,    14
         li          $5,4                	              #   120,    21
         mul         $4,$5,$13           	              #   120,    21
         cfc2        $7,vbase5           
         addu        $6,$7,$4            	              #   118,    15
         vstx.l      $vr6,$vr7,vbase0    	              #   120,    14 0[{$LCS_field_3:@VL_V23:8*(int) 0[@SI_V26:@VL_V23#I@CS_0001_C3:1#W@VW_V21].B#W@VW_V21}].B
         ctc2        $6,vbase5           
         subu        $14,$14,$13         	              #   118,    16
         ctc2        $14,vl              	              #   118,    17
         vsatvl                          	              #   118,    17
         cfc2        $13,vl              	              #   118,    17
         bgt         $14,$0,$L47         	              #   118,    18
$L49:
         move        $sp,$fp
         ld          $fp,8($sp)
         daddu       $sp,$sp,144         	              #   122,    19
         jr          $31                 	              #   122,    19
         .end        gups64
         # -----------------------------------------
         #     Routine timetest
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        timetest
timetest:
         .frame      $sp, 128, $31
         .mask       0xc0010000,-96
         dsubu       $sp,$sp,128         
         sd          $31,24($sp)
         sd          $fp,16($sp)
         sd          $16,8($sp)
         move        $fp,$sp
         move        $13,$5              
         ble         $13,$0,$L59         	              #   131,     3
         addu        $4,$13,$0           	              #   131,     5
         subu        $16,$0,$4           	              #   131,     5
$L56:
         dla         $25,lfsr            	              #   133,     6
         jalr        $25                 	              #   133,     6
         addu        $16,$16,1           	              #   131,     7
         blt         $16,$0,$L56         	              #   131,     8
$L59:
         move        $sp,$fp
         ld          $31,24($sp)
         ld          $fp,16($sp)
         ld          $16,8($sp)
         daddu       $sp,$sp,128         	              #   138,     9
         jr          $31                 	              #   138,     9
         .end        timetest
         # -----------------------------------------
         #     Routine timeDiff
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        timeDiff
timeDiff:
         .frame      $sp, 160, $31
         .mask       0x40030000,-128
         dsubu       $sp,$sp,160         
         sd          $fp,24($sp)
         sd          $17,16($sp)
         sd          $16,8($sp)
         move        $fp,$sp
         move        $16,$5              
         move        $17,$6              
         move        $13,$4              
         lw          $8,0($17)           	              #   147,     3 0[$LCS_b_4].B
         lw          $7,0($16)           	              #   147,     5 0[$LCS_a_5].B
         lw          $3,4($17)           	              #   148,     9 4[$LCS_b_4].B
         lw          $2,4($16)           	              #   148,    10 4[$LCS_a_5].B
         subu        $14,$7,$8           	              #   147,     7
         sw          $14,0($13)          	              #   147,     8 0[$LCS_d_6].B
         subu        $15,$2,$3           	              #   148,    11
         sw          $15,4($13)          	              #   148,    12 4[$LCS_d_6].B
         bge         $15,$0,$L66         	              #   149,    13
         li          $6,1000000          	              #   152,    14
         addu        $5,$6,$15           	              #   152,    14
         addu        $4,$14,-1           	              #   151,    16
         sw          $5,4($13)           	              #   152,    15 4[$LCS_d_6].B
         sw          $4,0($13)           	              #   151,    17 0[$LCS_d_6].B
$L66:
         move        $sp,$fp
         ld          $fp,24($sp)
         ld          $17,16($sp)
         ld          $16,8($sp)
         daddu       $sp,$sp,160         	              #   154,    18
         jr          $31                 	              #   154,    18
         .end        timeDiff
         # -----------------------------------------
         #     Routine elapsed
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        elapsed
elapsed:
         .frame      $sp, 128, $31
         .mask       0xc0030000,-96
         dsubu       $sp,$sp,128         
         sd          $31,24($sp)
         sd          $fp,16($sp)
         sd          $17,8($sp)
         sd          $16,0($sp)
         move        $fp,$sp
         move        $17,$4              
         move        $16,$5              
         dla         $25,timeDiff        	              #   162,     2
         daddu       $4,$fp,56           	              #   162,     2
         move        $5,$16              	              #   162,     2
         move        $6,$17              	              #   162,     2
         jalr        $25                 	              #   162,     2
         daddu       $6,$fp,56           	              #   163,     8
         lw          $5,4($6)            	              #   163,     3 4[$LCS_diff_7].B
         lw          $4,0($6)            	              #   163,     4 0[$LCS_diff_7].B
         dmtc1       $5,$f3              	              #   163,     5
         cvt.d.w     $f7,$f3             	              #   163,     5
         li.d        $f2,1.0000000000000000e+06 	              #   163,     5
         div.d       $f5,$f7,$f2         	              #   163,     5
         dmtc1       $4,$f6              	              #   163,     5
         cvt.d.w     $f4,$f6             	              #   163,     5
         add.d       $f0,$f4,$f5         	              #   163,     5
         move        $sp,$fp
         ld          $31,24($sp)
         ld          $fp,16($sp)
         ld          $17,8($sp)
         ld          $16,0($sp)
         daddu       $sp,$sp,128         	              #   164,     6
         jr          $31                 	              #   164,     6
         .end        elapsed
         # -----------------------------------------
         #     Routine calc_indices
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        calc_indices
calc_indices:
         .frame      $sp, 176, $31
         .mask       0xc00f0000,-128
         dsubu       $sp,$sp,176         
         sd          $31,40($sp)
         sd          $fp,32($sp)
         sd          $19,24($sp)
         sd          $18,16($sp)
         sd          $17,8($sp)
         sd          $16,0($sp)
         move        $fp,$sp
         move        $17,$4              
         move        $16,$5              
         dla         $25,malloc          	              #   171,     2
         dli         $7,4                	              #   171,     2
         dmul        $6,$7,$17           	              #   171,     2
         addu        $5,$6,$0            	              #   171,     2
         and         $4,$5,4294967295    	              #   171,     2
         jalr        $25                 	              #   171,     2
         dla         $3,indices          	              #   171,     2
         sw          $2,0($3)            	              #   171,     2 indices
         dla         $7,indices          	              #   172,     3
         lwu         $6,0($7)            	              #   172,     3 indices
         subu        $5,$6,$0            	              #   172,     4
         sltu        $4,$0,$5            	              #   172,     4
         bnez        $4,$L84             	              #   172,     4
         dla         $4,__gstatic        	              #   174,     5
         dla         $25,printf          	              #   174,     5
         jalr        $25                 	              #   174,     5
         dla         $5,indices          	              #   175,     6
         lwu         $4,0($5)            	              #   175,     6 indices
         subu        $6,$4,$0            	              #   175,     7
         sltu        $7,$0,$6            	              #   175,     7
         bnez        $7,$L84             	              #   175,     7
         dla         $9,__gstatic        	              #   175,     8
         dla         $25,__assert        	              #   175,     8
         li          $6,175              	              #   175,     8
         daddu       $4,$9,56            	              #   175,     8
         daddu       $5,$9,64            	              #   175,     8
         jalr        $25                 	              #   175,     8
$L84:
         ble         $17,$0,$L90         	              #   177,    10
         move        $14,$0              	              #   179,    14
         daddu       $9,$17,-1           	              #   177,    15
         addu        $8,$9,$0            	              #   177,    15
         addu        $13,$8,1            	              #   177,    15
         subu        $15,$0,$13          	              #   177,    16
         move        $18,$15             
         move        $17,$14             
$L87:
         dla         $25,lfsr            	              #   179,    17
         jalr        $25                 	              #   179,    17
         move        $19,$2              	              #   179,    17
         ddivu       $0,$19,$16
         teq         $16,$0,7
         mfhi        $11                 	              #   179,    19
         dla         $2,indices          	              #   179,    18
         lwu         $10,0($2)           	              #   179,    18 indices
         addu        $9,$11,$0           	              #   179,    19
         addu        $8,$17,$10          	              #   179,    20
         and         $13,$9,4294967295   	              #   179,    19
         sw          $13,0($8)           	              #   179,    20 0[@SR_E0 + (int) @SPLTMR_indices_P28].B
         addu        $14,$17,4           	              #   179,    21
         addu        $15,$18,1           	              #   177,    22
         move        $18,$15             
         move        $17,$14             
         blt         $15,$0,$L87         	              #   177,    23
$L90:
         move        $sp,$fp
         ld          $31,40($sp)
         ld          $fp,32($sp)
         ld          $19,24($sp)
         ld          $18,16($sp)
         ld          $17,8($sp)
         ld          $16,0($sp)
         daddu       $sp,$sp,176         	              #   181,    24
         jr          $31                 	              #   181,    24
         .end        calc_indices
         # -----------------------------------------
         #     Routine main
         # -----------------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
         .ent        main
main:
         .frame      $sp, 352, $31
         .mask       0xc0ff0000,-256
         .fmask      0x3000000,-336
         dsubu       $sp,$sp,352         
         sd          $31,88($sp)
         sd          $fp,80($sp)
         sd          $23,72($sp)
         sd          $22,64($sp)
         sd          $21,56($sp)
         sd          $20,48($sp)
         sd          $19,40($sp)
         sd          $18,32($sp)
         sd          $17,24($sp)
         sd          $16,16($sp)
         s.d         $f25,8($sp)
         s.d         $f24,0($sp)
         move        $fp,$sp
         dla         $3,__gstatic        	              #   219,     2
         dla         $25,scanf           	              #   219,     2
         daddu       $4,$3,336           	              #   219,     2
         daddu       $5,$fp,280          	              #   219,     2
         daddu       $6,$fp,276          	              #   219,     2
         daddu       $7,$fp,264          	              #   219,     2
         jalr        $25                 	              #   219,     2
         ld          $18,264($fp)        	              #   220,     3 width
         daddu       $10,$18,-16         	              #   220,     4
         li          $6,1                	              #   220,     4
         daddu       $9,$18,-8           	              #   220,     4
         sltiu       $7,$10,1            	              #   220,     4
         sltiu       $3,$9,1             	              #   220,     4
         daddu       $8,$18,-32          	              #   220,     4
         sltiu       $5,$8,1             	              #   220,     4
         movn        $7,$6,$3            	              #   220,     4
         daddu       $2,$18,-64          	              #   220,     4
         sltiu       $4,$2,1             	              #   220,     4
         movn        $5,$6,$7            	              #   220,     4
         movn        $4,$6,$5            	              #   220,     4
         bne         $4,$0,$L99          	              #   220,     4
         dla         $7,__gstatic        	              #   220,     5
         dla         $25,__assert        	              #   220,     5
         li          $6,220              	              #   220,     5
         daddu       $4,$7,56            	              #   220,     5
         daddu       $5,$7,64            	              #   220,     5
         jalr        $25                 	              #   220,     5
$L99:
         l.s         $f24,276($fp)       	              #   221,     6 expt
         cvt.d.s     $f7,$f24            	              #   221,     7
         li.d        $f6,3.0000000000000000e+00 	              #   221,     7
         c.olt.d     $fcc7,$f6,$f7       	              #   221,     7
         bc1t        $fcc7,$L102         	              #   221,     7
         dla         $9,__gstatic        	              #   221,     8
         dla         $25,__assert        	              #   221,     8
         li          $6,221              	              #   221,     8
         daddu       $4,$9,56            	              #   221,     8
         daddu       $5,$9,64            	              #   221,     8
         jalr        $25                 	              #   221,     8
$L102:
         ld          $17,280($fp)        	              #   222,     9 updates
         bgt         $17,$0,$L105        	              #   222,    10
         dla         $13,__gstatic       	              #   222,    11
         dla         $25,__assert        	              #   222,    11
         li          $6,222              	              #   222,    11
         daddu       $4,$13,56           	              #   222,    11
         daddu       $5,$13,64           	              #   222,    11
         jalr        $25                 	              #   222,    11
$L105:
         dla         $25,exp             	              #   223,    12
         cvt.d.s     $f15,$f24           	              #   223,    12
         li.d        $f14,6.9314718055994529e-01 	              #   223,    12
         mul.d       $f12,$f14,$f15      	              #   223,    12
         jalr        $25                 	              #   223,    12
         trunc.l.d   $f13,$f0            	              #   223,    13
         dmfc1       $16,$f13            
         dla         $14,__gstatic       	              #   225,    14
         ld          $5,280($fp)         	              #   225,    14 updates
         dla         $25,printf          	              #   225,    14
         daddu       $4,$14,352          	              #   225,    14
         jalr        $25                 	              #   225,    14
         dla         $13,__gstatic       	              #   226,    15
         ld          $5,264($fp)         	              #   226,    15 width
         dla         $25,printf          	              #   226,    15
         daddu       $4,$13,368          	              #   226,    15
         jalr        $25                 	              #   226,    15
         l.s         $f15,276($fp)       	              #   227,    16 expt
         dla         $15,__gstatic       	              #   227,    16
         dla         $25,printf          	              #   227,    16
         cvt.d.s     $f14,$f15           	              #   227,    16
         dmfc1       $5,$f14             
         daddu       $4,$15,392          	              #   227,    16
         move        $6,$16              	              #   227,    16
         jalr        $25                 	              #   227,    16
         dla         $25,malloc          	              #   229,    17
         addu        $14,$16,$0          	              #   229,    17
         and         $4,$14,4294967295   	              #   229,    17
         jalr        $25                 	              #   229,    17
         move        $19,$2              	              #   229,    17
         subu        $14,$19,$0          	              #   230,    18
         sltu        $15,$0,$14          	              #   230,    18
         bnez        $15,$L110           	              #   230,    18
         dla         $21,__gstatic       	              #   232,    19
         dla         $25,printf          	              #   232,    19
         daddu       $4,$21,424          	              #   232,    19
         move        $5,$16              	              #   232,    19
         jalr        $25                 	              #   232,    19
         dla         $20,__gstatic       	              #   233,    20
         dla         $25,__assert        	              #   233,    20
         li          $6,233              	              #   233,    20
         daddu       $4,$20,56           	              #   233,    20
         daddu       $5,$20,64           	              #   233,    20
         jalr        $25                 	              #   233,    20
$L110:
         l.s         $f18,276($fp)       	              #   237,    21 expt
         dla         $25,ceil            	              #   237,    21
         cvt.d.s     $f12,$f18           	              #   237,    21
         jalr        $25                 	              #   237,    21
         mov.d       $f24,$f0            	              #   237,    21
         dla         $21,lfsr_bits       	              #   237,    23
         trunc.w.d   $f19,$f24           	              #   237,    22
         dmfc1       $22,$f19            
         and         $20,$22,4294967295  	              #   237,    22
         sw          $20,0($21)          	              #   237,    23 lfsr_bits
         dla         $21,lfsr_bits       	              #   238,    24
         dla         $20,__gstatic       	              #   238,    24
         lwu         $5,0($21)           	              #   238,    24 lfsr_bits
         dla         $25,printf          	              #   238,    24
         daddu       $4,$20,464          	              #   238,    24
         jalr        $25                 	              #   238,    24
         daddu       $20,$18,-64         	              #   241,    25
         sltiu       $21,$20,1           	              #   241,    25
         bnez        $21,$L126           	              #   241,    25
         daddu       $22,$18,-32         	              #   241,    26
         sltiu       $23,$22,1           	              #   241,    26
         bnez        $23,$L124           	              #   241,    26
         daddu       $24,$18,-16         	              #   241,    27
         sltiu       $25,$24,1           	              #   241,    27
         bnez        $25,$L122           	              #   241,    27
         daddu       $25,$18,-8          	              #   241,    28
         sltu        $24,$0,$25          	              #   241,    28
         bnez        $24,$L128           	              #   241,    28
         sw          $19,252($fp)        	              #   244,    30 field8
         dli         $24,1               	              #   246,    31
         sd          $24,256($fp)        	              #   246,    31 elt_size
         beqz        $0,$L128   
$L122:
         sw          $19,248($fp)        	              #   249,    33 field16
         dli         $24,2               	              #   251,    34
         sd          $24,256($fp)        	              #   251,    34 elt_size
         beqz        $0,$L128   
$L124:
         sw          $19,244($fp)        	              #   254,    36 field32
         dli         $2,4                	              #   256,    37
         sd          $2,256($fp)         	              #   256,    37 elt_size
         beqz        $0,$L128   
$L126:
         sw          $19,240($fp)        	              #   259,    39 field64
         dli         $3,8                	              #   261,    40
         sd          $3,256($fp)         	              #   261,    40 elt_size
$L128:
         dla         $6,__gstatic        	              #   265,    41
         ld          $5,256($fp)         	              #   265,    41 elt_size
         dla         $25,printf          	              #   265,    41
         daddu       $4,$6,496           	              #   265,    41
         jalr        $25                 	              #   265,    41
         ld          $5,256($fp)         	              #   266,    42 elt_size
         ddivu       $16,$16,$5          	              #   266,    43
         dla         $3,__gstatic        	              #   267,    44
         dla         $25,printf          	              #   267,    44
         and         $6,$19,4294967295   	              #   267,    44
         daddu       $4,$3,528           	              #   267,    44
         move        $5,$16              	              #   267,    44
         jalr        $25                 	              #   267,    44
         dla         $2,__gstatic        	              #   271,    45
         dla         $25,printf          	              #   271,    45
         daddu       $4,$2,584           	              #   271,    45
         jalr        $25                 	              #   271,    45
         ld          $4,280($fp)         	              #   272,    46 updates
         dla         $25,calc_indices    	              #   272,    46
         move        $5,$16              	              #   272,    46
         jalr        $25                 	              #   272,    46
         dla         $7,__gstatic        	              #   281,    47
         dla         $25,printf          	              #   281,    47
         daddu       $4,$7,608           	              #   281,    47
         jalr        $25                 	              #   281,    47
         daddu       $6,$18,-64          	              #   282,    48
         sltiu       $7,$6,1             	              #   282,    48
         bnez        $7,$L144            	              #   282,    48
         daddu       $8,$18,-32          	              #   282,    49
         sltiu       $9,$8,1             	              #   282,    49
         bnez        $9,$L142            	              #   282,    49
         daddu       $10,$18,-16         	              #   282,    50
         sltiu       $11,$10,1           	              #   282,    50
         bnez        $11,$L140           	              #   282,    50
         daddu       $14,$18,-8          	              #   282,    51
         sltu        $13,$0,$14          	              #   282,    51
         bnez        $13,$L146           	              #   282,    51
         dla         $25,fake_gettimeofday 	              #   285,    52
         daddu       $4,$fp,232          	              #   285,    52
         move        $5,$0               	              #   285,    52
         jalr        $25                 	              #   285,    52
         ld          $13,280($fp)        	              #   286,    53 updates
         lwu         $4,252($fp)         	              #   286,    53 field8
         dla         $25,gups8           	              #   286,    53
         addu        $14,$13,$0          	              #   286,    53
         addu        $15,$16,$0          	              #   286,    53
         and         $5,$14,4294967295   	              #   286,    53
         and         $6,$15,4294967295   	              #   286,    53
         jalr        $25                 	              #   286,    53
         dla         $25,fake_gettimeofday 	              #   287,    54
         daddu       $4,$fp,224          	              #   287,    54
         move        $5,$0               	              #   287,    54
         jalr        $25                 	              #   287,    54
         beqz        $0,$L146   
$L140:
         dla         $25,fake_gettimeofday 	              #   290,    55
         daddu       $4,$fp,232          	              #   290,    55
         move        $5,$0               	              #   290,    55
         jalr        $25                 	              #   290,    55
         ld          $20,280($fp)        	              #   291,    56 updates
         lwu         $4,248($fp)         	              #   291,    56 field16
         dla         $25,gups16          	              #   291,    56
         addu        $19,$20,$0          	              #   291,    56
         addu        $18,$16,$0          	              #   291,    56
         and         $5,$19,4294967295   	              #   291,    56
         and         $6,$18,4294967295   	              #   291,    56
         jalr        $25                 	              #   291,    56
         dla         $25,fake_gettimeofday 	              #   292,    57
         daddu       $4,$fp,224          	              #   292,    57
         move        $5,$0               	              #   292,    57
         jalr        $25                 	              #   292,    57
         beqz        $0,$L146   
$L142:
         dla         $25,fake_gettimeofday 	              #   295,    58
         daddu       $4,$fp,232          	              #   295,    58
         move        $5,$0               	              #   295,    58
         jalr        $25                 	              #   295,    58
         ld          $20,280($fp)        	              #   296,    59 updates
         lwu         $4,244($fp)         	              #   296,    59 field32
         dla         $25,gups32          	              #   296,    59
         addu        $18,$20,$0          	              #   296,    59
         addu        $19,$16,$0          	              #   296,    59
         and         $5,$18,4294967295   	              #   296,    59
         and         $6,$19,4294967295   	              #   296,    59
         jalr        $25                 	              #   296,    59
         dla         $25,fake_gettimeofday 	              #   297,    60
         daddu       $4,$fp,224          	              #   297,    60
         move        $5,$0               	              #   297,    60
         jalr        $25                 	              #   297,    60
         beqz        $0,$L146   
$L144:
         dla         $25,fake_gettimeofday 	              #   300,    61
         daddu       $4,$fp,232          	              #   300,    61
         move        $5,$0               	              #   300,    61
         jalr        $25                 	              #   300,    61
         ld          $22,280($fp)        	              #   301,    62 updates
         lwu         $4,240($fp)         	              #   301,    62 field64
         dla         $25,gups64          	              #   301,    62
         addu        $20,$22,$0          	              #   301,    62
         addu        $21,$16,$0          	              #   301,    62
         and         $5,$20,4294967295   	              #   301,    62
         and         $6,$21,4294967295   	              #   301,    62
         jalr        $25                 	              #   301,    62
         dla         $25,fake_gettimeofday 	              #   302,    63
         daddu       $4,$fp,224          	              #   302,    63
         move        $5,$0               	              #   302,    63
         jalr        $25                 	              #   302,    63
$L146:
         dla         $25,elapsed         	              #   306,    64
         daddu       $4,$fp,232          	              #   306,    64
         daddu       $5,$fp,224          	              #   306,    64
         jalr        $25                 	              #   306,    64
         mov.d       $f24,$f0            	              #   306,    64
         dla         $22,__gstatic       	              #   308,    65
         dla         $25,printf          	              #   308,    65
         dmfc1       $5,$f0              
         daddu       $4,$22,624          	              #   308,    65
         jalr        $25                 	              #   308,    65
         dsll        $16,$17,1           	              #   309,    66
         slt         $22,$17,$0          	              #   309,    66
         dsrl        $21,$16,1           	              #   309,    66
         dmtc1       $21,$f17            	              #   309,    66
         dli         $20,4890909195324358656 	              #   309,    66
         cvt.d.l     $f20,$f17           	              #   309,    66
         li.d        $f16,1.0000000000000000e+09 	              #   309,    66
         move        $23,$0              
         movn        $23,$20,$22         	              #   309,    66
         mul.d       $f22,$f16,$f24      	              #   309,    66
         dmtc1       $23,$f21            	              #   309,    66
         add.d       $f23,$f20,$f21      	              #   309,    66
         div.d       $f25,$f23,$f22      	              #   309,    66
         dla         $24,__gstatic       	              #   310,    67
         dla         $25,printf          	              #   310,    67
         daddu       $4,$24,656          	              #   310,    67
         dmfc1       $5,$f25             	              #   310,    67
         jalr        $25                 	              #   310,    67
         move        $2,$0               	              #   313,    68
         move        $sp,$fp
         ld          $31,88($sp)
         ld          $fp,80($sp)
         ld          $23,72($sp)
         ld          $22,64($sp)
         ld          $21,56($sp)
         ld          $20,48($sp)
         ld          $19,40($sp)
         ld          $18,32($sp)
         ld          $17,24($sp)
         ld          $16,16($sp)
         l.d         $f25,8($sp)
         l.d         $f24,0($sp)
         daddu       $sp,$sp,352         	              #   313,    68
         jr          $31                 	              #   313,    68
         .end        main
         # -----------------------------------------
         .globl      fake_gettimeofday
         .globl      lfsr
         .globl      gups8
         .globl      gups16
         .globl      gups32
         .globl      gups64
         .globl      timetest
         .globl      timeDiff
         .globl      elapsed
         .globl      calc_indices
         .globl      main
         .extern     printf
         .extern     scanf
         .extern     malloc
         .extern     __assert
         .extern     time
         .extern     ceil
         .extern     exp
         # end of compilation unit
