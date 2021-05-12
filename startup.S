.section ".text.boot"

#define _STACK      0x8000
.extern _regs
.extern main
.extern dispatch

_start:
    ldr pc, _reset_h
    ldr pc, _undefined_instruction_vector_h
    ldr pc, _software_interrupt_vector_h
    ldr pc, _prefetch_abort_vector_h
    ldr pc, _data_abort_vector_h
    ldr pc, _unused_handler_h
    ldr pc, _interrupt_vector_h
    ldr pc, _fast_interrupt_vector_h

_reset_h:                           .word   _reset_
_undefined_instruction_vector_h:    .word   _reset_
_software_interrupt_vector_h:       .word   _reset_
_prefetch_abort_vector_h:           .word   _reset_
_data_abort_vector_h:               .word   _reset_
_unused_handler_h:                  .word   _reset_
_interrupt_vector_h:                .word   interrupt_vector
_fast_interrupt_vector_h:           .word   _reset_

_reset_:

    mov     r0, #0x8000 // from (_start)
    mov     r1, #0x0000 // to
            // load 8x 4Byte into r2..r9 from addr in r0 ...
    ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
            // and copy to ..
    stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}
            // do it again with the next bytes (.._h words/const)
    ldmia   r0!,{r2, r3, r4, r5, r6, r7, r8, r9}
    stmia   r1!,{r2, r3, r4, r5, r6, r7, r8, r9}

// init the exception vectors are done.

    ldr     r1, =_STACK
    mov     sp, r1
    
    bl      main
_halt:
    wfi
    b       _halt

.global PUT32
PUT32:
    str r1,[r0]
    // mov pc, lr
    bx lr

.global GET32
GET32:
    ldr r0,[r0]
    // mov pc, lr
    bx lr

// based on https://github.com/rsta2/circle
.global interrupt_vector
interrupt_vector:
    sub     lr, lr, #4                  /* lr: return address */
    stmfd   sp!, {r0-r3, r12, lr}       /* save r0-r3, r12 and return address */
    ldr     r0, =IRQReturnAddress       /* store return address for profiling */
    str     lr, [r0]
    bl      dispatch
    ldmfd   sp!, {r0-r3, r12, pc}       /* restore registers and return */

.global IRQReturnAddress
IRQReturnAddress:
    .word   0
