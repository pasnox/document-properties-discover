#include <QtGui>

#include "IndentDiscover.h"

int main( int argc, char** argv )
{
    QApplication app( argc, argv );
    app.setApplicationName( "indent-discover" );
    QObject::connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );

    const QString path = "/home/pasnox/Temporaire/indent_finder-1.4/test_files/mixed4";
    const int indent = IndentDiscover::MixedIndent;
    QDir dir( path );
    const QFileInfoList files = dir.entryInfoList( QDir::Files );
    int count = 0;
    
    foreach ( const QFileInfo& fi, files ) {
        if ( fi.fileName() == "README" ) {
            continue;
        }
        
        IndentDiscover::GuessedProperties properties = IndentDiscover::guessFileProperties( fi.absoluteFilePath() );
        
        if ( properties.indent != indent ) {
            qWarning() << "Bad guessing" << fi.absoluteFilePath().toUtf8().constData();
            Q_ASSERT( properties.indent == indent );
        }
        
        count++;
        
        if ( count == 1 ) {
            //break;
        }
    }
    
    return 0;//app.exec();
}
