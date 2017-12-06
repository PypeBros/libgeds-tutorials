/*---------------------------------------------------------------------------------
  $Id: exceptionHandler.s,v 1.1.2.1 2008-07-03 19:08:13 pype Exp $

  Copyright (C) 2005
  	Dave Murphy (WinterMute)

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any
  damages arising from the use of this software.

  Permission is granted to anyone to use this software for any
  purpose, including commercial applications, and to alter it and
  redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you
     must not claim that you wrote the original software. If you use
     this software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and
     must not be misrepresented as being the original software.
  3. This notice may not be removed or altered from any source
     distribution.

  $Log: not supported by cvs2svn $
  Revision 1.3  2006/08/03 09:35:36  wntrmute
  fix storing pc

  Revision 1.2  2006/07/06 02:14:33  wntrmute
  read r15 in enterException
  add return to bios

  Revision 1.1  2006/06/18 21:16:26  wntrmute
  added arm9 exception handler API


---------------------------------------------------------------------------------*/
	.text

	.arm

@---------------------------------------------------------------------------------
	.global getCPSR
@---------------------------------------------------------------------------------
getCPSR:
@---------------------------------------------------------------------------------
	mrs	r0,cpsr
	bx	r14

@---------------------------------------------------------------------------------
	.global stubException
@---------------------------------------------------------------------------------
stubException:
@---------------------------------------------------------------------------------
	// store context
	ldr	r12,=exceptionRegisters
	stmia	r12,{r0-r11}
	str	r13,[r12,#oldStack - exceptionRegisters]
	// assign a stack
	ldr	r13,=exceptionStack
	ldr	r13,[r13]

	// renable MPU
	mrc	p15,0,r0,c1,c0,0
	orr	r0,r0,#1
	mcr	p15,0,r0,c1,c0,0

	// bios exception stack
	ldr 	r0, =0x027FFD90

	// grab r15 from bios exception stack
	ldr	r2,[r0,#8]
	str	r2,[r12,#reg15 - exceptionRegisters]

	// grab stored r12 and SPSR from bios exception stack
	ldmia	r0,{r2,r12}


	// grab banked registers from correct processor mode
	mrs	r3,cpsr
	bic	r4,r3,#0x1F
	and	r2,r2,#0x1F
	orr	r4,r4,r2
	msr	cpsr,r4
	ldr	r0,=reg12
	stmia	r0,{r12-r14}
	msr	cpsr,r3

	// Get C function & call it
	ldr	r12,=exceptionC
	ldr	r12,[r12,#0]
	cmp	r12,#0
	beq	no_handler
	bx	r12
no_handler:	
	// restore registers
	ldr	r12,=exceptionRegisters
	ldmia	r12,{r0-r11}
	ldr	r13,[r12,#oldStack - exceptionRegisters]

	// return through bios
	mov	pc,lr

@---------------------------------------------------------------------------------
	.global exceptionC
@---------------------------------------------------------------------------------
exceptionC:
@---------------------------------------------------------------------------------
	.word	0x00000000
@---------------------------------------------------------------------------------
	.global exceptionStack
@---------------------------------------------------------------------------------
exceptionStack:
@---------------------------------------------------------------------------------
	.word	0x00000000
@---------------------------------------------------------------------------------
	.global exceptionRegisters
@---------------------------------------------------------------------------------
exceptionRegisters:
@---------------------------------------------------------------------------------
	.space	12 * 4
reg12:		.word	0
reg13:		.word	0
reg14:		.word	0
reg15:		.word	0
oldStack:	.word	0
