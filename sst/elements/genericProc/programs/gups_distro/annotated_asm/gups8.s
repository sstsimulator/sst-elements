         # ----------------------------------
         #     Routine gups8
         # ----------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
	 .globl      gups8
         .ent        gups8
gups8:                                   # gups8(field, iters, size)
         .frame      $sp, 144, $31       # {
         .mask       0x40000000,-128
         dsubu       $sp,$sp,144         # set up stack
         sd          $fp,8($sp)          # save frame pointer
         move        $fp,$sp             # set up new frame pointer
         ctc2        $4,vbase0           # vbase0 points to beginning of field
         move        $13,$5              # $13 <- iters
         ble         $13,$0,$L22         # if no iters, then skip to end
         dla         $2,indices          # $2 <- pointer to pointer to indices
         dli         $7,2                # $7 <- 2
         addu        $4,$13,$0           # $4 <- iters
         lwu         $6,0($2)            # $6 <- pointer to indices
         ctc2        $7,vpw              # vpw <- 32 (2 is code for 32)
         move        $5,$6               # $5 <- pointer to indices
         ctc2        $5,vbase5           # vbase5 points to beginning of indices
         ctc2        $4,vl               # set vector length to iters
         vsatvl                          # saturate vector length
         cfc2        $13,vl              # $13 <- real vector length (64)
         move        $14,$4              # $14 <- iters
$L20:                                    #   BEGINNING OF LOOP:
         ctc2        $13,vl              # vl <- real vector length (64)
         vld.u.w     $vr1,vbase5,vinc0   # Load vr1 from indices (vbase5).
         vldx.u.b    $vr5,$vr1,vbase0    # Load vr5 using indices from field.
         li          $3,1                # $3 <- 1
         mtc2        $3,$vs7             # vs7 <- 1
         vadd.sv     $vr4,$vs7,$vr5      # vr4 <- vr5 + 1
         dli         $2,255              # $2 <- 0xff
         mtc2        $2,$vs6             # vs6 <- 0xff
         vand.sv     $vr6,$vs6,$vr4      # vr6 <- (vr5 + 1) & 0xff
         vld.u.w     $vr7,vbase5,vinc0   # Load vr7 from indices.
         li          $5,4                # $5 <- 4
         mul         $4,$5,$13           # $4 <- 4 * real vector length
         cfc2        $7,vbase5           # $7 <- ptr to beginning of indices
         addu        $6,$7,$4            # $6 <- indices + 4*real vector length
         ctc2        $6,vbase5           # vbase5 += 4*real vector length
         subu        $14,$14,$13         # $14 (iters) -= real vector length
         vstx.b      $vr6,$vr7,vbase0    # Store vr6 to indices.
         ctc2        $14,vl              # vl <- remaining iters
         vsatvl                          # saturate vector length
         cfc2        $13,vl              # $13 <- real vector length (64)
         bgt         $14,$0,$L20         # repeat loop if more iters to do.
$L22:                                    #   END OF LOOP.
         move        $sp,$fp             # restore stack pointer.
         ld          $fp,8($sp)          # restore frame pointer.
         daddu       $sp,$sp,144         # deallocate stack frame.
         jr          $31                 # return
         .end        gups8               # }
