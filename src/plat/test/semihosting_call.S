/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch/armv7a/asm_macros_common.S>

	.globl	semihosting_call

func semihosting_call
	svc	#0x123456
	bx	lr
endfunc semihosting_call
