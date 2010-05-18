         # ----------------------------------
         #     Routine gups32
         # ----------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
	 .globl      gups32
         .ent        gups32
gups32:                                   # gups32(field, iters, size)
         .frame      $sp, 144, $31        # {
         .mask       0x40000000,-128
         dsubu       $sp,$sp,144          # set up stack      
         sd          $fp,8($sp)           # save frame pointer
         move        $fp,$sp              # set up new frame pointer
         ctc2        $4,vbase0            # vbase0 points to beginning of field        
         move        $13,$5               # $13 <- iters 
         ble         $13,$0,$L40          # if no iters, then skip to end 
         dla         $2,indices           # $2 <- pointer to pointer to indices
         dli         $7,2                 # $7 <- 2
         addu        $4,$13,$0            # $4 <- iters     
         lwu         $6,0($2)             # $6 <- pointer to indices    
         ctc2        $7,vpw               # vpw <- 32 (2 is code for 32)   
         move        $5,$6                # $5 <- pointer to indices
         ctc2        $5,vbase5            # vbase5 points to beginning of indices 
         ctc2        $4,vl                # set vector length to iters 
         vsatvl                           # saturate vector length 
         cfc2        $13,vl               # $13 <- real vector length (64)    
         move        $14,$4               # $14 <- iters
$L38:                                     #   BEGINNING OF LOOP:
         ctc2        $13,vl               # vl <- real vector length (64)        
         vld.u.w     $vr2,vbase5,vinc0    # Load vr2 from indices (vbase5).
         li          $5,4                 # $5 <- 4
         mtc2        $5,$vs7              # vs7 <- 4
         vmullo.sv   $vr5,$vs7,$vr2       # vr5 <- 4 * vr2 (all indices mult. by 4)
         vld.u.w     $vr1,vbase5,vinc0    # Load vr1 from indices (vbase5).
         li          $2,1                 # $2 <- 1
         mtc2        $2,$vs6              # vs6 <- 1
         vmullo.sv   $vr7,$vs7,$vr1       # vr7 <- 4 * vr1 (all indices mult. by 4)
         vldx.u.w    $vr4,$vr5,vbase0     # Load vr4 using indices from field.
         mul         $4,$5,$13            # $4 <- 4 * real vector length
         vadd.u.sv   $vr6,$vs6,$vr4       # vr6 <- vr4 + 1
         cfc2        $7,vbase5            # $7 <- ptr to beginning of indices
         addu        $6,$7,$4             # $6 <- indices + 4*real vector length
         ctc2        $6,vbase5            # vbase5 += 4*real vector length
         subu        $14,$14,$13          # $14 (iters) -= real vector length
         vstx.w      $vr6,$vr7,vbase0     # Store vr6 using indices to field.
         ctc2        $14,vl               # vl <- remaining iters   
         vsatvl                           # saturate vector length
         cfc2        $13,vl               # $13 <- real vector length (64)
         bgt         $14,$0,$L38          # repeat loop if more iters to do.             
$L40:                                     #   END OF LOOP.
         move        $sp,$fp              # restore stack pointer.
         ld          $fp,8($sp)           # restore frame pointer.
         daddu       $sp,$sp,144          # deallocate stack frame.       
         jr          $31                  # return           
         .end        gups32               # }
