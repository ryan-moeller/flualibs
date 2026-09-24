# FreeBSD Lua Libraries

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

- replace Cirrus CI
- more modules
- improve tests, samples, and documentation
- enable targetting Lua from ports instead of flua
