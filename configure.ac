# Process this file with autoconf to produce a configure script.C
AC_INIT(rts2, 0.9.5, petr@rts2.org)

AC_CANONICAL_BUILD
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([gnu dist-bzip2])

LT_INIT()

AC_PREREQ(2.50)

AC_CONFIG_MACRO_DIR([m4])

AM_MAINTAINER_MODE

# Checks for programs.
AM_PROG_CC_C_O
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

AM_PATH_PYTHON

libcheck="no"

PKG_CHECK_MODULES([CHECK],[check >= 0.9.6],,AC_MSG_RESULT(not found))
AM_CONDITIONAL(LIBCHECK, test "x$CHECK_LIBS" != x)
AS_IF([test "x$CHECK_LIBS" != x], [
  AC_DEFINE_UNQUOTED(LIBCHECK, "1", [use libcheck checking])
  libcheck="yes"
])

liberfa="no"

PKG_CHECK_MODULES([ERFA],[erfa >= 0.1.0],,AC_MSG_RESULT(not found))
AM_CONDITIONAL(LIBERFA, test "x$ERFA_LIBS" != x)
AS_IF([test "x$ERFA_LIBS" != x], [
  AC_DEFINE_UNQUOTED(LIBERFA, "1", [use erfa/sofa library instead of libnova])
  liberfa="yes"
])

libwebsockets="no"

PKG_CHECK_MODULES([LIBWEBSOCKETS],[libwebsockets >= 1.7.0 libwebsockets < 2.2.0],,AC_MSG_RESULT(not found))
AM_CONDITIONAL(LIBWEBSOCKETS,test "x$LIBWEBSOCKETS_LIBS" != x)
AS_IF([test "x$LIBWEBSOCKETS_LIBS" != x], [
  AC_DEFINE_UNQUOTED(LIBWEBSOCKETS, "1", [has libwebsockets])
  libwebsockets="yes"
])

hiredis="no"

PKG_CHECK_MODULES([HIREDIS],[hiredis >= 0.10],,AC_MSG_RESULT(not fould))
AM_CONDITIONAL(HIREDIS,test "x$HIREDIS_LIBS" != x)
AS_IF([test "x$HIREDIS_LIBS" != x], [
  AC_DEFINE_UNQUOTED(HIREDIS, "1", [has hiredis])
  hiredis="yes"
])

# check for lang
AC_LANG_CPLUSPLUS

CFLAGS="-Wall -Wshadow -g"
AC_SUBST(CFLAGS)
CXXFLAGS="-Wall -Wshadow -g -std=c++11"
AC_SUBST(CXXFLAGS)

AC_CHECK_LIB([m], [sqrt], LIB_M="-lm";
LIBS="${LIBS} -lm"; AC_SUBST(LIB_M), [cat << EOF
**** You don't have math library.
**** Please install one and make sure, that it is available.
EOF
exit 1])

AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, gethostbyname)

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_TYPE_SIGNAL
AC_FUNC_STRTOD
AC_CHECK_FUNCS([dup2 floor gethostbyname gettimeofday inet_ntoa memmove memset poll socket strchr strdup strerror strtol mkdir sqrt strcasecmp strncasecmp pow getaddrinfo getopt_long flock strtod isinf scandir alphasort isblank strcasestr trunc getline inotify_init inotify_add_watch inotify_init1 nftw round strtof isatty])

AC_FUNC_CHOWN
AC_FUNC_MEMCMP
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_TYPE_MODE_T

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([limits.h sys/ioccom.h argz.h arpa/inet.h dirent.h fcntl.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h termios.h unistd.h sys/inotify.h curses.h ncurses/curses.h endian.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

AC_ARG_ENABLE(ssl,
[  --enable-ssl            enable openssl encryption in httpd],
[case "${enableval}" in
	yes) ssl=yes ;;
	no)  ssl=no ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --enable-ssl) ;;
esac],[ssl=yes])

AC_ARG_ENABLE(focusing,
[  --disable-focusing      disables focusing in camd (=>camd does not need cfitsio to build)],
[case "${enableval}" in
	yes) focusing=yes ;;
	no)  focusing=no ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --enable-focusing) ;;
esac],[focusing=yes])

AM_CONDITIONAL(FOCUSING, test x$focusing = xyes)
AC_ARG_WITH(x-focusc,
[  --without-x-focusc      disables xfocusc],
[case "${withval}" in
	yes) xfocusc=yes ;;
	no)  xfocusc=no ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --enable-xfocusc) ;;
esac],[xfocusc=yes])

AM_CONDITIONAL(XFOCUSC, test x$xfocusc = xyes)

AC_ARG_WITH(libnova,
[  --with-libnova       path to libnova or "no" if libnova is not installed],
[case "${withval}" in
	yes) libnova=yes ;;
	no)  libnova=no ;;
	*) libnova=${withval} ;;
esac],[libnova=yes])

AM_CONDITIONAL(LIBNOVA, test x$libnova != xno)

AC_ARG_WITH(cfitsio,
[  --with-cfitsio       path to CFITSO library or "no" if cfitsio is not installed],
[case "${withval}" in
	yes) cfitsio=yes ;;
	no)  cfitsio=no ;;
	*) cfitsio=${withval} ;;
esac],[cfitsio=yes])

AS_IF([test "x${CFITSIO_LIBS}" != "xno"],[
	PKG_CHECK_MODULES([CFITSIO],[cfitsio],,[
		AC_MSG_RESULT(not found)
		AS_IF([test "x${cfitsio}" != "xyes"], [LDFLAGS="${LDFLAGS} -L${cfitsio}"])
		AC_CHECK_LIB([cfitsio], [ffinit], CFITSIO_LIBS="-lcfitsio",[
			cat <<EOF
**** cfitsio is not installed.
**** Please download one from
**** http://heasarc.gsfc.nasa.gov/docs/software/fitsio/fitsio.html
EOF
			exit 1])
		AS_IF([test "x${cfitsio}" != "xyes"],[
			CFITSIO_LIBS="-L${cfitsio} ${CFITSIO_LIBS}"
			CFITSIO_CFLAGS="-I${cfitsio}"
		],[
			CFITSIO_CFLAGS=""
		])
		AC_SUBST(CFITSIO_LIBS)
		AC_SUBST(CFITSIO_CFLAGS)
	])
])

