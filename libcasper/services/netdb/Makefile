SHLIB_NAME=	netdb.so
SRCS+=	lua_netdb.c
CFLAGS+=${FLUA_CFLAGS} ${CASPER_CFLAGS}
LDADD+=	${CASPER_LDADD} -lcap_netdb
MAN=	casper.netdb.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
