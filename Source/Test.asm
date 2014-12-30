# Reads in three integers and prints their sum

		.text
main:
		jal		pread			# read first integer
		nop						#
		mov		$s0,$v0			# save it in $s0

		jal		pread			# read second integer
		nop						#
		mov		$s1,$v0			# save it in $s1

		jal		pread			# read third integer
		nop						#
		mov		$s2,$v0			# save it in $s2

		la		$a0,s_sum		# print final label
		li		$v0,4			# service 4

		syscall
		
		addu	$s0,$s0,$s1		# compute the sum
		addu	$a0,$s0,$s2		#

		li		$v0,1			# print the sum
		syscall					#

		la		$a0,s_quit		# print quit instructions
		li		$v0,4			# service 4
		syscall					#
         
		li		$v0,5			# wait for input
		syscall					#

		li		$v0,10			# exit
		syscall

# pread -- prompt for and read an integer
#
# on entry:
#	$ra -- return address
#
# on exit:
#	$v0 -- the integer

		.text
pread:   
		la		$a0,prompt		# print string
		li		$v0,4			# service 4
		syscall					#
        
		li		$v0,5		# read int into $v0
		syscall					# service 5
        
		jr		$ra				# return
		nop                     #  
        
		.data
prompt:	.asciiz	"Enter an integer: "
s_sum:	.asciiz	"Sum\t\t= "
s_quit:	.asciiz	"\n\nEnter any number to quit: "