# NOTE: Must make cleandepend when changing SRCTOP between builds.
SRCTOP?=/usr/src

SHLIBDIR?=	${LIBDIR}/flua

FLUALIBSTOP:=	${.PARSEDIR:tA}
FLUA_CFLAGS:=	\
	-I${FLUALIBSTOP} \
	-I${SRCTOP}/contrib/lua/src \
	-I${SRCTOP}/lib/liblua \

ZFSTOP?=${SRCTOP}/sys/contrib/openzfs
ZFS_CFLAGS:=	\
	-DIN_BASE -DHAVE_RPC_TYPES \
	-I${ZFSTOP}/include \
	-I${ZFSTOP}/include/os/freebsd \
	-I${ZFSTOP}/lib/libspl/include \
	-I${ZFSTOP}/lib/libspl/include/os/freebsd \
	-I${ZFSTOP}/lib/libzfs \
	-I${ZFSTOP}/lib/libzpool/include \
	-I${SRCTOP}/sys \
	-I${SRCTOP}/cddl/compat/opensolaris/include \
	-include ${ZFSTOP}/include/os/freebsd/spl/sys/ccompile.h \
	-DHAVE_ISSETUGID -DHAVE_STRLCAT -DHAVE_STRLCPY \
