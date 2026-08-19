.syntax unified
.thumb

/* Keep module_initialize at a nonzero Thumb offset within the standard .text. */
.section .text, "ax", %progbits
.p2align 1
.type shellpp_ii_module_prelude, %function
shellpp_ii_module_prelude:
    bx lr
.size shellpp_ii_module_prelude, . - shellpp_ii_module_prelude
