# FreeBSD Lua Libraries

[![13.5-STABLE Build Status](https://api.cirrus-ci.com/github/ryan-moeller/flualibs.svg?branch=main&task=snapshots/amd64/13.5-STABLE)](https://cirrus-ci.com/github/ryan-moeller/flualibs)
[![14.3-STABLE Build Status](https://api.cirrus-ci.com/github/ryan-moeller/flualibs.svg?branch=main&task=snapshots/amd64/14.3-STABLE)](https://cirrus-ci.com/github/ryan-moeller/flualibs)
[![15.0-STABLE Build Status](https://api.cirrus-ci.com/github/ryan-moeller/flualibs.svg?branch=main&task=snapshots/amd64/15.0-STABLE)](https://cirrus-ci.com/github/ryan-moeller/flualibs)
[![13.5-RELEASE Build Status](https://api.cirrus-ci.com/github/ryan-moeller/flualibs.svg?branch=main&task=releases/amd64/13.5-RELEASE)](https://cirrus-ci.com/github/ryan-moeller/flualibs)
[![14.3-RELEASE Build Status](https://api.cirrus-ci.com/github/ryan-moeller/flualibs.svg?branch=main&task=releases/amd64/14.3-RELEASE)](https://cirrus-ci.com/github/ryan-moeller/flualibs)
[![15.0-RELEASE Build Status](https://api.cirrus-ci.com/github/ryan-moeller/flualibs.svg?branch=main&task=releases/amd64/15.0-RELEASE)](https://cirrus-ci.com/github/ryan-moeller/flualibs)

flualibs is a collection of Lua modules for the FreeBSD base system.  It has no
dependencies outside of the FreeBSD base system and sources.

## Build Example

The build system is currently set up to build and install the modules for the
FreeBSD base system's Lua interpreter (flua).  Adapting the Makefiles to build
and install for Lua from ports should be easy enough.

```
$ make
# make install # optional
```

## TODO

- more modules
- improve tests, samples, and documentation
- enable targetting Lua from ports instead of flua
