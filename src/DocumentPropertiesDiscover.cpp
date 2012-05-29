#include "DocumentPropertiesDiscover.h"

#include <QString>
#include <QRegExp>
#include <QTextCodec>
#include <QFile>
#include <QHash>
#include <QDebug>

#define PRINT_OUTPUT false

namespace DocumentPropertiesDiscover {
    enum LineType {
        Null,
        NoIndent,
        SpaceOnly,
        TabOnly,
        Mixed,
        BeginSpace
    };
    
    struct LineInfo {
        LineInfo( DocumentPropertiesDiscover::LineType _first = DocumentPropertiesDiscover::Null, const QString& _second = QString::null, const QString& _third = QString::null ) {
            first = _first;
            second = _second;
            third = _third;
        }
        
        bool operator==( const DocumentPropertiesDiscover::LineInfo& other ) const {
            return
                first == other.first &&
                second == other.second &&
                third == other.third
            ;
        }
        
        DocumentPropertiesDiscover::LineType first;
        QString second;
        QString third;
    };
    
    DocumentPropertiesDiscover::GuessedProperties defaultGuessedProperties( int eol ) {
        return DocumentPropertiesDiscover::GuessedProperties( eol, DocumentPropertiesDiscover::defaultIndent(), DocumentPropertiesDiscover::defaultIndentWidth(), DocumentPropertiesDiscover::defaultTabWidth() );
    }
    
    DocumentPropertiesDiscover::Eol _defaultEol = DocumentPropertiesDiscover::UnixEol;
    DocumentPropertiesDiscover::Indent _defaultIndent = DocumentPropertiesDiscover::SpacesIndent;
    int _defaultIndentWidth = 4;
    int _defaultTabWidth = 4;
    
    QHash<QString, int> lines;
    QHash<DocumentPropertiesDiscover::Eol, int> eols;
    int nb_processed_lines;
    int nb_indent_hint;
    QRegExp indent_re;
    QRegExp mixed_re;
    bool skip_next_line;
    DocumentPropertiesDiscover::LineInfo previous_line_info;
    
    int linesMax( const QString& key ) {
        int value = -1;
        
        foreach ( const QString& k, DocumentPropertiesDiscover::lines.keys() ) {
            if ( k.startsWith( key ) ) {
                const int n = k.mid( key.length() ).toInt();
                
                if ( n >= 2 && n <= 8 ) {
                    value = qMax( value, DocumentPropertiesDiscover::lines[ k ] );
                }
            }
        }
        
        return value;
    }
    
    int eolMax() {
        int eol = DocumentPropertiesDiscover::UndefinedEol;
        int value = -1;
        
        foreach ( const DocumentPropertiesDiscover::Eol& key, DocumentPropertiesDiscover::eols.keys() ) {
            const int count = DocumentPropertiesDiscover::eols[ key ];
            
            if ( count > value ) {
                eol = key;
                value = count;
            }
        }
        
        if ( eol == DocumentPropertiesDiscover::UndefinedEol ) {
            eol = DocumentPropertiesDiscover::defaultEol();
        }
        
        return eol;
    }
    
    void clear() {
        DocumentPropertiesDiscover::lines.clear();
        DocumentPropertiesDiscover::eols.clear();
        
        DocumentPropertiesDiscover::nb_processed_lines = 0;
        DocumentPropertiesDiscover::nb_indent_hint = 0;
        
        DocumentPropertiesDiscover::indent_re = QRegExp( "^([ \t]+)([^ \t]+)" );
        DocumentPropertiesDiscover::mixed_re = QRegExp( "^(\t+)( +)$" );
        
        DocumentPropertiesDiscover::skip_next_line = false;
        DocumentPropertiesDiscover::previous_line_info = DocumentPropertiesDiscover::LineInfo();
    }
    
