         # ----------------------------------
         #     Routine gups64
         # ----------------------------------
         .section    .text , 1, 0x06, 4, 16
         # loop_depth 0
	 .globl      gups64
         .ent        gups64
gups64:                                   # gups64(field, iters, size)
         .frame      $sp, 144, $31        # {
         .mask       0x40000000,-128
         dsubu       $sp,$sp,144          # set up stack            
         sd          $fp,8($sp)           # save frame pointer
         move        $fp,$sp              # set up new frame pointer
         ctc2        $4,vbase0            # vbase0 points to beginning of field         
         move        $13,$5               # $13 <- iters         
         ble         $13,$0,$L49          # if no iters, then skip to end      
         dla         $2,indices           # $2 <- pointer to pointer to indices     
         dli         $7,3                 # $7 <- 3
         addu        $4,$13,$0            # $4 <- iters
         lwu         $6,0($2)             # $6 <- pointer to indices 
         ctc2        $7,vpw               # vpw <- 64 (3 is code for 64) 
         move        $5,$6                # $5 <- pointer to indices
         ctc2        $5,vbase5            # vbase5 points to beginning of indices 
         ctc2        $4,vl                # set vector length to iters  
         vsatvl                           # saturate vector length   
         cfc2        $13,vl               # $13 <- real vector length (32)  
         move        $14,$4               # $14 <- iters    
$L47:                                     #   BEGINNING OF LOOP:
         ctc2        $13,vl               # vl <- real vector length (32)
         vld.u.w     $vr10,vbase5,vinc0   # Load vr10 from indices (vbase5).
         dli         $3,32                # $3 <- 32
         mtc2        $3,$vs5              # vs5 <- 32
         vsll.vs     $vr9,$vr10,$vs5      # vr9 = vr10 << 32
         vld.u.w     $vr3,vbase5,vinc0    # Load vr3 from indices (vbase5).
         vsra.vs     $vr8,$vr9,$vs5       # vr8 = (vr10 << 32) >> 32
         li          $8,8                 # $8 <- 8
         mtc2        $8,$vs7              # vs7 <- 8
         vmullo.sv   $vr2,$vs7,$vr8       # vr2 <- 8 * vr8 (all indices mult. by 8)
         mtc2        $3,$vs4              # vs4 <- 32
         vsll.vs     $vr1,$vr3,$vs4       # vr1 = vr3 << 32
         vldx.l      $vr4,$vr2,vbase0     # Load vr4 using indices from field.
         vsra.vs     $vr5,$vr1,$vs4       # vr5 = (vr3 << 32) >> 32
         dli         $2,1                 # $2 <- 1
         mtc2        $2,$vs6              # vs6 <- 1
         vmullo.sv   $vr7,$vs7,$vr5       # vr7 = ((vr3 << 32) >> 32) * 8
         vadd.u.sv   $vr6,$vs6,$vr4       # vr6 = vr4 + 1
         li          $5,4                 # $5 <- 4
         mul         $4,$5,$13            # $4 <- 4 * real vector length
         cfc2        $7,vbase5            # $7 <- pointer to beginning of indices
         addu        $6,$7,$4             # $6 <- indices + 4*real vector length
         vstx.l      $vr6,$vr7,vbase0     # Store vr6 using indices to field.
         ctc2        $6,vbase5            # vbase5 += 4*real vector length
         subu        $14,$14,$13          # $14 (iters) -= real vector length
         ctc2        $14,vl               # vl <- remaining iters
         vsatvl                           # saturate vector length                      
         cfc2        $13,vl               # $13 <- real vector length (32)        
         bgt         $14,$0,$L47          # repeat loop if more iters to do.           
$L49:                                     #   END OF LOOP.
         move        $sp,$fp              # restore stack pointer.
         ld          $fp,8($sp)           # restore frame pointer.
         daddu       $sp,$sp,144          # deallocate stack frame.           
         jr          $31                  # return   
         .end        gups64               # }
