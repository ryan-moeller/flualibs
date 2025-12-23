SHLIB_NAME=	ifconfig.so
SRCS+=	lua_ifconfig.c
CFLAGS+=${FLUA_CFLAGS}
LDADD+=	-lsysdecode
MAN=	ifconfig.3lua

.include "../Makefile.inc"

# XXX: libifconfig is an internal library, so we build it in as a workaround.
# If this seems to be picking up files from the wrong SRCTOP, make cleandepend.
.PATH:	${SRCTOP}/lib/libifconfig

SRCS+=	\
	libifconfig.c \
	libifconfig_bridge.c \
	libifconfig_carp.c \
	libifconfig_inet.c \
	libifconfig_inet6.c \
	libifconfig_internal.c \
	libifconfig_lagg.c \
	libifconfig_media.c \
	libifconfig_sfp.c \

.if exists(libifconfig_nl.c)
SRCS+=	libifconfig_nl.c
.endif

GEN=	\
	libifconfig_sfp_tables.h \
	libifconfig_sfp_tables.c \
	libifconfig_sfp_tables_internal.h \

SRCS+=	${GEN}

CFLAGS+= \
	-I${SRCTOP}/lib/libifconfig \
	-I${.CURDIR} \

LUA=	/usr/libexec/flua

.SUFFIXES: .tpl.c .tpl.h
.tpl.c.c .tpl.h.h: sfp.lua
	${LUA} ${SRCTOP}/lib/libifconfig/sfp.lua ${.IMPSRC} >${.TARGET}

CLEANFILES+=	${GEN}

.include <bsd.lib.mk>
