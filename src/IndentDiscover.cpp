#include "IndentDiscover.h"

#include <QString>
#include <QRegExp>
#include <QTextCodec>
#include <QFile>
#include <QHash>
#include <QDebug>

#define PRINT_OUTPUT false

namespace IndentDiscover {
    enum LineType {
        Null,
        NoIndent,
        SpaceOnly,
        TabOnly,
        Mixed,
        BeginSpace
    };
    
    struct LineInfo {
        LineInfo( IndentDiscover::LineType _first = IndentDiscover::Null, const QString& _second = QString::null, const QString& _third = QString::null ) {
            first = _first;
            second = _second;
            third = _third;
        }
        
        bool operator==( const IndentDiscover::LineInfo& other ) const {
            return
                first == other.first &&
                second == other.second &&
                third == other.third
            ;
        }
        
        IndentDiscover::LineType first;
        QString second;
        QString third;
    };
    
    IndentDiscover::GuessedProperties defaultGuessedProperties( int eol ) {
        return IndentDiscover::GuessedProperties( eol, IndentDiscover::defaultIndent, IndentDiscover::defaultIndentWidth, IndentDiscover::defaultTabWidth );
    }
    
    QHash<QString, int> lines;
    QHash<IndentDiscover::Eol, int> eols;
    int nb_processed_lines;
    int nb_indent_hint;
    QRegExp indent_re;
    QRegExp mixed_re;
    bool skip_next_line;
    IndentDiscover::LineInfo previous_line_info;
    
    int linesMax( const QString& key ) {
        int value = 0;
        
        foreach ( QString k, IndentDiscover::lines.keys() ) {
            if ( k.startsWith( key ) ) {
                const int n = k.mid( key.length() ).toInt();
                
                if ( n >= 2 && n <= 8 ) {
                    value = qMax( value, IndentDiscover::lines[ k ] );
                }
            }
        }
        
        return value;
    }
    
    int eolMax() {
        int eol = IndentDiscover::UndefinedEol;
        int value = -1;
        
        foreach ( const IndentDiscover::Eol& key, IndentDiscover::eols.keys() ) {
            const int count = IndentDiscover::eols[ key ];
            
            if ( count > value ) {
                eol = key;
                value = count;
            }
        }
        
        if ( eol == IndentDiscover::UndefinedEol ) {
            eol = IndentDiscover::defaultEol;
        }
        
        return eol;
    }
    
    void clear() {
        IndentDiscover::lines.clear();
        IndentDiscover::eols.clear();
        
        /*for ( int i = 2; i < 9; i++ ) {
            IndentDiscover::lines[ QString( "space%1" ).arg( i ) ] = 0;
            IndentDiscover::lines[ QString( "mixed%1" ).arg( i ) ] = 0;
        }
        
        IndentDiscover::lines[ "tab" ] = 0;*/
        
        IndentDiscover::nb_processed_lines = 0;
        IndentDiscover::nb_indent_hint = 0;
        
        IndentDiscover::indent_re = QRegExp( "^([ \t]+)([^ \t]+)" );
        IndentDiscover::mixed_re = QRegExp( "^(\t+)( +)$" );
        
        IndentDiscover::skip_next_line = false;
        IndentDiscover::previous_line_info = IndentDiscover::LineInfo();
    }
    
