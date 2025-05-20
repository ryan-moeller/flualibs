#!/bin/sh

# A hack to make `make manlint` work with out-of-base cross-references.
# The manuals must have been installed by `make install` for this to work.
exec mandoc -T lint -W style -m /usr/local/share/man ${@#-Tlint}
