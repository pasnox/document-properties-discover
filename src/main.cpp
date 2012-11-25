#include <QtGui>

#include "DocumentPropertiesDiscover.h"

int main( int argc, char** argv )
{
    QApplication app( argc, argv );
    app.setApplicationName( "document-properties-discover" );
    QObject::connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );

    const QString path = QFileDialog::getExistingDirectory( 0, QString::null, QApplication::applicationDirPath() );
    QDir dir( path );
    const QFileInfoList files = dir.entryInfoList( QDir::Files );
    int count = 0;
    
    foreach ( const QFileInfo& fi, files ) {
        if ( fi.fileName() == "README" ) {
            continue;
        }
        
        DocumentPropertiesDiscover::GuessedProperties properties = DocumentPropertiesDiscover::guessFileProperties( fi.absoluteFilePath(), true, true );
        
        qWarning() << qPrintable( fi.fileName() ) << qPrintable( properties.toString() );
        
        /*const int indent = DocumentPropertiesDiscover::MixedIndent;
        if ( properties.indent != indent ) {
            qWarning() << "Bad guessing" << fi.absoluteFilePath().toUtf8().constData();
            Q_ASSERT( properties.indent == indent );
        }*/
        
        count++;
        
        if ( count == 1 ) {
            //break;
        }
    }
    
    return 0;//app.exec();
}
