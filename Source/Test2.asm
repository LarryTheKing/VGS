		.text
main:   
        lw      $t0,a          # get a
        lw      $t1,bb         # get b
        mult    $t0,$t1		   # a*b
		mflo	$t0
        addi    $sp,$sp,-4      # push a*b onto stack
        sw      $t0,($sp)
        
        lw      $t0,a          # get a
        li      $t1,-12        # 
        mult    $t0,$t1        # -12a
		mflo	$t0
        addi    $sp,$sp,-4      # push a*b onto stack
        sw      $t0,($sp)
        
        lw      $t0,bb         # get b
        li      $t1,18         # 
        mult    $t0,$t1		   # 18b
		mflo	$t0
        addi    $sp,$sp,-4     # push a*b onto stack
        sw      $t0,($sp)

        li      $t1,-7         # init sum to -7
        lw      $t0,($sp)      # pop 18b
        addiu   $sp,$sp,4
        addu    $t1,$t1,$t0    # 18b -7
                
        lw      $t0,($sp)      # pop -12a
        addiu   $sp,$sp,4
        addu    $t1,$t1,$t0    # -12a + 18b -7
                
        lw      $t0,($sp)      # pop ab
        addi    $sp,$sp,4
        addi    $t1,$t1,$t0    # ab - 12a + 18b -7
         
done:   li      $v0,1          # print sum
        mov     $a0,$t1
        syscall
        li      $v0,10         # exit
        syscall   

        .data
a:      .word  0
bb:     .word  10