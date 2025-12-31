SHLIB_NAME=	capsicum_helpers.so
SRCS+=	lua_capsicum_helpers.c
CFLAGS+=${FLUA_CFLAGS} -DWITH_CASPER
LDADD+=	-lcasper
MAN=	capsicum_helpers.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
