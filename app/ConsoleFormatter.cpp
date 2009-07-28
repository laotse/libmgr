/*
 *
 * ConsoleFormatter to print formatted text to a stream
 * used primarily to print help texts
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: ConsoleFormatter.cpp,v 1.6 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *
 * This defines the values:
 *
 */

#include "ConsoleFormatter.h"

#define DEBUG_TRACE 1
#define DEBUG_DUMP  2
#define DEBUG_MIND  4
#define DEBUG ( DEBUG_TRACE )
#include <mgrDebug.h>

#include "ConsoleFormatter.tag"

using namespace mgr;

// assumes that indent < width !
/*! \return Error code as defined in mgrError.h

    Maintains the std::string indent_str as a string containing
    only spaces. If the indent is larger than the capacity of
    the string, i.e. before first use or after changing the width, 
    it will be reserved as width+1.
    If the indent is larger than the size of the string, i.e.
    before first use or after changing the width, it will be filled
    with spaces and a trailing 0 up to its capacity
    Finally, cIndent will be set to the position inside the string,
    which produces indent spaces and the trailing 0.

    \todo As it seems the trailing 0 is not initialized. After all
    using std::string is nonsense. Use some _wtBuffer instead.
*/
m_error_t ConsoleFormatter::makeIndent(){
  xpdbg(MIND,"### makeIndent for indent: %u, width: %u\n",indent,width);
  if(indent+1 > indent_str.capacity()){
    try {
      xpdbg(MIND,"### reserve %u bytes for indent_str\n",width+1);
      indent_str.reserve(width+1);
    }
    catch (...) {
      // reserve failed ...
      if(width <= 0) return ERR_PARAM_RANG;
      return ERR_MEM_AVAIL;
    }
  }
  if(indent > indent_str.size()){
    xpdbg(MIND,"### makeIndent: now fill string sized %d to %d\n",
	  indent_str.size(), indent_str.capacity()-1);
    indent_str = std::string(indent_str.capacity()-1,' ');
  }
  if(indent) {
    cIndent = &(indent_str.at(indent_str.size() - indent - 1));
  } else cIndent = "";

  xpdbg(MIND,"### Created indent: %d as cIndent >%s<\n",indent,cIndent);

  return ERR_NO_ERROR;
}

/*! \brief Print a portion of a string to file
    \param start First character to print
    \param stop Final character to print (including)
    \param fil Stream to print to
    \param line Strip leading spaces - for use after line breaks
    \return len Number of characters printed
*/
static int fput_substring(const char *start, const char *stop, FILE *fil, const bool& line){
  if(line) while((start<stop) && isspace(*start)) start++;
  
  int len = 0;
  for(len=0;start<=stop;++start,++len) 
    if(EOF == fputc(*start,fil)) {
      len = EOF;
      break;
    }

  return len;
}

