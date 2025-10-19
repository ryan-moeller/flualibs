SHLIB_NAME=	kqueue.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_kqueue.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	kqueue.3lua

.include <bsd.lib.mk>
