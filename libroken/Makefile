
SHLIB_NAME=	roken.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_roken.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lroken

MAN=	roken.3lua

.include <bsd.lib.mk>
