/*
 *
 * XML parser for XMLTree
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: XMLParser.cpp,v 1.6 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *  XMLParser - the parser object
 *
 * This defines the values:
 *
 */

/*! \file XMLParser.cpp
    \brief DOM style XML parser based on Expat and HTree

    Expat is a stream-oriented XML parser underlying the Mozilla
    project and several other implementations, e.g. Perl XML::Parser.
    XMLParser wraps the C-style Expat library to read the entire
    document into a XMLTree structure. 

    \sa http://expat.sourceforge.net/

    \note Using gcc 3.4.4 on cygwin warnings are produced
    for stl_tree.h about unitinialised __top and __tmp variables.
    These errors relate to gcc optimization and are a known
    bug of gcc (see PR 22207).

    \author Dr. Lars Hanke
    \date 2006-2007
*/

//#define DEBUG
//! Debug Expat callbacks
#define DEBUG_XML 1
#define DEBUG (DEBUG_XML)
#include <mgrDebug.h>

#ifdef DEBUG
# include <stdio.h>
#endif

#include "XMLParser.h"
#include "XMLParser.tag"

using namespace mgr;

void XMLParser::_StartElementHandler( void *userData, 
				      const XML_Char *name, 
				      const XML_Char **atts ){
  XMLParser *parser = static_cast<XMLParser *>(userData);
  parser->StartElementHandler( name, atts );
}

void XMLParser::_EndElementHandler( void *userData, 
				    const XML_Char *name ){
  XMLParser *parser = static_cast<XMLParser *>(userData);
  parser->EndElementHandler( name );
}

void XMLParser::_TextHandler( void *userData, const XML_Char *text, int len ){
  XMLParser *parser = static_cast<XMLParser *>(userData);
  StringBuffer *txt = new StringBuffer( text, len );
  parser->TextHandler( txt );
}

m_error_t XMLParser::StartElementHandler( const XML_Char *name,
					  const XML_Char **atts ){
  xpdbg(XML,"Start tag: <%s>\n",name);
  XMLNode *n = new XMLNode( name );
  while(atts[0] && atts[1]){
    m_error_t res = n->addAttribute(atts[0],atts[1]);
    if(res != ERR_NO_ERROR){
      delete n;
      return res;
    }
    atts += 2;
  }
  m_error_t res = n->branch();
  if(res != ERR_NO_ERROR){
    delete n;
    return res;
  }
  xml->appendChild( n, true );
  return ERR_NO_ERROR;
}

m_error_t XMLParser::EndElementHandler( const XML_Char *name ){
  xpdbg(XML,"End tag: </%s>\n",name);
  XMLNode *n = xml->parent();
  if(!n) return ERR_PARS_STX;
  // todo check Tag value
  return ERR_NO_ERROR;
}

m_error_t XMLParser::TextHandler( StringBuffer *txt ){
  if(!txt) return ERR_PARAM_NULL;
  xpdbg(XML,"Text: %s\n",txt->cptr());
  return xml->addText( *txt );
}

const char * XMLParser::VersionTag(void) const{
  return _VERSION_;
}

m_error_t XMLParser::setEncoding( const XML_Char *e ){
  if( Expat != NULL ) return ERR_PARAM_LCK;
  if( enc ) free( const_cast< XML_Char * >(enc) );
  if(!e){
    enc  = NULL;
    return ERR_NO_ERROR;
  }
  enc = strdup( e );
  if(!enc) return ERR_MEM_AVAIL;
  return ERR_NO_ERROR;
}

m_error_t XMLParser::reset() {
  if( Expat ) XML_ParserFree( Expat );
  Expat = NULL;
  if( xml ) xml->clear();
  return ERR_NO_ERROR;
}

m_error_t XMLParser::start() {
  if( Expat ){
    m_error_t ierr = reset();
    if( ierr != ERR_NO_ERROR ) return ierr;
  }
  if(!xml) xml = new XMLTree;
  if(!xml) return ERR_MEM_AVAIL;
  Expat = XML_ParserCreate( enc );
  if(!Expat) return ERR_CLS_CREATE;
  XML_SetUserData( Expat, this );
  XML_SetElementHandler( Expat, _StartElementHandler, _EndElementHandler);
  XML_SetCharacterDataHandler( Expat, _TextHandler );
  return ERR_NO_ERROR;
}


/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>
#include <fcntl.h>
#include <libgen.h>

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;
  FileDump dout(stdout);

  tests = errors = 0;

  try {
    printf("Test %d: XMLParser.CTOR\n",++tests);
    XMLParser parser;
    puts("+++ XMLParser.CTOR() finished OK!");
    char *xpath = strdup(argv[0]);
    if(!xpath) mgrThrowExplain(ERR_MEM_AVAIL,"Cannot allocate pathname");
    xpath = dirname(xpath);
    if(!xpath) mgrThrowExplain(ERR_MEM_AVAIL,"dirname() failed");
    if(0 > chdir( xpath )) mgrThrowFormat(ERR_FILE_STAT,"chdir(\"%s\") failed",xpath);
    int fd = open("testcases.xml",O_RDONLY);
    if(-1 == fd) mgrThrowExplain(ERR_FILE_OPEN,"Cannot open testcases.xml");
    printf("Test %d: XMLParser.read()\n",++tests);
    const size_t bufferSize = 512;
    char Buffer[bufferSize];
    ssize_t rd = 0;
    res = ERR_NO_ERROR;
    while(((rd = read( fd, Buffer, bufferSize)) > 0) && (res == ERR_NO_ERROR)){
      printf("??? read() %d bytes\n",rd);
      res = parser.read(Buffer,rd);
    }    
    if((rd == 0) && (res == ERR_NO_ERROR)){
      puts("??? finish()");
      res = parser.finish();
    } else if(rd < 0){
      res = ERR_FILE_READ;
    }
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: read() failed 0x%.4x\n",(int)res);
      if((res & ERR_MAJORCODE) == ERR_FILE){
	perror("*** System error");
      }
      if((res & ERR_MAJORCODE) == ERR_PARS){
	printf("*** Expat error: %s\n",parser.describeError());
	int row,col;
	parser.pos( row, col );
	printf("*** Expat at:    %d, %d\n",row, col);
      }
    } else {
      puts("+++ read() finished OK!");
    }
    printf("Test %d: XMLParser.submit()\n",++tests);
    XMLTree *tree = parser.submit( &res );
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: submit() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ read() finished OK!");
    }
    printf("Test %d: XMLTree.dump()\n",++tests);
    tree->root();
    res = tree->dump(dout);
    dout.putchar('\n');
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: dump() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ dump() finished OK!");
    }

    printf("\n%d tests completed with %d errors.\n",tests,errors);
    printf("Used version: %s\n",parser.VersionTag());
  }
  catch(const std::exception& e){
    printf("*** Exception thrown: %s\n",e.what());
  }
  catch(int err){
    printf("*** Exception thrown: 0x%.4x\n",err);
  }

  return 0;  
}

#endif //TEST
