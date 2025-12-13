SHLIB_NAME=	copy_file_range.so
SHLIBDIR=	${LIBDIR}/flua

SRCS+=		lua_copy_file_range.c

CFLAGS+= \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

MAN=	copy_file_range.3lua

.include <bsd.lib.mk>
