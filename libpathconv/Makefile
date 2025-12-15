SHLIB_NAME=	pathconv.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_pathconv.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

LDADD+=	-lpathconv

MAN=	pathconv.3lua

.include <bsd.lib.mk>
