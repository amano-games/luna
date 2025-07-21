#pragma once

#define flag_set(n, f)        ((n) |= (f))
#define flag_clr(n, f)        ((n) &= ~(f))
#define flag_tgl(n, f)        ((n) ^= (f))
#define flag_eq(n, f)         ((n) == (f))         // Checks if 'n' is exactly equal to 'f'.
#define flag_has(n, f)        (((n) & (f)) == (f)) // Checks if all bits in 'f' are set in 'n'.
#define flag_intersects(n, f) (((n) & (f)) > 0)    // Checks if any bits in 'f' are set in 'n'.
