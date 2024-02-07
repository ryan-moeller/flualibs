
SHLIB_NAME=	mq.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_mq.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lrt

MAN=	mq.3lua

.include <bsd.lib.mk>
