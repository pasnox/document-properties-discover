#include "IndentDiscover.h"

#include <QString>
#include <QRegExp>
#include <QTextCodec>
#include <QFile>
#include <QHash>
#include <QDebug>

QTextCodec* textCodec( const QByteArray& name, const QByteArray& data = QByteArray() )
{
    QTextCodec* codec = QTextCodec::codecForName( name );
    
    if ( !codec ) {
        codec = QTextCodec::codecForUtfText( data, QTextCodec::codecForLocale() );
    }
    
    return codec;
}

namespace IndentDiscover {
    enum LineType {
        NoIndent,
        SpaceOnly,
        TabOnly,
        Mixed,
        BeginSpace
    };
    
    QHash<QString, int> lines;
    QHash<IndentDiscover::Eol, int> eols;
    int nb_processed_lines;
    int nb_indent_hint;
    QRegExp indent_re;
    QRegExp mixed_re;
    bool skip_next_line;
    void* previous_line_info;
    
    void clear() {
        IndentDiscover::lines.clear();
        IndentDiscover::eols.clear();
        
        for ( int i = 2; i < 9; i++ ) {
            IndentDiscover::lines[ QString( "space%1" ).arg( i ) ] = 0;
            IndentDiscover::lines[ QString( "mixed%1" ).arg( i ) ] = 0;
        }
        
        IndentDiscover::lines[ "tab" ] = 0;
        
        IndentDiscover::nb_processed_lines = 0;
        IndentDiscover::nb_indent_hint = 0;
        
        IndentDiscover::indent_re = QRegExp( "^([ \t]+)([^ \t]+)" );
        IndentDiscover::mixed_re = QRegExp( "^(\t+)( +)$" );
        
        IndentDiscover::skip_next_line = false;
        IndentDiscover::previous_line_info = 0;
    }
    
    QByteArray readDataLine( int& offset, const QByteArray& data ) {
        const int start = offset;
        const int length = data.length();
        
        // read chars until end of line
        for ( int i = offset; i < length; i++ ) {
            const char& c = data[ i ];
            
            switch ( c ) {
                // maybe macos eol or dos eol
                case '\r':
                    // chars after
                    if ( i < length -1 -1 ) {
                        // dos eol
                        if ( data[ i +1 ] == '\n' ) {
                            i++; // we read one more char
                            IndentDiscover::eols[ IndentDiscover::DOS ]++;
                        }
                        // macos eol
                        else {
                            IndentDiscover::eols[ IndentDiscover::MacOS ]++;
                        }
                    }
                    // ending char, macos eol
                    else {
                        IndentDiscover::eols[ IndentDiscover::MacOS ]++;
                    }
                    break;
                // unix eol
                case '\n':
                    IndentDiscover::eols[ IndentDiscover::Unix ]++;
                    break;
                // continue to read
                default:
                    continue;
            }
            
            offset = i +1; // +1 to take eol
            break;
        }
        
        return data.mid( start, offset++ -start ); // offset++ so next call start after current offset
    }
    
    void analyzeLine( const QByteArray& line ) {
    }
    
    void parseData( const QByteArray& data ) {
        int offset = 0;
        
        QByteArray line = IndentDiscover::readDataLine( offset, data );
        
        while( !line.isNull() ) {
            qWarning() << "---Line" << line;
            analyzeLine( line );
            line = IndentDiscover::readDataLine( offset, data );
        }
        
        qWarning() << eols /*<< lines */<< data.count( "\n" );
    }
}

IndentDiscover::GuessedProperties::GuessedProperties()
{
    eol = IndentDiscover::defaultEol;
    indent = IndentDiscover::defaultIndent;
    indentWidth = IndentDiscover::defaultIndentWidth;
    tabWidth = IndentDiscover::defaultTabWidth;
}

IndentDiscover::GuessedProperties IndentDiscover::guessDataProperties( const QByteArray& data, const QByteArray& _codec )
{
    IndentDiscover::GuessedProperties properties;
    QTextCodec* codec = textCodec( _codec, data );
    const QString content = codec->toUnicode( data );
    
    IndentDiscover::clear();
    IndentDiscover::parseData( data );
    
    return properties;
}

IndentDiscover::GuessedProperties IndentDiscover::guessFileProperties( const QString& filePath, const QByteArray& codec )
{
    QFile file( filePath );
    
    if ( !file.exists() || !file.open( QIODevice::ReadOnly ) ) {
        return IndentDiscover::GuessedProperties();
    }
    
    return IndentDiscover::guessDataProperties( file.readAll(), codec );
}

IndentDiscover::GuessedProperties::List IndentDiscover::guessFilesProperties( const QStringList& filePaths, const QByteArray& codec )
{
    IndentDiscover::GuessedProperties::List propertiesList;
    
    foreach ( const QString& filePath, filePaths ) {
        propertiesList << IndentDiscover::guessFileProperties( filePath, codec );
    }
    
    return propertiesList;
}