    DocumentPropertiesDiscover::GuessedProperties results() {
        const int max_line_space = DocumentPropertiesDiscover::linesMax( "space" );
        const int max_line_mixed = DocumentPropertiesDiscover::linesMax( "mixed" );
        const int max_line_tab = DocumentPropertiesDiscover::lines[ "tab" ];

        /*
        ### Result analysis
        #
        # 1. Space indented file
        #    - lines indented with less than 8 space will fill mixed and space array
        #    - lines indented with 8 space or more will fill only the space array
        #    - almost no lines indented with tab
        #
        # => more lines with space than lines with mixed
        # => more a lot more lines with space than tab
        #
        # 2. Tab indented file
        #    - most lines will be tab only
        #    - very few lines as mixed
        #    - very few lines as space only
        #
        # => a lot more lines with tab than lines with mixed
        # => a lot more lines with tab than lines with space
        #
        # 3. Mixed tab/space indented file
        #    - some lines are tab-only (lines with exactly 8 step indentation)
        #    - some lines are space only (less than 8 space)
        #    - all other lines are mixed
        #
        # If mixed is tab + 2 space indentation:
        #     - a lot more lines with mixed than with tab
        # If mixed is tab + 4 space indentation
        #     - as many lines with mixed than with tab
        #
        # If no lines exceed 8 space, there will be only lines with space
        # and tab but no lines with mixed. Impossible to detect mixed indentation
        # in this case, the file looks like it's actually indented as space only
        # and will be detected so.
        #
        # => same or more lines with mixed than lines with tab only
        # => same or more lines with mixed than lines with space only
        #
        */

        DocumentPropertiesDiscover::GuessedProperties result;

        // Detect space indented file
        if ( max_line_space >= max_line_mixed && max_line_space > max_line_tab ) {
            int nb = 0;
            int indent_value = -1;
            
            for ( int i = 8; i > 1; --i ) {
                // give a 10% threshold
                if ( DocumentPropertiesDiscover::lines[ QString( "space%1" ).arg( i ) ] > int( nb *1.1 ) ) {
                    indent_value = i;
                    nb = DocumentPropertiesDiscover::lines[ QString( "space%1" ).arg( indent_value ) ];
                }
            }

            // no lines
            if ( indent_value == -1 ) {
                result = DocumentPropertiesDiscover::defaultGuessedProperties( DocumentPropertiesDiscover::eolMax() );
            }
            else {
                result = DocumentPropertiesDiscover::GuessedProperties( DocumentPropertiesDiscover::eolMax(), DocumentPropertiesDiscover::SpacesIndent, indent_value );
            }
        }
        // Detect tab files
        else if ( max_line_tab > max_line_mixed && max_line_tab > max_line_space ) {
            result = DocumentPropertiesDiscover::GuessedProperties( DocumentPropertiesDiscover::eolMax(), DocumentPropertiesDiscover::TabsIndent, DocumentPropertiesDiscover::defaultIndentWidth(), DocumentPropertiesDiscover::defaultTabWidth() );
        }
        // Detect mixed files
        else if ( max_line_mixed >= max_line_tab && max_line_mixed > max_line_space ) {
            int nb = 0;
            int indent_value = -1;
            
            for ( int i = 8; i > 1; --i ) {
                // give a 10% threshold
                if ( DocumentPropertiesDiscover::lines[ QString( "mixed%1" ).arg( i ) ] > int( nb *1.1 ) ) {
                    indent_value = i;
                    nb = DocumentPropertiesDiscover::lines[ QString( "mixed%1" ).arg( indent_value ) ];
                }
            }

            // no lines
            if ( indent_value == -1 ) {
                result = DocumentPropertiesDiscover::defaultGuessedProperties( DocumentPropertiesDiscover::eolMax() );
            }
            else {
                result = DocumentPropertiesDiscover::GuessedProperties( DocumentPropertiesDiscover::eolMax(), DocumentPropertiesDiscover::MixedIndent, indent_value, 8 );
            }
        }
        // not enough information to make a decision
        else {
            result = DocumentPropertiesDiscover::defaultGuessedProperties( DocumentPropertiesDiscover::eolMax() );
        }

#if PRINT_OUTPUT
        qWarning( "Nb of scanned lines : %d", DocumentPropertiesDiscover::nb_processed_lines );
        qWarning( "Nb of indent hint : %d", DocumentPropertiesDiscover::nb_indent_hint );
        qWarning( "Collected data:" );
        
        foreach( const QString& key, DocumentPropertiesDiscover::lines.keys() ) {
            if ( DocumentPropertiesDiscover::lines[ key ] > 0 ) {
                qWarning( "%s: %d", qPrintable( key ), DocumentPropertiesDiscover::lines[ key ] );
            }
        }
        
        qWarning( "unix_eol: %d", DocumentPropertiesDiscover::eols[ DocumentPropertiesDiscover::UnixEol ] );
        qWarning( "dos_eol: %d", DocumentPropertiesDiscover::eols[ DocumentPropertiesDiscover::DOSEol ] );
        qWarning( "macos_eol: %d", DocumentPropertiesDiscover::eols[ DocumentPropertiesDiscover::MacOSEol ] );
        
        qWarning( "max_line_space: %d", max_line_space );
        qWarning( "max_line_mixed: %d", max_line_mixed );
        qWarning( "max_line_tab: %d", max_line_tab );
        
        qWarning( "Result: %s", qPrintable( result.toString() ) );
#endif
        return result;
    }
    