AM_CONDITIONAL(CFITSIO, test "x${CFITSIO_LIBS}" != "x")

AC_ARG_WITH(ncurses,
[  --with-ncurses       path to ncurses library or "yes" if ncurses is in standard path],
[case "${withval}" in
	yes) ncurses=yes ;;
	*) ncurses=${withval} ;;
esac],[ncurses=yes])

PKG_CHECK_MODULES([NCURSESW],[ncursesw],,[
	AC_MSG_RESULT(not found)
	AS_IF([test "x${ncursesw}" != "xyes"], [LDFLAGS="${LDFLAGS} -L${ncursesw}"])
	AC_CHECK_LIB([ncursesw], [mvwprintw], [
		NCURSESW_LIBS="-lncursesw"
	],[
		cat<<EOF
**** ncursesw library is not installed.
**** Please install ncurses (development package)
EOF
		exit 1
	])
	AS_IF([test "x${ncursesw}" != "xyes"], [
		NCURSESW_LIBS="-L${ncursesw} ${NCURSES_LIBS}"
		NCURSESW_CFLAGS="-I${ncursesw}"
	],[
		NCURSESW_CFLAGS=""
	])
])

AC_SUBST(NCURSESW_LIBS)
AC_SUBST(NCURSESW_CFLAGS)

dnl Check for GraphicsMagic++ library

AH_TEMPLATE([HAVE_LIBJPEG],[If GraphicsMagick++ is installed])

PKG_CHECK_MODULES([MAGIC], [GraphicsMagick++], AC_DEFINE_UNQUOTED(HAVE_LIBJPEG,1,[If JPEG is installed]), AC_MSG_RESULT(not found))

AH_TEMPLATE([JSONSOUP],[JSON and SOUP libraries are present])

dnl check for JSON Glib
PKG_CHECK_MODULES([JSONGLIB], [json-glib-1.0 >= 0.1 libsoup-2.4 >= 0.1],,AC_MSG_RESULT(not found))

AM_CONDITIONAL(JSONSOUP, test "x$JSONGLIB_CFLAGS" != "x")

AS_IF([test "x$JSONGLIB_CFLAGS" != "x"], [
	AC_DEFINE_UNQUOTED(JSONSOUP, "1", [JSON and SOUP libraries are present])
])

libthrift="no"
PKG_CHECK_MODULES([THRIFT], [thrift],,AC_MSG_RESULT(not found))
AM_CONDITIONAL(LIBTHRIFT, test "x$THRIFT_LIBS" != x)
AS_IF([test "x$THRIFT_LIBS" != x], [
  AC_DEFINE_UNQUOTED(LIBTHRIFT, "1", [THRIFT is available])
  libthrift="yes"
])

AM_CONDITIONAL(THRIFT, test "x$THRIFT_CFLAGS" != x)

dnl Check for ROOT (to build contrib scripts)

AC_ARG_WITH(cern-root,
[  --with-cern-root        path to CERN ROOT config or "no" if it is not installed],
[case "${withval}" in
	yes) root_config="root-config" ;;
	no)  root_config=no ;;
	*) root_config=${withval} ;;
esac],[root_config="root-config"])

AC_MSG_CHECKING(looking for ROOT libraries)
AS_IF(["${root_config}" --libs > /dev/null 2>&1], [
	ROOT_VERS=`${root_config} --version`
	AC_MSG_RESULT(${ROOT_VERS} found)
], [
	ROOT_VERS="no"
	AC_MSG_RESULT(${root_config} not found)
	root_config="no"
])

dnl Check for the XML library
AC_ARG_WITH(xml2,
[  --with-xml2             specify path for libxml2 config file],
[case "${withval}" in
	yes) XML_CONFIG=xml2-config ;;
	no) AC_MSG_ERROR(You really need to have libxml2-dev) ;;
	*) XML_CONFIG=${withval} ;;
esac],[XML_CONFIG=xml2-config])
AC_MSG_CHECKING(looking for xml2 library)
AS_IF(["${XML_CONFIG}" --libs > /dev/null 2>&1], [
	XMLVERS=`${XML_CONFIG} --version`
	LIBXML_LIBS=`${XML_CONFIG} --libs`
	LIBXML_CFLAGS=`${XML_CONFIG} --cflags`
	AC_SUBST(LIBXML_LIBS)
	AC_SUBST(LIBXML_CFLAGS)
	AC_MSG_RESULT(${XMLVERS} found)
], [
	AC_MSG_ERROR([Could not find ${XML_CONFIG} anywhere, apt-get install libxml2-dev or check ftp://xmlsoft.org/])
])

AC_ARG_WITH(pgsql,
[  --without-pgsql         don't build pgsql dependent clients],
[case "${withval}" in
	yes) psql=yes ;;
	no)  psql=no ;;
	*) AC_MSG_ERROR(bad value ${withval} for --without-pgsql) ;;
esac],[psql=yes])

AC_ARG_WITH(wcstools,
[  --with-wcstools           path to WCS programs],
[case "${withval}" in
	yes) libwcstools=yes ;;
	no)  libwcstools=no ;;
	*) libwcstools=${withval} ;;
esac],[libwcstools=yes])


# check for EPICS
AC_ARG_WITH(epics,
[  --with-epics 	path to EPICS library, build epics dependent binaries],
[case "${withval}" in
	yes) epics=yes ;;
	no)  epics=no ;;
	*) epics=${withval} ;;
esac],[epics=no])

AM_CONDITIONAL(EPICS, test x$epics != xno)

libusb1="no"
PKG_CHECK_MODULES(USB1, [libusb-1.0],,AC_MSG_RESULT(not found))

AM_CONDITIONAL(LIBUSB1, test "x$USB1_LIBS" != x)
AS_IF([test "x$USB1_LIBS" != x], [
  AC_DEFINE_UNQUOTED(LIBUSB1, "1", [libusb1])
  libusb1="yes"
])

SBIG=yes

AC_ARG_WITH(sbig-include,
[  --with-sbig-include     path to SBIG USB CCD driver include files, build SBIG USB CCD driver],
SBIG_INC="${withval}";
INC_SBIG="-I${SBIG_INC}",
[SBIG=no])

