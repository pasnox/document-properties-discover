#include <QtGui>

#include "IndentDiscover.h"

int main( int argc, char** argv )
{
    QApplication app( argc, argv );
    app.setApplicationName( "indent-discover" );
    QObject::connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );

    const QString path = "/home/pasnox/Temporaire/indent_finder-1.4/test_files/mixed4";
    QDir dir( path );
    const QFileInfoList files = dir.entryInfoList( QDir::Files );
    int count = 0;
    
    foreach ( const QFileInfo& fi, files ) {
        qWarning() << fi.absoluteFilePath().toUtf8().constData();
        IndentDiscover::guessFileProperties( fi.absoluteFilePath() );
        
        count++;
        
        if ( count == 1 ) {
            break;
        }
    }
    
    return 0;//app.exec();
}