    DocumentPropertiesDiscover::LineInfo analyzeLineType( const QString& line ) {
        /*
        Analyse the type of line and return (LineType, <indentation part of the line>).

        The function will reject improperly formatted lines (mixture of tab
        and space for example) and comment lines.
        */
        
        bool mixed_mode = false;
        QString tab_part;
        QString space_part;
        
        if ( line.length() > 0 && !line.startsWith( ' ' ) && !line.startsWith( '\t' ) ) {
            return DocumentPropertiesDiscover::LineInfo( DocumentPropertiesDiscover::NoIndent, QString::null );
        }
        
        if ( DocumentPropertiesDiscover::indent_re.indexIn( line ) == -1 ) {
            return DocumentPropertiesDiscover::LineInfo();
        }
        
        const QString indent_part = DocumentPropertiesDiscover::indent_re.cap( 1 );
        const QString text_part = DocumentPropertiesDiscover::indent_re.cap( 2 );

        // continuation of a C/C++ comment, unlikely to be indented correctly
        if ( text_part.startsWith( "*" ) ) {
            return DocumentPropertiesDiscover::LineInfo();
        }

        // python, C/C++ comment, might not be indented correctly
        if ( text_part.startsWith( "/*" ) || text_part.startsWith( '#' ) ) {
            return DocumentPropertiesDiscover::LineInfo();
        }

        // mixed mode
        if ( indent_part.contains( "\t" ) && indent_part.contains( " " ) ) {
            // line is not composed of '\t\t\t    ', ignore it
            if ( !DocumentPropertiesDiscover::mixed_re.exactMatch( indent_part ) ) {
                return DocumentPropertiesDiscover::LineInfo();
            }
            
            mixed_mode = true;
            tab_part = DocumentPropertiesDiscover::mixed_re.cap( 1 );
            space_part = DocumentPropertiesDiscover::mixed_re.cap( 2 );
        }
        
        if ( mixed_mode ) {
            // this is not mixed mode, this is garbage !
            if ( space_part.length() >= 8 ) {
                return DocumentPropertiesDiscover::LineInfo();
            }
            
            return DocumentPropertiesDiscover::LineInfo( DocumentPropertiesDiscover::Mixed, tab_part, space_part );
        }

        if ( indent_part.contains( "\t" ) ) {
            return DocumentPropertiesDiscover::LineInfo( DocumentPropertiesDiscover::TabOnly, indent_part );
        }

        if ( indent_part.contains( " " ) ) {
            // this could be mixed mode too
            if ( indent_part.length() < 8 ) {
                return DocumentPropertiesDiscover::LineInfo( DocumentPropertiesDiscover::BeginSpace, indent_part );
            }
            // this is really a line indented with spaces
            else {
                return DocumentPropertiesDiscover::LineInfo( DocumentPropertiesDiscover::SpaceOnly, indent_part );
            }
        }

        qFatal( "%s: We should never get there !", Q_FUNC_INFO );
        return DocumentPropertiesDiscover::LineInfo();
    }
    
