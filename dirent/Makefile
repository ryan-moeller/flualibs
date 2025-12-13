SHLIB_NAME=	dirent.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_dirent.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	dirent.3lua

.include <bsd.lib.mk>
