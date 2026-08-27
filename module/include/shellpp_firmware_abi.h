#ifndef SHELLPP_FIRMWARE_ABI_H
#define SHELLPP_FIRMWARE_ABI_H

/*
 * Stable nativeApp-facing ABI contract. The build project generates
 * shellpp_target_abi.h from one firmware profile for every compilation.
 * Adding another firmware therefore does not require source changes here.
 */
#include "shellpp_target_abi.h"

#ifndef SHELLPP_TARGET_ABI_GENERATED
#error "Shell++ II must be compiled through shellpp-ii-build"
#endif

#endif
