# NOTE: Must make cleandepend when changing SRCTOP between builds.
SRCTOP?= /usr/src
.export SRCTOP

SUBDIR= \
	acl \
	b64 \
	capsicum \
	chflags \
	clock \
	confstr \
	cpuset \
	extattr \
	fileno \
	kldstat \
	kqueue \
	libfetch \
	libifconfig \
	libmagic \
	libmd \
	libnv \
	libpthread \
	mac \
	mq \
	pathconf \
	sendfile \
	sysconf \
	sysctl \
	xor \

.if exists(${SRCTOP}/contrib/bsddialog/lib/bsddialog.h)
SUBDIR+=libbsddialog
.endif
.if exists(/usr/include/libnvpair.h)
SUBDIR+=libbe
.endif
.if exists(/usr/include/dpv.h)
SUBDIR+=libdpv
.endif

MANDOC_CMD=	sh ../manlint.sh

.include <bsd.subdir.mk>
