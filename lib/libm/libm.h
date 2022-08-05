/*****************************************************************************/
/*****************************************************************************/
// portions extracted from musl-0.9.15 libm.h
/*****************************************************************************/
/*****************************************************************************/

/* origin: FreeBSD /usr/src/lib/msun/src/math_private.h */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include <stdint.h>
#include <math.h>

#define FLT_EVAL_METHOD 0

// Upstream, this contains code that attempts to prevent the compiler
// from optimizing out the expression 'x' just for the side effect of
// the expression when it comes to 'subnormal' numbers. CircuitPython
// does not use or rely on this behavior, and removing the expressions
// entirely saves flash space, so the body of FORCE_EVAL is made empty.
#define FORCE_EVAL(x) do {                        \
} while(0)

/* Get a 32 bit int from a float.  */
#define GET_FLOAT_WORD(w,d)                       \
do {                                              \
  union {float f; uint32_t i;} __u;               \
  __u.f = (d);                                    \
  (w) = __u.i;                                    \
} while (0)

/* Set a float from a 32 bit int.  */
#define SET_FLOAT_WORD(d,w)                       \
do {                                              \
      union {float f; uint32_t i;} __u;           \
      __u.i = (w);                                \
      (d) = __u.f;                                \
} while (0)
