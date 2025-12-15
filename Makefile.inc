# NOTE: Must make cleandepend when changing SRCTOP between builds.
SRCTOP?=/usr/src

SHLIBDIR?=	${LIBDIR}/flua

FLUALIBSTOP:=	${.PARSEDIR:tA}
FLUA_CFLAGS:=	\
	-I${FLUALIBSTOP} \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \
