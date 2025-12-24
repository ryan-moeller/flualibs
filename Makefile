SUBDIR= \
	b64 \
	dirent \
	fcntl \
	libfetch \
	libifconfig \
	libmagic \
	libmd \
	libnv \
	libpathconv \
	libpthread \
	mqueue \
	netdb \
	netinet \
	stdio \
	sys \
	time \
	unistd \
	xor \

.include "Makefile.inc"

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
