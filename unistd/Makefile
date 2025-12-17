SHLIB_NAME=	unistd.so
SRCS+=	lua_unistd.c
CFLAGS+=${FLUA_CFLAGS}
MAN=	\
	confstr.3lua \
	copy_file_range.3lua \
	pathconf.3lua \
	sysconf.3lua \
	truncate.3lua \

.include "../Makefile.inc"
.include <bsd.lib.mk>
