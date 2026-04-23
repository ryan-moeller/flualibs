SHLIB_NAME=	nvpair.so
SRCS+=	lua_nvpair.c
CFLAGS+=${FLUA_CFLAGS}
LDADD+=	-lnvpair -lpthread
MAN=	nvpair.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
