SHLIB_NAME=	pwd.so
SRCS+=	lua_pwd.c
CFLAGS+=${FLUA_CFLAGS} ${CASPER_CFLAGS}
LDADD+=	${CASPER_LDADD} -lcap_pwd
MAN=	casper.pwd.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
