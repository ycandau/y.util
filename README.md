# setcalc

>  A Max external to perform musical set theory calculations.

Author: Yves Candau

Extends the similar object by Matthew McCabe.

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at [http://mozilla.org/MPL/2.0/](http://mozilla.org/MPL/2.0/).

## Inlet

0. A list of pitch values: integers from 0 to 11.

## Outlets
0. The **Forte number** for the set.
1. The **interval class vector**: a 6-member tally of the interval classes between distinct elements of a set.
2. The **prime form** for the set (same length as sorted set). Depending on the attribute `prime_type`, the prime form is determined by the Rahn algorithm or the Forte algorithm (i.e. left packing from right to left or left to right).
3. The **normal form** for the set (same length as sorted set).
4. The input set, sorted and with duplicates removed.
5. The **complementary set** (length is 12 minus the sorted set length).

## Arguments

None.

## Attributes

- **prime_type**: choose between Forte or Rahn prime forms.
- **verbose**: report warnings on invalid input or not.

## Messages

Integers or lists with pitch values clipped between 0 and 11.

## Notes

Invalid input values are simply dropped with an optional warning.

## Links
- [http://www.jaytomlin.com/music/settheory/help.html](http://www.jaytomlin.com/music/settheory/help.html)
- [http://solomonsmusic.net/pcsets.htm](http://solomonsmusic.net/pcsets.htm)


## Changes from the original setcalc

- Choose between Forte or Rahn prime forms.
- The external can process sets of any length (not just 3 to 9).
- 32-bit and 64-bit support.
- Optional warnings on invalid inputs.
