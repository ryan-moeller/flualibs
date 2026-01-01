SHLIB_NAME=	syslog.so
SRCS+=	lua_syslog.c
CFLAGS+=${FLUA_CFLAGS} ${CASPER_CFLAGS}
LDADD+=	${CASPER_LDADD} -lcap_syslog
MAN=	casper.syslog.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
