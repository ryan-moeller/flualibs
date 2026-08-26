SHLIB_NAME=	devstat.so
SRCS+=	lua_devstat.c
CFLAGS+=${FLUA_CFLAGS}
LDADD+=	-ldevstat
MAN=	devstat.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
