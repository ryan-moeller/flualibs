SHLIB_NAME=	pthread.so
SRCS+=	lua_pthread.c
CFLAGS+=${FLUA_CFLAGS}
LDADD+=	-lpthread
MAN=	pthread.3lua

.include "../Makefile.inc"
.include <bsd.lib.mk>
