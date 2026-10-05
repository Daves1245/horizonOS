#!/bin/bash

# doing `clang-format "$(git ls-files '*.c"' '*.h')"` will overflow the
# command and fail halfway with a "filename too long" error

git ls-files "*.c" "*.h" | xargs clang-format -i $x