AC_ARG_WITH(sbig-lib,
[  --with-sbig-lib         path to SBIG USB CCD driver library files, build SBIG USB CCD driver],
SBIG_LIB="${withval}";
LIB_SBIG="-L${SBIG_LIB} -lsbigudrv ${SBIG_INC}/csbigcam.o ${SBIG_INC}/csbigimg.o ${USB_LIBS}",
[SBIG=no])

AM_CONDITIONAL(SBIG, test x$SBIG != xno)
AC_SUBST(SBIG)
AC_SUBST(LIB_SBIG)
AC_SUBST(INC_SBIG)

AC_ARG_WITH(andor,
[  --with-andor            path to Andor CCD driver, build Andor CCD driver],
ANDOR="${withval}";
LIB_ANDOR="-landor",
[ANDOR=no])

AM_CONDITIONAL(ANDOR, test x$ANDOR != xno)
AC_SUBST(ANDOR)
AC_SUBST(LIB_ANDOR)
AC_SUBST(INC_ANDOR)

# test for EDT PDV boards
AC_ARG_WITH(edtpdv,
[  --with-edtpdv           path to EDT-PDV SDK files, needed to build EDT cameras (http://www.edt.com/software],
EDTPDV="${withval}";
LIB_EDTPDV="-L${EDTPDV} -lpdv -ldl";
INC_EDTPDV="-I${EDTPDV}",
[EDTPDV=no])

AM_CONDITIONAL(EDTPDV, test x$EDTPDV != xno)
AC_SUBST(EDTPDV)
AC_SUBST(LIB_EDTPDV)
AC_SUBST(INC_EDTPDV)

# Test for ARC/Leech controller headers

ARC=no

AH_TEMPLATE([ARC_API_3],[If ARC API version 3 is used])
AC_ARG_WITH(arc-3,
[  --with-arc-3              path to Astronomical Research Cameras headers, version 3.x. You can download them from http://www.astro-cam.com/arcpage.php?txt=software.php (you will need to get password from ARC).],
ARC_3="${withval}";
ARC="${withval}";
AS_IF([test x${target_os} == xcygwin], [
	LIB_ARC="${ARC}/CController/CController.a ${ARC}/CDeinterlace/CDeinterlace.o /lib/w32api/libsetupapi.a";
	INC_ARC="-I${ARC}/CArcDevice/src -I${ARC}/CArcDeinterlace/src"
], [
	LIB_ARC="-L${ARC}/Release/x64 -lCArcDevice -lCArcDeinterlace";
	INC_ARC="-I${ARC}/CArcDevice/src -I${ARC}/CArcDeinterlace/src -std=gnu++11"
]),
[ARC_3=no])

AS_IF([test x$ARC_3 != xno], [
	AC_DEFINE_UNQUOTED([ARC_API_3],1,[If ARC API version 3 is used])
])

AH_TEMPLATE([ARC_API_2],[If ARC API version 2 is used])
AC_ARG_WITH(arc-2,
[  --with-arc-2              path to Astronomical Research Cameras headers, version 2.0. You can download them from http://www.astro-cam.com/arcpage.php?txt=software.php (you will need to get password from ARC).],
ARC_2="${withval}";
ARC="${withval}";
AS_IF([test x${target_os} == xcygwin] , [
	LIB_ARC="${ARC}/CController/CController.a ${ARC}/CDeinterlace/CDeinterlace.o /lib/w32api/libsetupapi.a";
	INC_ARC="-DARC_API_2 -I${ARC}"
], [
	LIB_ARC="-L${ARC}/Release -lCController";
	INC_ARC="-DARC_API_2 -I${ARC}"
]),
[ARC_2=no])

AS_IF([test x$ARC_2 != xno], [
	AC_DEFINE_UNQUOTED([ARC_API_2],1,[If ARC API version 2 is used])
])

AH_TEMPLATE([ARC_API_1_7],[If ARC API version 1.7 is used])
AC_ARG_WITH(arc-1-7,
[  --with-arc-1-7          path to Astronomical Research Cameras headers, version 1.7. You can download them from http://www.astro-cam.com/arcpage.php?txt=software.php (you will need to get password from ARC).],
ARC_1_7="${withval}";
ARC="${withval}"
LIB_ARC="-lastropciAPI";
INC_ARC="-DARC_API_1_7 -I${ARC_1_7}/src",
[ARC_1_7=no])

AS_IF([test x$ARC_1_7 != xno], [
	AC_DEFINE_UNQUOTED([ARC_API_1_7],1,[If ARC API version 1.7 is used])
])

AM_CONDITIONAL(ARC, test x$ARC != xno)
AC_SUBST(ARC)
AC_SUBST(LIB_ARC)
AC_SUBST(INC_ARC)

# check for ASI SDK
AC_ARG_WITH(asi,
[  --with-asi                ASI SDK path],
ASI="${withval}";
ASI_INCLUDE=${ASI}/include;
AS_IF([test `uname -i` == 'x86_64'], [ASI_LIB=${ASI}/lib/x64],
	[AS_IF([test `uname -i` == 'i686'], [ASI_LIB=${ASI}/lib/x86])]
),
[ASI=no])

AM_CONDITIONAL(ASI, test x$ASI != xno)
AC_SUBST(ASI)
AC_SUBST(ASI_INCLUDE)
AC_SUBST(ASI_LIB)

# check for Princeton SDK
AC_ARG_WITH(princeton,
[  --with-princeton          Princeton SDK path],
PRINCETON="${withval}";
PI_INCLUDE=${PRINCETON}/include;
PI_LIB=${PRINCETON}/lib,
[PRINCETON=no])

AM_CONDITIONAL(PRINCETON, test x$PRINCETON != xno)
AC_SUBST(PRINCETON)
AC_SUBST(PI_INCLUDE)
AC_SUBST(PI_LIB)

# for Alta we need special library:(
AC_ARG_WITH(alta,
[  --with-alta             path to Apogee Alta CCD driver, build Apogee Alta CCD driver],
# checks for libusb-config
ALTA="${withval}";
LIB_ALTA_USB="${ALTA}/apogee_USB.so ${USB_LIBS}";
LIB_ALTA_NET="${ALTA}/apogee_NET.so ${USB_LIBS}";
INC_ALTA="-I${ALTA} -I${ALTA}/FpgaRegs ${USB_CFLAGS}",
[ALTA=no])

AM_CONDITIONAL(ALTA, test x$ALTA != xno)
AC_SUBST(ALTA)
AC_SUBST(LIB_ALTA_USB)
AC_SUBST(LIB_ALTA_NET)
AC_SUBST(LIB_ALTA_FILTER)
AC_SUBST(INC_ALTA)

# FLI..
AC_ARG_WITH(fli,
[  --with-fli             path to libfli CCD driver, build FLI CCD driver],
FLI="${withval}";
LIB_FLI="-L${FLI} -lfli";
INC_FLI="-I${FLI}",
[FLI=no])

AM_CONDITIONAL(FLI, test x$FLI != xno)
AC_SUBST(FLI)
AC_SUBST(LIB_FLI)
AC_SUBST(INC_FLI)

AC_ARG_WITH(miccd,
[  --with-miccd           path to MI CCD driver, build MI CCD driver],
MICCD="${withval}";
LIB_MICCD="${MICCD}/lib/libmiccd.a";
INC_MICCD="-I${MICCD}/include",
[MICCD=no])

AM_CONDITIONAL(MICCD, test x$MICCD != xno)
AC_SUBST(MICCD)
AC_SUBST(LIB_MICCD)
AC_SUBST(INC_MICCD)

AC_CHECK_LIB([pthread], [pthread_exit], LIB_PTHREAD="-lpthread";
AC_SUBST(LIB_PTHREAD), [cat << EOF
**** You don't have pthread library installed.
**** Please try to get it somewhere.
EOF
exit 1])

AC_ARG_WITH(gxccd,
[  --with-gxccd           path to GX CCD driver, build GX CCD driver],
GXCCD="${withval}";
GXCCD_LIBS="${GXCCD}/lib/libgxccd.a -lrt";
GXCCD_CFLAGS="-I${GXCCD}/include",
[GXCCD=no])

AM_CONDITIONAL(GXCCD, test x$GXCCD != xno -a "x$USB1_LIBS" != x)
AC_SUBST(GXCCD)
AC_SUBST(GXCCD_LIBS)
AC_SUBST(GXCCD_CFLAGS)

AC_ARG_WITH(k8055,
[  --with-k8055           path to Velleman k8055 library],
K8055="${withval}";
LIB_K8055="-lk8055 ${USB_LIBS}",
K8055=no;
LIB_K8055='')

AH_TEMPLATE([WITH_K8055],[K8055 library is not present])

AS_IF([test x$K8055 != xno], [
	AC_DEFINE_UNQUOTED(WITH_K8055, "1", [K8055 library is present])
])

AM_CONDITIONAL(K8055, test x$K8055 != xno)
AC_SUBST(K8055)
AC_SUBST(LIB_K8055)

# check for SI3097
AC_ARG_WITH(si3097,
[  --with-si3097        path to Spectral Instruments SI3097 PCI card driver, build SI CCD support],
SI3097="${withval}";
INC_SI3097="-I${SI3097}",
[SI3097=no])

AM_CONDITIONAL(SI3097, test x$SI3097 != xno)
AC_SUBST(SI3097)
AC_SUBST(INC_SI3097)

# check for COMEDI
AC_ARG_WITH(comedi,
[  --with-comedi	path to COMEDI library, build comedi dependent binaries],
COMEDI="${withval}";
LIB_COMEDI="-lcomedi";
AS_IF([test "x${withval}" != "xyes"], [INC_COMEDI="-I${withval}"], [INC_COMEDI=""]),
[COMEDI=no])

AM_CONDITIONAL(COMEDI, test x$COMEDI != xno)
AC_SUBST(COMEDI)
AC_SUBST(LIB_COMEDI)
AC_SUBST(INC_COMEDI)

# check for paramount library
AC_ARG_WITH(paralib,
[  --with-paralib          path to Paramount library, build Paramount driver],
PARALIB="${withval}";
LIB_PARA="${PARALIB}/libmks3.o";
INC_PARA="-I${PARALIB}",
[PARALIB=no])

AM_CONDITIONAL(PARALIB, test x$PARALIB != xno)
AC_SUBST(PARALIB)
AC_SUBST(LIB_PARA)
AC_SUBST(INC_PARA)

AC_ARG_WITH(mdm,
[  --with-mdm              path to MDM (Kitt Peak, Arizona) TCS library, build MDM driver],
MDMTCS="${withval}";
LIB_MDMTCS="${MDMTCS}/libtcs.a"
INC_MDMTCS="-I${MDMTCS}",
[MDMTCS=no])

AM_CONDITIONAL(MDMTCS, test x$MDMTCS != xno)
AC_SUBST(MDMTCS)
AC_SUBST(LIB_MDMTCS)
AC_SUBST(INC_MDMTCS)

# check for PCI8255
AC_ARG_WITH(pci8255,
[  --with-pic8255          path to PCI8255 sources, build Dublin dome],
PCI8255="${withval}";
LIB_PCI8255="-L${PCI8255}";
INC_PCI8255="-I${PCI8255}",
[PCI8255=no])

AM_CONDITIONAL(PCI8255, test x$PCI8255 != xno)
AC_SUBST(PCI8255)
AC_SUBST(LIB_PCI8255)
AC_SUBST(INC_PCI8255)

# check for GPIB
AC_ARG_WITH(gpib,
[  --with-gpib             build with support for GPIB devices],
GPIBLIB="${withval}";
LIB_GPIB="-lgpib",
[GPIBLIB=no])

AS_IF([test "x$GPIBLIB" != "xno"], [
  AC_CHECK_HEADERS([gpib/ib.h])
])

AM_CONDITIONAL(GPIBLIB, test x$GPIBLIB != xno)
AC_SUBST(GPIBLIB)
AC_SUBST(LIB_GPIB)

# Check for A3200
AC_ARG_WITH(a3200,
[  --with-a3200           path for A3200 SDK],
A3200="${withval}";
LIB_A3200="${withval}/Lib/A32Cmplr.lib ${withval}/Lib/A32Sys.lib";
INC_A3200="-I${withval}/Include",
[A3200=no])

AM_CONDITIONAL(A3200, test x$A3200 != xno)
AC_SUBST(A3200)
AC_SUBST(LIB_A3200)
AC_SUBST(INC_A3200)

# Check for Boltwood
AC_ARG_WITH(boltwood,
[  --with-boltwood        path for Boltwood SDK],
BOLTWOOD="${withval}";
LIB_BOLTWOOD="${withval}/weather/libOCWeather.a ${withval}/common/libOCCdrivers.a ${withval}/util/libMTutil.a";
INC_BOLTWOOD="-I${withval}",
[BOLTWOOD=no])

AM_CONDITIONAL(BOLTWOOD, test x$BOLTWOOD != xno)
AC_SUBST(BOLTWOOD)
AC_SUBST(LIB_BOLTWOOD)
AC_SUBST(INC_BOLTWOOD)


# check for T-Balancer bigNG
AC_ARG_WITH(bigng,
[  --with-bigng            path for Tban SDK (libtan)],
BIGNG="${withval}";
LIB_BIGNG="${withval}/big_ng.o ${withval}/parser.o ${withval}/tban.o ${withval}/libtban.a";
INC_BIGNG="-I${withval}",
[BIGNG=no])

AM_CONDITIONAL(BIGNG, test x$BIGNG != xno)
AC_SUBST(BIGNG)
AC_SUBST(LIB_BIGNG)
AC_SUBST(INC_BIGNG)

# Define prefix variable

AH_TEMPLATE([CONFDIR],[configuration directory])
AH_TEMPLATE([SHARE_PREFIX],[share prefix])

AC_ARG_WITH(confdir,
[  --with-confdir          path to RTS2 configuration directory],
CONFDIR="${withval}",
CONFDIR="/etc")

AS_IF([test "x${prefix}" = "x/"], [
        AC_DEFINE_UNQUOTED(SHARE_PREFIX, "/usr/share", [share prefix])
], [
	AS_IF([test "x${prefix}" = "x" -o "x${prefix}" = "xNONE"], [
		loc_prefix="/usr/local"
	], [
		loc_prefix="${prefix}"
	])
        AC_DEFINE_UNQUOTED(SHARE_PREFIX, "${loc_prefix}/share", [share prefix])
])

AC_DEFINE_UNQUOTED(CONFDIR,"${CONFDIR}", [configuration directory])

AC_SUBST(CONFDIR)
AC_SUBST(SHARE_PREFIX)

# Define lock file prefix
AC_ARG_WITH(lock,
[  --with-lock             specify lock file path prefix, default to /var/run/rts2_],
LOCK_PREFIX="${withval}",
LOCK_PREFIX="/var/run/rts2_")

AH_TEMPLATE([LOCK_PREFIX],[Lock file prefix path])

AC_DEFINE_UNQUOTED(LOCK_PREFIX, "$LOCK_PREFIX", [Lock file prefix path])
AC_SUBST(LOCK_PREFIX)

AC_ARG_WITH(log,
[  --with-log              specify log file path prefix, default to /var/log/rts2-debug],
LOG_FILE="${withval}",
LOG_FILE="/var/log/rts2-debug")

AH_TEMPLATE([LOG_FILE],[Log file])

AC_DEFINE_UNQUOTED(LOG_FILE, "$LOG_FILE", [Log file])
AC_SUBST(LOG_FILE)

# Default centrald port
AC_ARG_WITH(port,
[  --with-port             default centrald port. If this value is not specified, 617 is used],
CENTRALD_PORT="${withval}",
CENTRALD_PORT="617")

AH_TEMPLATE([CENTRALD_PORT],[Default centrald port])

AC_DEFINE_UNQUOTED(CENTRALD_PORT, "$CENTRALD_PORT", [Default centrald port])
AC_SUBST(CENTRALD_PORT)


# Checks for libraries.

AH_TEMPLATE([SSL], [using openssl in httpd])
AS_IF([test "x${ssl}" != "xno"], [
  PKG_CHECK_MODULES(SSL, [openssl >= 1.0.1],,AC_MSG_RESULT(not found))
])
AM_CONDITIONAL(SSL, test "x$SSL_LIBS" != x)
AS_IF([test "x$SSL_LIBS" != x], [
  AC_DEFINE_UNQUOTED(SSL, "1", [using openssl in httpd])
])

AS_IF([test "x${libwcstools}" != "xno"], [
  AS_IF([test "x${libwcstools}" != "xyes"], [
    LDFLAGS="${LDFLAGS} -L${libwcstools}/libwcstools"
  ])
  AC_CHECK_LIB([wcstools], [wcs2pix], LIBWCS_LDFLAGS="-lwcstools",
  [cat <<EOF
**** You don't have wcstools installed.
**** You can download one from
**** http://tdc-www.harvard.edu/software/wcstools/.
**** That will not affect compilation, as RTS2 comes with
**** own WCS functions in case wcstools are not available.
EOF
  libwcstools=no
  ])
  AS_IF([test "x${libwcstools}" != "xyes"], [
    LIBWCS_LDFLAGS="-L${libwcstools}/libwcstools -lwcstools"
    LIBWCS_CFLAGS="-I${libwcstools}"
  ], [
    LIBWCS_LDFLAGS="-lwcstools"
  ])
])

AM_CONDITIONAL(LIBWCS, test x$libwcstools != xno)
AC_SUBST(LIBWCS_CFLAGS)
AC_SUBST(LIBWCS_LDFLAGS)

AS_IF([test "x${libnova}" != "xyes"], [
  LDFLAGS="${LDFLAGS} -L${libnova}/lib"
])
AC_CHECK_LIB([nova], [ln_get_version], LIB_NOVA="-lnova",
[cat << EOF
**** You don't have libnova library version >= 0.10.
**** Please get one from http://libnova.sourceforge.net.
EOF
exit 1])
AS_IF([test "x${libnova}" != "xyes"], [
  LIB_NOVA="-L${libnova}/lib ${LIB_NOVA}"
  NOVA_CFLAGS="-I${libnova}/include"
], [
  NOVA_CFLAGS=""
])
AC_SUBST(LIB_NOVA)
AC_SUBST(NOVA_CFLAGS)

AH_TEMPLATE([HAVE_DECL_LN_GET_HELIOCENTRIC_TIME_DIFF],[If Libnova provides ln_get_heliocentric_time_diff function])

AC_CHECK_DECL(ln_get_heliocentric_time_diff, , ,[#include <libnova/libnova.h>])
AS_IF([test "x$ac_cv_have_decl_ln_get_heliocentric_time_diff" = "xyes"], [
	AC_DEFINE_UNQUOTED(HAVE_DECL_LN_GET_HELIOCENTRIC_TIME_DIFF, 1, [If Libnova provides ln_get_heliocentric_time_diff function])
])

AH_TEMPLATE([HAVE_DECL_LN_GET_ALT_FROM_AIRMASS],[If Libnova provides ln_get_alt_from_airmass function])

AC_CHECK_DECL(ln_get_alt_from_airmass, , ,[#include <libnova/libnova.h>])
AS_IF([test "x$ac_cv_have_decl_ln_get_alt_from_airmass" = "xyes"], [
	AC_DEFINE_UNQUOTED(HAVE_DECL_LN_GET_ALT_FROM_AIRMASS, 1, [If Libnova provides ln_get_alt_from_airmass function])
])

AS_IF([test "x"$xfocusc = "xyes"], [
	AC_PATH_XTRA
	AC_SUBST([RTS2_XFOCUSC], ['rts2-xfocusc${EXEEXT}'])
])

AM_CONDITIONAL(GRAPHICSMAGICK, test x$MAGIC_LIBS != x)

AH_TEMPLATE([HAVE_CERN_ROOT],[If CERN ROOT package is installed])

AS_IF([test "x${root_config}" != "xno"], [
  LIB_ROOT=`${root_config} --libs`
  ROOT_CXXFLAGS="-I `${root_config} --incdir`"
  AC_DEFINE_UNQUOTED(HAVE_CERN_ROOT,1,[If CERN ROOT package is installed])
], [
  LIB_ROOT=""
  ROOT_CXXFLAGS=""
])
AC_SUBST(LIB_ROOT)
AC_SUBST(ROOT_CXXFLAGS)

AM_CONDITIONAL(CERN_ROOT, test x$root_config != xno)

AC_CHECK_LIB([archive],[archive_write_data], LIBARCHIVE_LIBS="-larchive", LIBARCHIVE_CFLAGS="")
AC_SUBST(LIBARCHIVE_LIBS)
AC_SUBST(LIBARCHIVE_CFLAGS)

AH_TEMPLATE([HAVE_LIBARCHIVE],[If libarchive is present])

AS_IF([test "x$LIBARCHIVE_LIBS" != "x"], [
	AC_DEFINE_UNQUOTED([HAVE_LIBARCHIVE],1,[If libarchive is present])
	libarchive="yes"
], [
	libarchive="no"
])

AS_IF([test "x$COMEDI" != "xno"], [
	AC_CHECK_LIB([comedi], [comedi_open], LIB_COMEDI="-lcomedi";
	AC_SUBST(LIB_COMEDI), [cat << EOF
#**** You don't have comedi library installed.
**** Please download it from http://www.comedi.org
EOF
	exit 1])
])

AS_IF([test "x$epics" != "xno"], [
	AS_IF([test `uname` == 'Linux'], [
		AC_CHECK_LIB([readline],[readline],,[cat << EOF
**** You do not have readline library. For some reasons, it is
**** needed if you build with EPICS. Please install readline-dev package.
EOF
			exit 1])
	])
  	AS_IF([test "x$epics" != "xyes"], [
		AS_IF([test "x$EPICS_HOST_ARCH" = "x"], [
		  	cat << EOF
**** Please define EPICS_HOST_ARCH varible. It content should be same as
**** the variable you used when you build EPICS library.
EOF
			exit 1
		])
		LDFLAGS="${LDFLAGS} -L${epics}/lib/${EPICS_HOST_ARCH}"
		LIB_EPICS="-L${epics}/lib/${EPICS_HOST_ARCH}"
		AS_IF([echo "$EPICS_HOST_ARCH" | grep ^linux], [
			INC_EPICS="-I${epics}/include -I${epics}/include/os/Linux"
		], [
			AS_IF([echo "$EPICS_HOST_ARCH" | grep ^solaris], [
				INC_EPICS="-I${epics}/include -I${epics}/include/os/solaris"
			], [
			  	echo "**** Unsupported EPICS_HOST_ARCH argument ${EPICS_HOST_ARCH}"
				exit 1
			])
		])
	], [
		LIB_EPICS=""
		INC_EPICS=""
	])
	AC_CHECK_LIB([ca], [ca_context_create],
	LIB_EPICS="-lca ${LIB_EPICS}",
	[cat << EOF
**** You don't have epics library installed.
**** Please try to get it from http://www.aps.anl.gov/epics
EOF
	exit 1])

	AC_SUBST(LIB_EPICS)
	AC_SUBST(INC_EPICS)
])

AH_TEMPLATE([HAVE_CRYPT],[If crypt library is installed])

AC_CHECK_LIB([crypt], [crypt],
[LIB_CRYPT="-lcrypt"
AC_SUBST(LIB_CRYPT)
AC_DEFINE_UNQUOTED([HAVE_CRYPT],1,[If crypt library is installed])],
[cat << EOF
**** You don't have crypt library.
**** All passwords will be stored in plaintext
EOF
LIB_CRYPT=""
])

AC_MSG_CHECKING(for build date)
DATE=`date +%Y-%m-%d`
AS_IF([test "z"$DATE = "z"], [
	AC_MSG_RESULT(none found)
	DATE="unknow"
], [
	AC_MSG_RESULT($DATE)
])

LDADD=""
AC_SUBST(LDADD)
AC_SUBST(LDFLAGS)

AC_SUBST(DATE)

LIBCURL_CHECK_CONFIG()

debug=""
AC_SUBST(debug)

AC_SNMP

AH_TEMPLATE([HAVE_PGSQL],[Whenever PGSQL is installed])
AH_TEMPLATE([HAVE_PGSQL_8], [If PGSQL version is 8 or higher])
AH_TEMPLATE([HAVE_PGSQL_9], [If PGSQL version is 9 or higher])
AH_TEMPLATE([HAVE_ECPG_9], [If ECPG version is 9 or higher])
AH_TEMPLATE([HAVE_PGSQL_SET_VARSIZE], [If PGSQL provides SET_VARSIZE function])

AS_IF([test "x"$psql = xyes], [
  AC_POSTGRESQL
  AC_ECPG

  PG_MAJOR=`$PG_CONFIG --version | sed 's/PostgreSQL //' | sed 's/\..*$//'`
  PG_MINOR=`$PG_CONFIG --version | sed 's/PostgreSQL [[0-9]]*\.//' | sed 's/\..*$//'`

  AC_MSG_CHECKING(for PostgreSQL version)
  AC_MSG_RESULT(${PG_MAJOR}.${PG_MINOR})

  ECPG_MAJOR=`$ECPG --version | sed 's/ecpg \(([^\)]*)\) //' | sed 's/\..*$//'`
  ECPG_MINOR=`$ECPG --version | sed 's/ecpg \(([^)]*)\) [0-9]?\.//' | sed 's/\..*$//'`

  AC_MSG_CHECKING(for ECPG version)
  AC_MSG_RESULT(${ECPG_MAJOR}.${ECPG_MINOR})

  AC_DEFINE_UNQUOTED(HAVE_PGSQL, 1, [Whenever PGSQL is installed])

  AS_IF([test $PG_MAJOR -gt 7], [
    AC_DEFINE_UNQUOTED(HAVE_PGSQL_8, 1, [If PGSQL version is 8 or higher])
  ])
  AS_IF([test $PG_MAJOR -gt 8], [
    AC_DEFINE_UNQUOTED(HAVE_PGSQL_9, 1, [If PGSQL version is 9 or higher])
    AC_DEFINE_UNQUOTED(HAVE_PGSQL_SET_VARSIZE, 1, [If PGSQL provides SET_VARSIZE function])
  ])
  AS_IF([test $ECPG_MAJOR -gt 8], [
    AC_DEFINE_UNQUOTED(HAVE_ECPG_9, 1, [If ECPG version is 9 or higher])
  ])
  AS_IF([test $PG_MAJOR -eq 8 -a $PG_MINOR -gt 2], [
    AC_DEFINE_UNQUOTED(HAVE_PGSQL_SET_VARSIZE, 1, [If PGSQL provides SET_VARSIZE function])
  ])
], [
  AC_MSG_RESULT(no)
])

AM_CONDITIONAL(PGSQL, test x$psql = xyes)

DB2MAN=no
DB2FOP=no

AC_ARG_WITH(docbook,
[  --with-docbook         path DocBook stylesheets (needed for manual page generation)],
docbook="${withval}";
DB2MAN=$docbook/manpages/docbook.xsl;
DB2FOP=$docbook/fo/docbook.xsl,
[docbook=detect])

AS_IF([test "x$docbook" == 'xdetect'], [
	# Ubuntu docbook localization
	AC_CHECK_FILE([/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/manpages/docbook.xsl],DB2MAN=/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/manpages/docbook.xsl,[])
	AC_CHECK_FILE([/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/fo/docbook.xsl],DB2FOP=/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/fo/docbook.xsl,[])

	# Fedore docbook localization
	AC_CHECK_FILE([/usr/share/sgml/docbook/xsl-stylesheets/manpages/docbook.xsl],DB2MAN=/usr/share/sgml/docbook/xsl-stylesheets/manpages/docbook.xsl,[])
	AC_CHECK_FILE([/usr/share/sgml/docbook/xsl-stylesheets/manpages/fo/docbook.xsl],DB2FOP=/usr/share/sgml/docbook/xsl-stylesheets/manpages/fo/docbook.xsl,[])

	# SUSE docbook localization
	AC_CHECK_FILE([/usr/share/xml/docbook/stylesheet/nwalsh/current/manpages/docbook.xsl],DB2MAN=/usr/share/xml/docbook/stylesheet/nwalsh/current/manpages/docbook.xsl,[])
	AC_CHECK_FILE([/usr/share/xml/docbook/stylesheet/nwalsh/current/fo/docbook.xsl],DB2FOP=/usr/share/xml/docbook/stylesheet/nwalsh/current/manpages/fo/docbook.xsl,[])
], [
	AS_IF([test "x$docbook" != "xno"], [
		AS_IF([test x$DB2MAN != "xno"], [
			AC_CHECK_FILE([$DB2MAN],[],exit)
		])

		AS_IF([test x$DB2FOP != "xno"], [
			AC_CHECK_FILE([$DB2FOP],[],exit)
		])
	])
])

AC_ARG_WITH(a2x,
[  --with-a2x             path to a2x executable],
a2x="${withval}",
a2x="a2x"
)

AC_CHECK_PROG(A2X, "$a2x", "$a2x", "no")

AC_SUBST(DB2MAN)
AC_SUBST(DB2FOP)
AC_SUBST(A2X)

AM_CONDITIONAL(HAVE_DB2MAN, test x$DB2MAN != xno)
AM_CONDITIONAL(HAVE_DB2FOP, test x$DB2FOP != xno)
AM_CONDITIONAL(HAVE_A2X, test x$A2X != xno)

AC_ARG_WITH(aspell,
[  --with-aspell           path to aspell executable],
aspell="${withval}",
aspell="aspell"
)

AC_CHECK_PROG(ASPELL, "$aspell", "$aspell", "no")

AM_CONDITIONAL(HAVE_ASPELL, test x$ASPELL != xno)

AC_CONFIG_SRCDIR(src)
AM_CONFIG_HEADER(config.h)
AX_PREFIX_CONFIG_H(include/rts2-config.h)

AM_CONDITIONAL(NOT_GETADDRINFO, test x${ac_cv_func_getaddrinfo} = xno)
AM_CONDITIONAL(NOT_GETOPT_LONG, test x${ac_cv_func_getopt_long} = xno)
AM_CONDITIONAL(SUNOS, test x${target_os} = xsolaris2.8)
AM_CONDITIONAL(CYGWIN, test x${target_os} = xcygwin)
AM_CONDITIONAL(MACOSX, test x${target_os/darwin} != x${target_os})
AM_CONDITIONAL(SUNCYGMAC, test x${target_os} = xcygwin -o x${target_os} = xsolaris2.8 -o x${target_os/darwin} != x${target_os})
AM_CONDITIONAL(ARMCPU, test x${target_cpu/arm} != x${target_cpu} -o x${target_cpu/aarch64} != x${target_cpu})
AM_CONDITIONAL(ARM64CPU, test x${target_cpu} != xaarch64)

AC_OUTPUT([Makefile
	   rts2-init
	   rts2.initd
	   man/Makefile
	   man/rts2.ent
	   include/Makefile
	   include/connection/Makefile
	   include/rts2db/Makefile
	   include/rts2fits/Makefile
	   include/rts2lx200/Makefile
	   include/rts2script/Makefile
	   include/rts2scheduler/Makefile
	   include/rts2json/Makefile
	   include/vermes/Makefile
	   include/xmlrpc++/Makefile
	   include/pluto/Makefile
	   include/sep/Makefile
	   include/ucac5/Makefile
	   include/gtp/Makefile
	   conf/Makefile
	   conf/event.d/Makefile
	   conf/rts2.service
	   conf/rts2@.service
	   conf/logrotate/Makefile
	   conf/logrotate/logrotate-rts2.conf
	   contrib/Makefile
	   doc/Makefile
	   doc/doxyfile
	   lib/Makefile
	   lib/xmlrpc++/Makefile
	   lib/pluto/Makefile
	   lib/sgp4/Makefile
	   lib/rts2/Makefile
	   lib/rts2/vermes/Makefile
	   lib/rts2fits/Makefile
	   lib/rts2db/Makefile
	   lib/rts2lx200/Makefile
	   lib/rts2script/Makefile
	   lib/rts2scheduler/Makefile
	   lib/rts2json/Makefile
	   lib/rts2tel/Makefile
	   lib/libindi/Makefile
	   lib/sep/Makefile
	   lib/ucac5/Makefile
	   lib/gtp/Makefile
	   python/Makefile
	   python/rts2/Makefile
	   python/rts2/db/Makefile
	   scripts/Makefile
	   src/Makefile
	   src/axisd/Makefile
	   src/bckimages/Makefile
	   src/camd/Makefile
	   src/camd/urvc2/Makefile
	   src/camd/apogee/Makefile
	   src/camd/edtsao/Makefile
	   src/camd/si8821/Makefile
	   src/camd/sxccd/Makefile
	   src/dome/Makefile
	   src/dome/zelio/Makefile
	   src/filterd/Makefile
	   src/focusc/Makefile
	   src/focusd/Makefile
	   src/grb/Makefile
	   src/mirror/Makefile
	   src/monitor/Makefile
	   src/multidevd/Makefile
	   src/plan/Makefile
	   src/sql/Makefile
	   src/sql/auger/Makefile
	   src/sql/create/Makefile
	   src/sql/data/Makefile
	   src/sql/drop/Makefile
	   src/sql/update/Makefile
	   src/sql/telma/Makefile
	   src/sql/phot/Makefile
	   src/sql/bb/Makefile
	   src/pgsql/Makefile
	   src/phot/Makefile
	   src/centrald/Makefile
	   src/teld/Makefile
	   src/teld/irait/Makefile
	   src/db/Makefile
	   src/sensord/Makefile
	   src/rotad/Makefile
	   src/logger/Makefile
	   src/httpd/Makefile
	   src/wsd/Makefile
	   src/scheduler/Makefile
	   src/bb/Makefile
	   src/pluto/Makefile
	   src/catd/Makefile
	   src/thrift/Makefile
	   src/redis/Makefile
	   src/ucac5/Makefile
	   tests/Makefile
	   checks/Makefile
	   checks/data/Makefile
	   scripts/rts2saf/Makefile
	   scripts/rts2saf/rts2saf/Makefile]
)

echo "
****************************************************************************
 RTS2 build configuration for ${target_os} on ${build_os}, CPU ${target_cpu}

  focusing      ${focusing}
  xfocusc       ${xfocusc}
  psql          ${psql}
  libnova       ${libnova}
  libusb1	${libusb1}
  ncurses       ${NCURSES_CFLAGS} ${NCURSES_LIBS}
  cfitsio       ${CFITSIO_CFLAGS} ${CFITSIO_LIBS}
  libwcstools   ${libwcstools}
  pgsql		${psql}
  gpib          ${GPIBLIB}
  comedilib     ${COMEDI}
  epics         ${epics}
  libxml2       ${XMLVERS}
  graphicsmagic ${MAGIC_CFLAGS} ${MAGIC_LIBS}
  CERN ROOT     ${ROOT_VERS}
  libarchive    ${libarchive}
  crypt         ${LIB_CRYPT}
  libgjson	${JSONGLIB_CFLAGS} ${JSONGLIB_LIBS}
  openssl       ${ssl}
  libcheck	${libcheck}
  libjsoncpp	${libjsoncpp}
  libwebsockets	${libwebsockets}
  libthrift     ${libthrift} ${THRIFT_CFLAGS} ${THRIFT_LIBS}
  hiredis	${hiredis}
  liberfa	${liberfa} ${ERFA_CFLAGS} ${ERFA_LIBS}
  snmp          ${SNMP_CONFIG}

CCDs:
  alta          ${ALTA}
  andor         ${ANDOR}
  arc           ${ARC}
  arc-1.7       ${ARC_1_7}
  edtpdv        ${EDTPDV}
  fli           ${FLI}
  miccd         ${MICCD}
  gxccd         ${GXCCD}
  sbig          ${SBIG}
  sbig include  ${SBIG_INC}
  sbig lib      ${SBIG_LIB}
  SI3097        ${SI3097}
  ASI           ${ASI}
  princeton     ${PRINCETON}

MOUNTs:
  paralib       ${PARALIB}
  mdmtcs        ${MDMTCS}

DOMEs:
  PCI8255       ${PCI8255}

SENSORs:
  A3200         ${A3200}
  BOLTWOOD      ${BOLTWOOD}
  BIGNG         ${BIGNG}
  K8055         ${K8055}

  prefix        ${prefix}
  exec_prefix   ${exec_prefix}
  bindir        ${bindir}
  confdir	${CONFDIR}
  lock prefix   ${LOCK_PREFIX}
  log file      ${LOG_FILE}
  centrald port	${CENTRALD_PORT}

Manpages, docbook stylesheets:
  A2X           ${A2X}
  ASPELL        ${ASPELL}
  DB2MAN        ${DB2MAN}
  DB2FOP        ${DB2FOP}

***************************************

Now type 'make' followed by 'make install'
to install rts2 package.

Please note, that you should do 'make clean'
if you change any configuration option.

"
