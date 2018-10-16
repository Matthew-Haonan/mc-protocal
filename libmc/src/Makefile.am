lib_LTLIBRARIES = libmc.la
libmc_la_SOURCES = \
        mc.c \
        mc.h \
        mc-data.c \
        mc-private.h \
        mc-rtu.c \
        mc-rtu.h \
        mc-rtu-private.h \
        mc-tcp.c \
        mc-tcp.h \
        mc-tcp-private.h \
        mc-version.h

if OS_WIN32
libmc_la_LIBADD = -lwsock32
endif

if OS_QNX
libmc_la_LIBADD = -lsocket
endif

libmc_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined -version-info $(LIBMC_LT_VERSION_INFO)

# Header files to install
libmcincludedir = $(includedir)/mc
libmcinclude_HEADERS = mc.h mc-version.h mc-rtu.h mc-tcp.h

DISTCLEANFILES = mc-version.h
EXTRA_DIST = mc-version.h.in
CLEANFILES = *~
