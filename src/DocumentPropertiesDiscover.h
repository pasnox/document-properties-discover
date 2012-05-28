#ifndef DOCUMENTPROPERTIESDISCOVER_H
#define DOCUMENTPROPERTIESDISCOVER_H

#include <QByteArray>
#include <QStringList>

class QString;

namespace DocumentPropertiesDiscover
{
    enum Eol {
        UndefinedEol = 0x0,
        UnixEol = 0x1,
        DOSEol = 0x2,
        MacOSEol = 0x4
    };
    
    enum Indent {
        UndefinedIndent = 0x0,
        TabsIndent = 0x1,
        SpacesIndent = 0x2,
        MixedIndent = TabsIndent | SpacesIndent
    };
    
    struct GuessedProperties {
        typedef QList<GuessedProperties> List;
        
        GuessedProperties();
        GuessedProperties( int eol, int indent, int indentWidth );
        GuessedProperties( int eol, int indent, int tabWidth, int indentWidth );
        
        QString toString() const;
        
        int eol; // Eol flags
        int indent; // Indent flags
        int indentWidth; // indent size in spaces
        int tabWidth; // tab size in spaces
    };
    
    DocumentPropertiesDiscover::Eol defaultEol();// = DocumentPropertiesDiscover::UnixEol;
    void setDefaultEol( DocumentPropertiesDiscover::Eol eol );
    
    DocumentPropertiesDiscover::Indent defaultIndent();// = DocumentPropertiesDiscover::SpacesIndent;
    void setDefaultIndent( DocumentPropertiesDiscover::Indent indent );
    
    int defaultIndentWidth(); // = 4;
    void setDefaultIndentWidth( int indentWidth );
    
    int defaultTabWidth(); // = 4;
    void setDefaultTabWidth( int tabWidth );
    
    GuessedProperties guessContentProperties( const QString& content );
    GuessedProperties guessFileProperties( const QString& filePath, const QByteArray& codec = QByteArray( "UTF-8" ) );
    GuessedProperties::List guessFilesProperties( const QStringList& filePaths, const QByteArray& codec = QByteArray( "UTF-8" ) );
};

#endif // DOCUMENTPROPERTIESDISCOVER_H
