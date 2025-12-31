SHLIB_NAME=	fileargs.so
SRCS+=	lua_fileargs.c
CFLAGS+=${FLUA_CFLAGS} ${CASPER_CFLAGS}
LDADD+=	${CASPER_LDADD} -lcap_fileargs
MAN=	casper.fileargs.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
