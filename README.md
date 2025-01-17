# contest-tools
Programming contest preparation tools

These are tools for contest problem preparation.

## Reader

`Reader` parses input and output files. It is parametrized with a strictness
parameter:

* `strict`: require exact whitespace, no leading zeros, etc. Use this for input
  checkers
* `permissive`: is lenient about whitespace, leading zeros, etc. Use this for
  output verifiers

## Random

`Random` is a cryptographically strong random number generator. It can be used
to generate randomized test data, including in cases where the inputs are guaranteed
to be random according to a specified distribution.

A `Random` stream is determined by:

* `key`. This is hardcoded in `random.h`. A new key should be generated for each
  competition. See instructions in `random.h`.
* `problem_name`. A 0-4 character string, e.g. "abc".
* `test_id`. A 32-bit number unique for each test case.