/*! \brief Print text formatted
    \param text Text for formatted output
    \return Error code as defined in mgrError.h

    Lazy evaluates the indentation settings to cIndent.
    Prints the text using the given indent at each start
    of a new line. Word-wrapping line breaks are performed
    at absolute character position width.
    The LF character is handled to actively break the line.
    The TAB charcter is treated as space currently.

    \todo Treat TAB correctly and allow more formatting.
    \todo Once we have strings and streams use these.
*/
m_error_t ConsoleFormatter::print(const char *text){
  if(!text || !(*text)) return ERR_NO_ERROR;
  if(!fil) return ERR_PARAM_NULL;
  if(indent >= width) return ERR_PARAM_RANG;
  if(!cIndent){
    m_error_t err = makeIndent();
    if(err != ERR_NO_ERROR) return err;
  }

  // set beginning of line
  bool line = true;
  const char *p = text;
  const char *w = text;
  // the first indent
  size_t i = indent;
  if(EOF == fputs(cIndent,fil)) return ERR_FILE_WRITE;
  for(;*p;p++){
    switch(*p){
    case '\n':
      { 
	// give q a scope
	const char *q=p;
	for(;isspace(*q);q--);
	if(EOF == fput_substring(w,q,fil,line)) return ERR_FILE_WRITE;
	if(EOF == fputc('\n',fil)) return ERR_FILE_WRITE;
	if(EOF == fputs(cIndent,fil)) return ERR_FILE_WRITE;
	i = indent;
	w = p+1;
	// after LF a new line starts
	line = true;
      }
      break;
    case ' ':
      // todo: \t is not handled correctly
    case '\t':
      if(i >= width){
	if(EOF == fputc('\n',fil)) return ERR_FILE_WRITE;
	xpdbg(DUMP,"### wrap @%d\nstart: %s\nend: %s\n",i,w,p);
	if(EOF == fputs(cIndent,fil)) return ERR_FILE_WRITE;
	{
	  int l = fput_substring(w,p,fil,true);
	  if(l == EOF) return ERR_FILE_WRITE;
	  i = indent + l;
	}
      } else {
	if(EOF == fput_substring(w,p,fil,line)) return ERR_FILE_WRITE;
      }
      w = p+1;
      // we produced some output, we're definitely not a the line start
      line = false;
      i++;
      break;
    default:
      i++;
      break;
    }
  }
  if(!*p) p--;
  if(i > width){
    if(EOF == fputc('\n',fil)) return ERR_FILE_WRITE;
    if(EOF == fputs(cIndent,fil)) return ERR_FILE_WRITE;
    {
      int l = fput_substring(w,p,fil,true);
      if(l == EOF) return ERR_FILE_WRITE;
      i = l + indent;
    }
  } else {
    if(EOF == fput_substring(w,p,fil,line)) return ERR_FILE_WRITE;
  }
  // hmm, even if we are at a line start, we have printed the indent
  // if(!line) if(EOF == fputc('\n',fil)) return ERR_FILE_WRITE;
  if(EOF == fputc('\n',fil)) return ERR_FILE_WRITE;

  return ERR_NO_ERROR;
}


const char * ConsoleFormatter::VersionTag() const {
  return _VERSION_;
}

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

void print_scale(const size_t len = 80){
  const char *decade = "0123456789";
  for(size_t i = 0;i<len / 10;++i){
    printf(decade);
  }
  for(size_t i=0;i < len % 10;++i){
    putchar(decade[i]);
  }
  putchar('\n');
}

int main(int argc, const char *argv[]){
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;


  try {
    printf("Test %d: ConsoleFormatter::CTOR() \n",++tests);
    ConsoleFormatter cfm(stdout);
    puts("+++ CTOR() done!");

    const char *text = "Hallo, Hallo, ich bin Dein Ohrwurm, Dein Ohrwurm! "\
                       "Ich komme in der Nacht und am Tag und fülle "\
                       "sinnlos Zeilen von links nach rechts, und manchmal "\
                       "wechsele ich sie sogar:\n"\
                       "so wie hier. Außerdem kann ich Tabulatoren setzen. "\
                       "so wie dieser \"\t\" hier. Am Ende des gesamten "\
                       "Textes mache ich ein LF.";

    printf("Test %d: ConsoleFormatter::print() indent 0\n",++tests);
    print_scale(80);
    res = cfm.print(text);
    if(res != ERR_NO_ERROR){
      mgrException(e,res);
      e.explain("ConsoleFormatter::print()");
      printf("*** %s\n",e.what());
    } else {
      puts("+++ ConsoleFormatter::print() finished successfully!");
    }
    
    printf("Test %d: ConsoleFormatter::print() indent 3\n",++tests);
    cfm.Indent(3);
    print_scale(80);
    res = cfm.print(text);
    if(res != ERR_NO_ERROR){
      mgrException(e,res);
      e.explain("ConsoleFormatter::print()");
      printf("*** %s\n",e.what());
    } else {
      puts("+++ ConsoleFormatter::print() finished successfully!");
    }
           
    printf("\n%d tests completed with %d errors.\n",tests,errors);
    printf("Used version: %s\n",cfm.VersionTag());
  }
  catch(int& err){
    printf("Caught integer exception 0x%.4x\n",err);
  }
  catch(std::exception& e){
    printf("Caught standard exception: %s\n",e.what());
  }

  return 0;  
}


#endif // TEST
