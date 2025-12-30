SHLIB_NAME=	casper.so
SRCS+=	lua_casper.c
CFLAGS+=${FLUA_CFLAGS} ${CASPER_CFLAGS}
LDADD+=	${CASPER_LDADD}
MAN=	casper.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