    IndentDiscover::GuessedProperties results() {
        const int max_line_space = IndentDiscover::linesMax( "space" );
        const int max_line_mixed = IndentDiscover::linesMax( "mixed" );
        const int max_line_tab = IndentDiscover::lines[ "tab" ];

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

        IndentDiscover::GuessedProperties result;

        // Detect space indented file
        if ( max_line_space >= max_line_mixed && max_line_space > max_line_tab ) {
            int nb = 0;
            int indent_value = -1;
            
            for ( int i = 8; i > 1; --i ) {
                // give a 10% threshold
                if ( IndentDiscover::lines[ QString( "space%1" ).arg( i ) ] > int( nb *1.1 ) ) {
                    indent_value = i;
                    nb = IndentDiscover::lines[ QString( "space%1" ).arg( indent_value ) ];
                }
            }

            // no lines
            if ( indent_value == -1 ) {
                result = IndentDiscover::defaultGuessedProperties( IndentDiscover::eolMax() );
            }
            else {
                result = IndentDiscover::GuessedProperties( IndentDiscover::eolMax(), IndentDiscover::SpacesIndent, indent_value );
            }
        }
        // Detect tab files
        else if ( max_line_tab > max_line_mixed && max_line_tab > max_line_space ) {
            result = IndentDiscover::GuessedProperties( IndentDiscover::eolMax(), IndentDiscover::TabsIndent, IndentDiscover::defaultIndentWidth, IndentDiscover::defaultTabWidth );
        }
        // Detect mixed files
        else if ( max_line_mixed >= max_line_tab && max_line_mixed > max_line_space ) {
            int nb = 0;
            int indent_value = -1;
            
            for ( int i = 8; i > 1; --i ) {
                // give a 10% threshold
                if ( IndentDiscover::lines[ QString( "mixed%1" ).arg( i ) ] > int( nb *1.1 ) ) {
                    indent_value = i;
                    nb = IndentDiscover::lines[ QString( "mixed%1" ).arg( indent_value ) ];
                }
            }

            // no lines
            if ( indent_value == -1 ) {
                result = IndentDiscover::defaultGuessedProperties( IndentDiscover::eolMax() );
            }
            else {
                result = IndentDiscover::GuessedProperties( IndentDiscover::eolMax(), IndentDiscover::MixedIndent, indent_value, 8 );
            }
        }
        // not enough information to make a decision
        else {
            result = IndentDiscover::defaultGuessedProperties( IndentDiscover::eolMax() );
        }

#if PRINT_OUTPUT
        qWarning( "Nb of scanned lines : %d", IndentDiscover::nb_processed_lines );
        qWarning( "Nb of indent hint : %d", IndentDiscover::nb_indent_hint );
        qWarning( "Collected data:" );
        
        foreach( const QString& key, IndentDiscover::lines.keys() ) {
            if ( IndentDiscover::lines[ key ] > 0 ) {
                qWarning( "%s: %d", qPrintable( key ), IndentDiscover::lines[ key ] );
            }
        }
        
        qWarning( "unix_eol: %d", IndentDiscover::eols[ IndentDiscover::UnixEol ] );
        qWarning( "dos_eol: %d", IndentDiscover::eols[ IndentDiscover::DOSEol ] );
        qWarning( "macos_eol: %d", IndentDiscover::eols[ IndentDiscover::MacOSEol ] );
        
        qWarning( "max_line_space: %d", max_line_space );
        qWarning( "max_line_mixed: %d", max_line_mixed );
        qWarning( "max_line_tab: %d", max_line_tab );
        
        qWarning( "Result: %s", qPrintable( result.toString() ) );
#endif
        return result;
    }
    
    IndentDiscover::LineInfo analyzeLineType( QString& line ) {
        /*
        Analyse the type of line and return (LineType, <indentation part of the line>).

        The function will reject improperly formatted lines (mixture of tab
        and space for example) and comment lines.
        */
        
        bool mixed_mode = false;
        QString tab_part;
        QString space_part;
        
        if ( line.length() > 0 && !line.startsWith( ' ' ) && !line.startsWith( '\t' ) ) {
            return IndentDiscover::LineInfo( IndentDiscover::NoIndent, QString::null );
        }
        
        if ( IndentDiscover::indent_re.indexIn( line ) == -1 ) {
            return IndentDiscover::LineInfo();
        }
        
        const QString indent_part = IndentDiscover::indent_re.cap( 1 );
        const QString text_part = IndentDiscover::indent_re.cap( 2 );

        // continuation of a C/C++ comment, unlikely to be indented correctly
        if ( text_part.startsWith( "*" ) ) {
            return IndentDiscover::LineInfo();
        }

        // python, C/C++ comment, might not be indented correctly
        if ( text_part.startsWith( "/*" ) || text_part.startsWith( '#' ) ) {
            return IndentDiscover::LineInfo();
        }

        // mixed mode
        if ( indent_part.contains( "\t" ) && indent_part.contains( " " ) ) {
            // line is not composed of '\t\t\t    ', ignore it
            if ( !IndentDiscover::mixed_re.exactMatch( indent_part ) ) {
                return IndentDiscover::LineInfo();
            }
            
            mixed_mode = true;
            tab_part = IndentDiscover::mixed_re.cap( 1 );
            space_part = IndentDiscover::mixed_re.cap( 2 );
        }
        
        if ( mixed_mode ) {
            // this is not mixed mode, this is garbage !
            if ( space_part.length() >= 8 ) {
                return IndentDiscover::LineInfo();
            }
            
            return IndentDiscover::LineInfo( IndentDiscover::Mixed, tab_part, space_part );
        }

        if ( indent_part.contains( "\t" ) ) {
            return IndentDiscover::LineInfo( IndentDiscover::TabOnly, indent_part );
        }

        if ( indent_part.contains( " " ) ) {
            // this could be mixed mode too
            if ( indent_part.length() < 8 ) {
                return IndentDiscover::LineInfo( IndentDiscover::BeginSpace, indent_part );
            }
            // this is really a line indented with spaces
            else {
                return IndentDiscover::LineInfo( IndentDiscover::SpaceOnly, indent_part );
            }
        }

        qFatal( "%s: We should never get there !", Q_FUNC_INFO );
        return IndentDiscover::LineInfo();
    }
    
