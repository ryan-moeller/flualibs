SHLIB_NAME=	fetch.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_fetch.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lfetch

MAN=	fetch.3lua

.include <bsd.lib.mk>