    QString analyzeLineIndentation( const QString& line ) {
        const DocumentPropertiesDiscover::LineInfo previous_line_info = DocumentPropertiesDiscover::previous_line_info;
        const DocumentPropertiesDiscover::LineInfo current_line_info = DocumentPropertiesDiscover::analyzeLineType( line );
        DocumentPropertiesDiscover::previous_line_info = current_line_info;

        if ( current_line_info == DocumentPropertiesDiscover::LineInfo() || previous_line_info == DocumentPropertiesDiscover::LineInfo() ) {
            return QString::null;
        }
        
        const QPair<DocumentPropertiesDiscover::LineType, DocumentPropertiesDiscover::LineType> t = qMakePair( previous_line_info.first, current_line_info.first );
        
        if ( t == qMakePair( DocumentPropertiesDiscover::TabOnly, DocumentPropertiesDiscover::TabOnly ) ||
            t == qMakePair( DocumentPropertiesDiscover::NoIndent, DocumentPropertiesDiscover::TabOnly ) ) {
            if ( current_line_info.second.length() -previous_line_info.second.length() == 1 ) {
                DocumentPropertiesDiscover::lines[ "tab" ]++;
                return "tab";
            }
        }
        else if ( t == qMakePair( DocumentPropertiesDiscover::SpaceOnly, DocumentPropertiesDiscover::SpaceOnly ) ||
            t == qMakePair( DocumentPropertiesDiscover::BeginSpace, DocumentPropertiesDiscover::SpaceOnly ) ||
            t == qMakePair( DocumentPropertiesDiscover::NoIndent, DocumentPropertiesDiscover::SpaceOnly ) ) {
            const int nb_space = current_line_info.second.length() -previous_line_info.second.length();
            
            if ( 1 < nb_space && nb_space < 8 ) {
                QString key = QString( "space%1" ).arg( nb_space );
                DocumentPropertiesDiscover::lines[ key ]++;
                return key;
            }
        }
        else if ( t == qMakePair( DocumentPropertiesDiscover::BeginSpace, DocumentPropertiesDiscover::BeginSpace ) ||
            t == qMakePair( DocumentPropertiesDiscover::NoIndent, DocumentPropertiesDiscover::BeginSpace ) ) {
            const int nb_space = current_line_info.second.length() -previous_line_info.second.length();
            
            if ( 1 < nb_space && nb_space < 8 ) {
                QString key1 = QString( "space%1" ).arg( nb_space );
                QString key2 = QString( "mixed%1" ).arg( nb_space );
                DocumentPropertiesDiscover::lines[ key1 ]++;
                DocumentPropertiesDiscover::lines[ key2 ]++;
                return key1;
            }
        }
        else if ( t == qMakePair( DocumentPropertiesDiscover::BeginSpace, DocumentPropertiesDiscover::TabOnly ) ) {
            // we assume that mixed indentation used 8 characters tabs
            if ( current_line_info.second.length() == 1 ) {
                // more than one tab on the line --> not mixed mode !
                const int nb_space = current_line_info.second.length() *8 -previous_line_info.second.length();
                
                if ( 1 < nb_space && nb_space < 8 ) {
                    QString key = QString( "mixed%1" ).arg( nb_space );
                    DocumentPropertiesDiscover::lines[ key ]++;
                    return key;
                }
            }
        }
        else if ( t == qMakePair( DocumentPropertiesDiscover::TabOnly, DocumentPropertiesDiscover::Mixed ) ) {
            if ( previous_line_info.second.length() == current_line_info.second.length() ) {
                const int nb_space = current_line_info.third.length();
                
                if ( 1 < nb_space && nb_space < 8 ) {
                    QString key = QString( "mixed%1" ).arg( nb_space );
                    DocumentPropertiesDiscover::lines[ key ]++;
                    return key;
                }
            }
        }
        else if ( t == qMakePair( DocumentPropertiesDiscover::Mixed, DocumentPropertiesDiscover::TabOnly ) ) {
            if ( previous_line_info.second.length() +1 == current_line_info.second.length() ) {
                const int nb_space = 8 -previous_line_info.third.length();
                
                if ( 1 < nb_space && nb_space < 8 ) {
                    QString key = QString( "mixed%1" ).arg( nb_space );
                    DocumentPropertiesDiscover::lines[ key ]++;
                    return key;
                }
            }
        }

        return QString::null;
    }
    