    QString analyzeLineIndentation( QString& line ) {
        const IndentDiscover::LineInfo previous_line_info = IndentDiscover::previous_line_info;
        const IndentDiscover::LineInfo current_line_info = IndentDiscover::analyzeLineType( line );
        IndentDiscover::previous_line_info = current_line_info;

        if ( current_line_info == IndentDiscover::LineInfo() || previous_line_info == IndentDiscover::LineInfo() ) {
            return QString::null;
        }
        
        const QPair<IndentDiscover::LineType, IndentDiscover::LineType> t = qMakePair( previous_line_info.first, current_line_info.first );
        
        if ( t == qMakePair( IndentDiscover::TabOnly, IndentDiscover::TabOnly ) ||
            t == qMakePair( IndentDiscover::NoIndent, IndentDiscover::TabOnly ) ) {
            if ( current_line_info.second.length() -previous_line_info.second.length() == 1 ) {
                IndentDiscover::lines[ "tab" ]++;
                return "tab";
            }
        }
        else if ( t == qMakePair( IndentDiscover::SpaceOnly, IndentDiscover::SpaceOnly ) ||
            t == qMakePair( IndentDiscover::BeginSpace, IndentDiscover::SpaceOnly ) ||
            t == qMakePair( IndentDiscover::NoIndent, IndentDiscover::SpaceOnly ) ) {
            const int nb_space = current_line_info.second.length() -previous_line_info.second.length();
            
            if ( 1 < nb_space && nb_space < 8 ) {
                QString key = QString( "space%1" ).arg( nb_space );
                IndentDiscover::lines[ key ]++;
                return key;
            }
        }
        else if ( t == qMakePair( IndentDiscover::BeginSpace, IndentDiscover::BeginSpace ) ||
            t == qMakePair( IndentDiscover::NoIndent, IndentDiscover::BeginSpace ) ) {
            const int nb_space = current_line_info.second.length() -previous_line_info.second.length();
            
            if ( 1 < nb_space && nb_space < 8 ) {
                QString key1 = QString( "space%1" ).arg( nb_space );
                QString key2 = QString( "mixed%1" ).arg( nb_space );
                IndentDiscover::lines[ key1 ]++;
                IndentDiscover::lines[ key2 ]++;
                return key1;
            }
        }
        else if ( t == qMakePair( IndentDiscover::BeginSpace, IndentDiscover::TabOnly ) ) {
            // we assume that mixed indentation used 8 characters tabs
            if ( current_line_info.second.length() == 1 ) {
                // more than one tab on the line --> not mixed mode !
                const int nb_space = current_line_info.second.length() *8 -previous_line_info.second.length();
                
                if ( 1 < nb_space && nb_space < 8 ) {
                    QString key = QString( "mixed%1" ).arg( nb_space );
                    IndentDiscover::lines[ key ]++;
                    return key;
                }
            }
        }
        else if ( t == qMakePair( IndentDiscover::TabOnly, IndentDiscover::Mixed ) ) {
            if ( previous_line_info.second.length() == current_line_info.second.length() ) {
                const int nb_space = current_line_info.third.length();
                
                if ( 1 < nb_space && nb_space < 8 ) {
                    QString key = QString( "mixed%1" ).arg( nb_space );
                    IndentDiscover::lines[ key ]++;
                    return key;
                }
            }
        }
        else if ( t == qMakePair( IndentDiscover::Mixed, IndentDiscover::TabOnly ) ) {
            if ( previous_line_info.second.length() +1 == current_line_info.second.length() ) {
                const int nb_space = 8 -previous_line_info.third.length();
                
                if ( 1 < nb_space && nb_space < 8 ) {
                    QString key = QString( "mixed%1" ).arg( nb_space );
                    IndentDiscover::lines[ key ]++;
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
        
        IndentDiscover::nb_processed_lines++;
        const bool skip_current_line = IndentDiscover::skip_next_line;
        IndentDiscover::skip_next_line = false;
        
        // skip lines after lines ending in '\'
        if ( line.endsWith( '\\' ) ) {
            IndentDiscover::skip_next_line = true;
        }
        
        if ( skip_current_line ) {
            return QString::null;
        }
        
        const QString key = IndentDiscover::analyzeLineIndentation( line );
        
        if ( !key.isEmpty() ) {
            IndentDiscover::nb_indent_hint++;
        }
        
        return key;
    }
    
    QString readContentLine( int& offset, const QString& content ) {
        const int start = offset;
        const int length = content.length();
        
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
                            IndentDiscover::eols[ IndentDiscover::DOSEol ]++;
                        }
                        // macos eol
                        else {
                            IndentDiscover::eols[ IndentDiscover::MacOSEol ]++;
                        }
                    }
                    // ending char, macos eol
                    else {
                        IndentDiscover::eols[ IndentDiscover::MacOSEol ]++;
                    }
                    break;
                // unix eol
                case '\n':
                    IndentDiscover::eols[ IndentDiscover::UnixEol ]++;
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
    
    void parseContent( const QString& content ) {
        int offset = 0;
        
        QString line = IndentDiscover::readContentLine( offset, content );
        
        while( !line.isNull() ) {
            analyzeLine( line );
            line = IndentDiscover::readContentLine( offset, content );
        }
    }
}

IndentDiscover::GuessedProperties::GuessedProperties()
{
    eol = IndentDiscover::defaultEol;
    indent = IndentDiscover::defaultIndent;
    indentWidth = IndentDiscover::defaultIndentWidth;
    tabWidth = IndentDiscover::defaultTabWidth;
}

IndentDiscover::GuessedProperties::GuessedProperties( int _eol, int _indent, int _indentWidth )
{
    eol = _eol;
    indent = _indent;
    indentWidth = _indentWidth;
    tabWidth = IndentDiscover::defaultTabWidth;
}

IndentDiscover::GuessedProperties::GuessedProperties( int _eol, int _indent, int _indentWidth, int _tabWidth )
{
    eol = _eol;
    indent = _indent;
    indentWidth = _indentWidth;
    tabWidth = _tabWidth;
}

QString IndentDiscover::GuessedProperties::toString() const {
    switch ( indent ) {
        case IndentDiscover::SpacesIndent:
            return QString( "Spaces: tab %1 space %2 - Eol: %3" ).arg( tabWidth ).arg( indentWidth ).arg( eol );
        case IndentDiscover::TabsIndent:
            return QString( "Tabs: tab %1 space %2 - Eol: %3" ).arg( tabWidth ).arg( indentWidth ).arg( eol );
        case IndentDiscover::MixedIndent:
            return QString( "Mixed: tab %1 space %2 - Eol: %3" ).arg( tabWidth ).arg( indentWidth ).arg( eol );
        default:
            Q_ASSERT( 0 );
            return QString( "%1: tab %2 space %3 - Eol: %4" ).arg( indent ).arg( tabWidth ).arg( indentWidth ).arg( eol );
    }
}

IndentDiscover::GuessedProperties IndentDiscover::guessContentProperties( const QString& content )
{
    IndentDiscover::clear();
    IndentDiscover::parseContent( content );
    return IndentDiscover::results();
}

IndentDiscover::GuessedProperties IndentDiscover::guessFileProperties( const QString& filePath, const QByteArray& _codec )
{
    QFile file( filePath );
    
    if ( !file.exists() || !file.open( QIODevice::ReadOnly ) ) {
        return IndentDiscover::GuessedProperties();
    }
    
    const QByteArray data = file.readAll();
    QTextCodec* codec = QTextCodec::codecForName( _codec );
    
    if ( !codec ) {
        codec = QTextCodec::codecForUtfText( data, QTextCodec::codecForLocale() );
    }
    
    return IndentDiscover::guessContentProperties( codec->toUnicode( data ) );
}

IndentDiscover::GuessedProperties::List IndentDiscover::guessFilesProperties( const QStringList& filePaths, const QByteArray& codec )
{
    IndentDiscover::GuessedProperties::List propertiesList;
    
    foreach ( const QString& filePath, filePaths ) {
        propertiesList << IndentDiscover::guessFileProperties( filePath, codec );
    }
    
    return propertiesList;
}
