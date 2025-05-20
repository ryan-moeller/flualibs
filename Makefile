SRCTOP?= /usr/src
.export SRCTOP

SUBDIR= \
	fileno \
	libbe \
	libbsddialog \
	libfetch \
	libkldstat \
	libkqueue \
	libmd \
	libmq \
	libroken \
	libxor \

MANDOC_CMD=	sh ../manlint.sh

.include <bsd.subdir.mk>
