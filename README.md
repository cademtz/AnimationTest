# AnimationTest
Primitive animation software written from scratch in C for Windows and no extra libraries.

#### Now compiles with clang! (Windows theme fixing manifest not included)

`./clang.sh` or `clang ezcompile.c -l 'user32.lib' -l 'gdi32.lib' -l 'comctl32.lib'`

##### Want to use other compilers?
Only `ezcompile.c` is needed, and be sure to link the same .libs specified in the clang args above
