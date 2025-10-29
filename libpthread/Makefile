SHLIB_NAME=	pthread.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_pthread.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lpthread

MAN=	pthread.3lua

.include <bsd.lib.mk>
