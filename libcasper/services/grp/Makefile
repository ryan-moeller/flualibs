SHLIB_NAME=	grp.so
SRCS+=	lua_grp.c
CFLAGS+=${FLUA_CFLAGS} ${CASPER_CFLAGS}
LDADD+=	${CASPER_LDADD} -lcap_grp
MAN=	casper.grp.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
