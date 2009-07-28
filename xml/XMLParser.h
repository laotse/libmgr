/*
 *
 * XML parser for XMLTree
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: XMLParser.h,v 1.5 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  XMLParser - the parser object
 *
 * This defines the values:
 *
 */

#ifndef _XML_XMLPARSER_H_
# define _XML_XMLPARSER_H_

#include <XMLTree.h>
#include <expat.h>

/*! \file XMLParser.cpp
    \brief DOM style XML parser based on Expat and HTree

    Expat is a stream-oriented XML parser underlying the Mozilla
    project and several other implementations, e.g. Perl XML::Parser.
    XMLParser wraps the C-style Expat library to read the entire
    document into a XMLTree structure. 

    \note When using XMLParser you need to link to libexpat, e.g.
    by specifying \c -lexpat for gcc linker options.

    \sa http://expat.sourceforge.net/

    \author Dr. Lars Hanke
    \date 2006-2007
*/

namespace mgr {

  /*! \class XMLParser
      \brief Generate XMLTree from text using the Expat parser

      This class builds a XMLTree from text fed to it. It uses
      the Expat parser to parse the text fed. When parsing
      is finished, the tree can be exported to the application
      and the parser may be re-armed or discarded.
  */
  class XMLParser {
  protected:
    XMLTree *xml;          //!< The XMLTree we're building
    XML_Parser Expat;      //!< The Expat object we're using
    const XML_Char *enc;   //!< Character encoding argument passed to Expat

    static void _StartElementHandler( void *userData, const XML_Char *name, const XML_Char **atts );
    static void _EndElementHandler( void *userData, const XML_Char *name );
    static void _TextHandler( void *userData, const XML_Char *text, int len );
    virtual m_error_t StartElementHandler( const XML_Char *name, const XML_Char **atts );
    virtual m_error_t EndElementHandler( const XML_Char *name );
    virtual m_error_t TextHandler( StringBuffer *txt );

  public:
    XMLParser() : xml( NULL ), Expat( NULL ), enc( NULL ) {}
    virtual ~XMLParser() {
      if( Expat ) XML_ParserFree( Expat );
      if( enc ) free( const_cast< XML_Char *>(enc) );
      if( xml ) delete xml;
    }
    m_error_t setEncoding( const XML_Char *e );
    m_error_t reset();
    m_error_t start();
    m_error_t read( const char *s, size_t l ){
      if(!s) return ERR_PARAM_NULL;
      if(!Expat){
	m_error_t ierr = start();
	if(ierr != ERR_NO_ERROR) return ierr;
      }
      if(!XML_Parse(Expat, s, l, 0)){
	return ERR_PARS_STX;
      }
      return ERR_NO_ERROR;
    }
    m_error_t abort() {
      if(!Expat) return ERR_NO_ERROR;
      XML_ParserFree( Expat );
      Expat = NULL;
      return ERR_NO_ERROR;
    }
    m_error_t finish() {
      if(!XML_Parse(Expat, NULL, 0, 1)){
	return ERR_PARS_STX;
      }
      return abort();
    }
    XMLTree *submit( m_error_t *err = NULL ){
      if( Expat ){
	if( err ) *err = ERR_PARAM_LCK;
	return NULL;
      }
      XMLTree *txml = xml;
      xml = NULL;
      if( err ) *err = (txml)? ERR_NO_ERROR : ERR_CANCEL;
      return txml;
    }

    const char *describeError( void ){
      enum XML_Error xmlCode = XML_GetErrorCode ( Expat );
      return XML_ErrorString( xmlCode );
    }
    void pos( int& line, int& col ){
      line = XML_GetCurrentLineNumber( Expat );
      col  = XML_GetCurrentColumnNumber( Expat );
    }
    const char *VersionTag(void) const;
  };
};

#endif // _XML_XMLPARSER_H_
