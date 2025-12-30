SHLIB_NAME=	net.so
SRCS+=	lua_net.c
CFLAGS+=${FLUA_CFLAGS} ${CASPER_CFLAGS}
LDADD+=	${CASPER_LDADD} -lcap_net
MAN=	casper.net.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
