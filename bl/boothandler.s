  .syntax unified
  .cpu cortex-m0
  .fpu softvfp
  .thumb

.global Boot_Handler

  .section .bootstart
  .type Boot_Handler, %function
Boot_Handler:
  ldr   r0, =_estack
  subs  r0, #4
  ldr   r0, [r0]
  ldr   r1, =#0x873d9293
  cmp   r0, r1
  beq   .app_start
  bl    Reset_Handler

.app_start:
  movs  r0, #13*4
  ldr   r0, [r0]
  blx   r0

.size Boot_Handler, .-Boot_Handler
