SHLIB_NAME=	sysctl.so
SRCS+=	lua_sysctl.c
CFLAGS+=${FLUA_CFLAGS} ${CASPER_CFLAGS}
LDADD+=	${CASPER_LDADD} -lcap_sysctl
MAN=	casper.sysctl.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