    QString analyzeLine( QString& line ) {
        if ( line.endsWith( "\n" ) ) {
            line.chop( 1 );
        }
        
        if ( line.endsWith( "\r" ) ) {
            line.chop( 1 );
        }
        
        DocumentPropertiesDiscover::nb_processed_lines++;
        const bool skip_current_line = DocumentPropertiesDiscover::skip_next_line;
        DocumentPropertiesDiscover::skip_next_line = false;
        
        // skip lines after lines ending in '\'
        if ( line.endsWith( '\\' ) ) {
            DocumentPropertiesDiscover::skip_next_line = true;
        }
        
        if ( skip_current_line ) {
            return QString::null;
        }
        
        const QString key = DocumentPropertiesDiscover::analyzeLineIndentation( line );
        
        if ( !key.isEmpty() ) {
            DocumentPropertiesDiscover::nb_indent_hint++;
        }
        
        return key;
    }
    
    QString readContentLine( int& offset, const QString& content, bool detectEol ) {
        const int start = offset;
        const int length = content.length();
        
        if ( offset == length ) {
            return QString::null;
        }
        
        // read chars until end of line
        for ( int i = offset; i < length; i++ ) {
            const char& c = content[ i ].toAscii();
            
            switch ( c ) {
                // maybe macos eol or dos eol
                case '\r':
                    // chars after
                    if ( i < length -1 -1 ) {
                        // dos eol
                        if ( content[ i +1 ] == '\n' ) {
                            i++; // we read one more char
                            if ( detectEol ) {
                                DocumentPropertiesDiscover::eols[ DocumentPropertiesDiscover::DOSEol ]++;
                            }
                        }
                        // macos eol
                        else {
                            if ( detectEol ) {
                                DocumentPropertiesDiscover::eols[ DocumentPropertiesDiscover::MacOSEol ]++;
                            }
                        }
                    }
                    // ending char, macos eol
                    else {
                        if ( detectEol ) {
                            DocumentPropertiesDiscover::eols[ DocumentPropertiesDiscover::MacOSEol ]++;
                        }
                    }
                    break;
                // unix eol
                case '\n':
                    if ( detectEol ) {
                        DocumentPropertiesDiscover::eols[ DocumentPropertiesDiscover::UnixEol ]++;
                    }
                    break;
                // continue to read
                default:
                    continue;
            }
            
            offset = i +1; // +1 to take eol
            break;
        }
        
        return content.mid( start, offset -start );
    }
    
    void parseContent( const QString& content, bool detectEol, bool detectIndent ) {
        int offset = 0;
        
        if ( !detectEol && !detectIndent ) {
            return;
        }
        
        QString line = DocumentPropertiesDiscover::readContentLine( offset, content, detectEol );
        
        while( !line.isNull() ) {
            if ( detectIndent ) {
                analyzeLine( line );
            }
            
            line = DocumentPropertiesDiscover::readContentLine( offset, content, detectEol );
        }
    }
}

