###########################################################################################
##      Created using Monkey Studio IDE v1.9.0.1 (1.9.0.1)
##
##  Author    : You Name <your@email.org>
##  Project   : document-properties-discover
##  FileName  : document-properties-discover.pro
##  Date      : 2012-05-27T13:18:50
##  License   : GPL
##  Comment   : Creating using Monkey Studio RAD
##  Home Page : https://github.com/pasnox/document-properties-discover
##
##  This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
##  WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
###########################################################################################

XUP.QT_VERSION = Qt System (4.8.1)
XUP.OTHERS_PLATFORM_TARGET_DEFAULT = bin/Linux/document-properties-discover_debug

include( config.pri )
initializeProject( app, $${BUILD_TARGET}, $${BUILD_MODE}, $${BUILD_PATH}/$${TARGET_NAME}, $${BUILD_TARGET_PATH}, "" )

INCLUDEPATH *= $$getFolders( "." )
DEPENDPATH *= $${INCLUDEPATH}

HEADERS *= src/DocumentPropertiesDiscover.h

SOURCES *= src/main.cpp \
    src/DocumentPropertiesDiscover.cpp