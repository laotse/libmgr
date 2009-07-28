/*
 *
 * Option parser and help system
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: opts.cpp,v 1.4 2008-05-15 20:58:24 mgr Exp $
 *
 * This implements the classes:
 *  cmdOption  - a single command line option
 *  cmdOptList - a list of cmdOption, with usage printer
 *
 */

#include "../util/mgrError.h"
#include "opts.h"

/*
 * Option Item base class
 *
 */

using namespace mgr;

cmdOption::cmdOption(int c, const char *l, const char *help){
  optChar = c;
  optLong = l;
  optHelp = help;
  error = ERR_NO_ERROR;
  type = OPT_TYPE_BOOL;
  value.b = false;
  require = false;
}

m_error_t cmdOption::getValue(unsigned short rtyp, void *val){
  if(!val) return ERR_PARAM_NULL;
  if(rtyp != type) return ERR_PARAM_OPT;
  switch(type){
  case OPT_TYPE_INT:
    val = &(value.i);
    break;
  case OPT_TYPE_DOUBLE:
    val = &(value.d);
    break;
  case OPT_TYPE_BOOL:
    val = &(value.b);
    break;
  case OPT_TYPE_STRING:
    value.s = sval.c_str();
    val = &(value.s);
    break;
  default:
    return ERR_PARAM_RANG;
  }

  return ERR_NO_ERROR;
}

int cmdOption::getChar(void){
  return optChar;
}

const char *cmdOption::getLong(void){
  return optLong.c_str();
}

int cmdOption::compare(cmdOption *o){
  int res;

  if(o->getChar() == optChar) return 0;
  if(o->getLong() && optLong.c_str()){
    res = strcmp(o->getLong(),optLong.c_str());
    if(!res){
      error = ERR_PARAM_UNIQ;
      return error;
    }
  } else return(o->getChar() > optChar)? 1:-1;

  return res;
}

int cmdOption::match(int c, const char *l){
  if(c && (optChar == c)) return 0;
  if(l && optLong.c_str()) return strcmp(l,optLong.c_str());
  if(c){
    c-=optChar; 
    if(!c) return 0;
    return (c>0)?1:-1;
  } else return 0;
}

/*
 * String Option
 *
 */

cmdOption::cmdOption(int s, const char *l, const char *help, const char *def){ 
  cmdOption(s,l,help);
  
  type = OPT_TYPE_STRING;
  sval = def;
  value.s = sval.c_str();
  require = true;
}

/*
 * Double Option
 *
 */

cmdOption::cmdOption(int s, const char *l, const char *help, double def) {
  cmdOption(s,l,help);

  type = OPT_TYPE_DOUBLE;
  value.d = def;
  require = true;
}

/*
 * Integer Option
 *
 */

cmdOption::cmdOption(int s, const char *l, const char *help, int def) {
  cmdOption(s,l,help);

  type = OPT_TYPE_INT;
  value.i = def;
  require = true;
}

/*
 * A list of options concerning a common caption
 *
 */

#define OPT_LIST_BLOCK 10

cmdOptList::cmdOptList(){
  allocated = 0;
  length = 0;
  optList = (cmdOption **)NULL;
  error = ERR_NO_ERROR;
  pretext="";
  title="";
  indent=3;
  width=72;
}

int cmdOptList::add(cmdOption *opt){
  size_t new_size;

  if(!opt) return ERR_NO_ERROR;
  
  if(!optList || (length+1 > allocated)){
    for(new_size = allocated;length+1 >= new_size; new_size += OPT_LIST_BLOCK);
    optList = (cmdOption **)realloc(optList, new_size * sizeof(cmdOption *));
    if(!optList){
      allocated = 0;
      length = 0;
      error = ERR_MEM_AVAIL;
      return error;
    }
  }

  optList[length++] = opt;
  
  return ERR_NO_ERROR;
}

cmdOption *cmdOptList::find(int s, const char *l){
  size_t idx;

  for(idx=0;idx<length;idx++)
      if(!optList[idx]->match(s,l)) return optList[idx];

  return (cmdOption *)NULL;
}

m_error_t cmdOptList::groupTitle(const char *t){
  title = t;

  return ERR_NO_ERROR;
}

const char *cmdOptList::groupTitle(void){
  return title.c_str();
}

m_error_t cmdOptList::usage(FILE *f){
  size_t idx;
  int c,i;
  const char *l;
  std::string instr(indent,' ');
  std::string line(width,0);

  try{
    for(idx=0;idx<length;idx++){
      if(isprint(c=optList[idx]->getChar()))
	if(0 >= fprintf(f,"\n%s-%c",pretext.c_str(),c)) 
	  throw(ERR_FILE_WRITE);
      if((l=optList[idx]->getLong()))
	if(0 >= fprintf(f,"\n%s--%s",pretext.c_str(),l)) 
	  throw(ERR_FILE_WRITE);
      i=pretext.length()+instr.length();
      // Hier müssen wir den Formatter einbauen
      if(0 >= fprintf(f,"\n%s%s%s\n",pretext.c_str(),instr.c_str(), \
		                     optList[idx]->optHelp.c_str())) 
	throw(ERR_FILE_WRITE);     
    }
    if('\n' != putc('\n',f)) throw(ERR_FILE_WRITE);
  }

  catch(m_error_t err){
    error = err;    
    return err;
  }

  return ERR_NO_ERROR;

}

/*
 * The entire collection of all options for an application
 *
 */

cmdLine::cmdLine(void){
  
}
  
m_error_t cmdLine::addGroup(cmdOptList *group){
  return ERR_INT_IMP;
}

m_error_t cmdLine::parse(int argc, const char *argv[]){
  return ERR_INT_IMP;
}
