#ifndef DOCUMENTPROPERTIESDISCOVER_H
#define DOCUMENTPROPERTIESDISCOVER_H

#include <QByteArray>
#include <QStringList>
#include <QTime>
#include <QDebug>

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
        typedef QList<DocumentPropertiesDiscover::GuessedProperties> List;
        
        GuessedProperties();
        GuessedProperties( int eol, int indent, int indentWidth );
        GuessedProperties( int eol, int indent, int tabWidth, int indentWidth );
        
        bool operator==( const DocumentPropertiesDiscover::GuessedProperties& other ) const;
        bool operator!=( const DocumentPropertiesDiscover::GuessedProperties& other ) const;
        
        QString toString() const;
        
        static DocumentPropertiesDiscover::GuessedProperties null;
        
        int eol; // Eol flags
        int indent; // Indent flags
        int indentWidth; // indent size in spaces
        int tabWidth; // tab size in spaces
    };
    
    class TimeTracker : public QTime {
    public:
        TimeTracker( const QString& _name = QString::null ) {
            name = _name;
            start();
        }
        
        ~TimeTracker() {
            query();
        }
        
        void query( const QString& text = QString::null ) const {
            qWarning() << qPrintable( name ) << "Elapsed time:" << elapsed() /1000.0 << qPrintable( text );
        }
    
    protected:
        QString name;
    };
    
    DocumentPropertiesDiscover::Eol defaultEol();
    void setDefaultEol( DocumentPropertiesDiscover::Eol eol );
    
    DocumentPropertiesDiscover::Indent defaultIndent();
    void setDefaultIndent( DocumentPropertiesDiscover::Indent indent );
    
    int defaultIndentWidth();
    void setDefaultIndentWidth( int indentWidth );
    
    int defaultTabWidth();
    void setDefaultTabWidth( int tabWidth );
    
    DocumentPropertiesDiscover::GuessedProperties guessContentProperties( const QString& content, bool detectEol, bool detectIndent );
    DocumentPropertiesDiscover::GuessedProperties guessFileProperties( const QString& filePath, bool detectEol, bool detectIndent, const QByteArray& codec = QByteArray( "UTF-8" ) );
    DocumentPropertiesDiscover::GuessedProperties::List guessFilesProperties( const QStringList& filePaths, bool detectEol, bool detectIndent, const QByteArray& codec = QByteArray( "UTF-8" ) );
    
    void convertContent( QString& content, const DocumentPropertiesDiscover::GuessedProperties& from, const DocumentPropertiesDiscover::GuessedProperties& to, bool convertEol, bool convertIndent );
};

#endif // DOCUMENTPROPERTIESDISCOVER_H