// GuessedProperties

DocumentPropertiesDiscover::GuessedProperties::GuessedProperties()
{
    eol = DocumentPropertiesDiscover::defaultEol();
    indent = DocumentPropertiesDiscover::defaultIndent();
    indentWidth = DocumentPropertiesDiscover::defaultIndentWidth();
    tabWidth = DocumentPropertiesDiscover::defaultTabWidth();
}

DocumentPropertiesDiscover::GuessedProperties::GuessedProperties( int _eol, int _indent, int _indentWidth )
{
    eol = _eol;
    indent = _indent;
    indentWidth = _indentWidth;
    tabWidth = DocumentPropertiesDiscover::defaultTabWidth();
}

DocumentPropertiesDiscover::GuessedProperties::GuessedProperties( int _eol, int _indent, int _indentWidth, int _tabWidth )
{
    eol = _eol;
    indent = _indent;
    indentWidth = _indentWidth;
    tabWidth = _tabWidth;
}

bool DocumentPropertiesDiscover::GuessedProperties::operator==( const DocumentPropertiesDiscover::GuessedProperties& other ) const
{
    return
        eol == other.eol &&
        indent == other.indent &&
        indentWidth == other.indentWidth &&
        tabWidth == other.tabWidth
    ;
}

QString DocumentPropertiesDiscover::GuessedProperties::toString() const {
    switch ( indent ) {
        case DocumentPropertiesDiscover::SpacesIndent:
            return QString( "Spaces: tab %1 space %2 - Eol: %3" ).arg( tabWidth ).arg( indentWidth ).arg( eol );
        case DocumentPropertiesDiscover::TabsIndent:
            return QString( "Tabs: tab %1 space %2 - Eol: %3" ).arg( tabWidth ).arg( indentWidth ).arg( eol );
        case DocumentPropertiesDiscover::MixedIndent:
            return QString( "Mixed: tab %1 space %2 - Eol: %3" ).arg( tabWidth ).arg( indentWidth ).arg( eol );
        default:
            Q_ASSERT( 0 );
            return QString( "%1: tab %2 space %3 - Eol: %4" ).arg( indent ).arg( tabWidth ).arg( indentWidth ).arg( eol );
    }
}

// DocumentPropertiesDiscover

DocumentPropertiesDiscover::Eol DocumentPropertiesDiscover::defaultEol()
{
    return DocumentPropertiesDiscover::_defaultEol;
}

void DocumentPropertiesDiscover::setDefaultEol( DocumentPropertiesDiscover::Eol eol )
{
    DocumentPropertiesDiscover::_defaultEol = eol;
}

DocumentPropertiesDiscover::Indent DocumentPropertiesDiscover::defaultIndent()
{
    return DocumentPropertiesDiscover::_defaultIndent;
}

void DocumentPropertiesDiscover::setDefaultIndent( DocumentPropertiesDiscover::Indent indent )
{
    DocumentPropertiesDiscover::_defaultIndent = indent;
}

int DocumentPropertiesDiscover::defaultIndentWidth()
{
    return DocumentPropertiesDiscover::_defaultIndentWidth;
}

void DocumentPropertiesDiscover::setDefaultIndentWidth( int indentWidth )
{
    DocumentPropertiesDiscover::_defaultIndentWidth = indentWidth;
}

int DocumentPropertiesDiscover::defaultTabWidth()
{
    return DocumentPropertiesDiscover::_defaultTabWidth;
}

void DocumentPropertiesDiscover::setDefaultTabWidth( int tabWidth )
{
    DocumentPropertiesDiscover::_defaultTabWidth = tabWidth;
}

DocumentPropertiesDiscover::GuessedProperties DocumentPropertiesDiscover::guessContentProperties( const QString& content, bool detectEol, bool detectIndent )
{
    DocumentPropertiesDiscover::clear();
    DocumentPropertiesDiscover::parseContent( content, detectEol, detectIndent );
    return DocumentPropertiesDiscover::results();
}

