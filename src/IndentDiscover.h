#ifndef INDENTDISCOVER_H
#define INDENTDISCOVER_H

#include <QByteArray>
#include <QStringList>

class QString;

namespace IndentDiscover
{
    enum Eol {
        UndefinedEol = 0x0,
        Unix = 0x1,
        DOS = 0x2,
        MacOS = 0x4
    };
    
    enum Indent {
        UndefinedIndent = 0x0,
        Tabs = 0x1,
        Spaces = 0x2,
        Mixede = Tabs | Spaces
    };
    
    static IndentDiscover::Eol defaultEol = IndentDiscover::Unix;
    static IndentDiscover::Indent defaultIndent = IndentDiscover::Spaces;
    static int defaultIndentWidth = 4;
    static int defaultTabWidth = 4;
    
    struct GuessedProperties {
        typedef QList<GuessedProperties> List;
        
        GuessedProperties();
        
        int eol; // Eol flags
        int indent; // Indent flags
        int indentWidth; // indent size in spaces
        int tabWidth; // tab size in spaces
    };
    
    GuessedProperties guessDataProperties( const QByteArray& data, const QByteArray& codec = QByteArray( "UTF-8" ) );
    GuessedProperties guessFileProperties( const QString& filePath, const QByteArray& codec = QByteArray( "UTF-8" ) );
    GuessedProperties::List guessFilesProperties( const QStringList& filePaths, const QByteArray& codec = QByteArray( "UTF-8" ) );
};

#endif // INDENTDISCOVER_H