DocumentPropertiesDiscover::GuessedProperties DocumentPropertiesDiscover::guessFileProperties( const QString& filePath, bool detectEol, bool detectIndent, const QByteArray& _codec )
{
    QFile file( filePath );
    
    if ( !file.exists() || !file.open( QIODevice::ReadOnly ) ) {
        return DocumentPropertiesDiscover::GuessedProperties();
    }
    
    const QByteArray data = file.readAll();
    QTextCodec* codec = QTextCodec::codecForName( _codec );
    
    if ( !codec ) {
        codec = QTextCodec::codecForUtfText( data, QTextCodec::codecForLocale() );
    }
    
    return DocumentPropertiesDiscover::guessContentProperties( codec->toUnicode( data ), detectEol, detectIndent );
}

DocumentPropertiesDiscover::GuessedProperties::List DocumentPropertiesDiscover::guessFilesProperties( const QStringList& filePaths, bool detectEol, bool detectIndent, const QByteArray& codec )
{
    DocumentPropertiesDiscover::GuessedProperties::List propertiesList;
    
    foreach ( const QString& filePath, filePaths ) {
        propertiesList << DocumentPropertiesDiscover::guessFileProperties( filePath, detectEol, detectIndent, codec );
    }
    
    return propertiesList;
}

void DocumentPropertiesDiscover::convertContent( QString& content, const DocumentPropertiesDiscover::GuessedProperties& from, const DocumentPropertiesDiscover::GuessedProperties& to, bool convertEol, bool convertIndent )
{
    if ( content.isEmpty() ) {
        return;
    }
    
    if ( !convertEol && !convertIndent ) {
        return;
    }
    
    if ( from == to ) {
        return;
    }
    
    QHash<QString, int> stringEol;
    QHash<int, QString> eolString;
    
    stringEol[ "\r\n" ] = DocumentPropertiesDiscover::DOSEol;
    stringEol[ "\n" ] = DocumentPropertiesDiscover::UnixEol;
    stringEol[ "\r" ] = DocumentPropertiesDiscover::MacOSEol;
    
    eolString[ DocumentPropertiesDiscover::DOSEol ] = "\r\n";
    eolString[ DocumentPropertiesDiscover::UnixEol ] = "\n";
    eolString[ DocumentPropertiesDiscover::MacOSEol ] = "\r";
    
    const QRegExp rx( "\\r\\n|\\n|\\r" );
    const QRegExp rx2( "\\w" );
    const int length = content.length();
    int lastPos = 0;
    int end = 0;
    int pos = 0;
    int indent = 0;
    int matchedLength;
    
    while ( ( pos = rx.indexIn( content, pos ) ) != -1 ) {
        matchedLength = rx.matchedLength();
        
        if ( convertEol ) {
            const QString lineEol = content.mid( pos, matchedLength );
            const int eol = stringEol.value( lineEol );
            
            if ( eol != to.eol ) {
                const QString neededEol = eolString.value( to.eol );
                content.replace( pos, matchedLength, neededEol );
                matchedLength = neededEol.length();
                qWarning() << "eol replaced" << eol << to.eol << " - " << lineEol.length() << neededEol.length();
            }
        }
        
        if ( convertIndent ) {
            int start = lastPos;
            end = pos;
            indent = rx2.indexIn( content, start ) -1;
            
            if ( indent > end ) {
                indent = end -1;
            }
            
            if ( start < indent && indent < pos && start != indent ) {
                const QString s = content.mid( start, end -start +matchedLength );
                
                //qWarning() << "Found indented line" << s.trimmed();
            }
            else {
                //qWarning() << "skip line with no indent";
            }
            
            lastPos = pos +matchedLength;
        }
        
        pos += matchedLength;
    }
}
