/*
 *
 * Option parser and help system
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: options.cpp,v 1.7 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *
 * This defines the values:
 *
 */


#include "options.h"
#include <stdlib.h>
#include <getopt.h>

#include "options.tag"

#define DEBUG_TRACE 1
#define DEBUG ( DEBUG_TRACE )
#include <mgrDebug.h>

using namespace mgr;

#ifdef __CYGWIN__
double strtold(const char *s, char **e){
  double v;

  int l = sscanf(s,"%lf",&v);
  if(!l){
    // this is actually a bug in the common strtold() definition
    *e = const_cast<char *>(s);
    return v;
  }
  const char *t = s;
  if((*t == '-') || (*t == '+')) t++;
  while(isdigit(*t)) t++;
  if(*t == '.'){
    t++;
    while(isdigit(*t)) t++;
  }
  if(toupper(*t) == 'E'){
    t++;
    if((*t == '-') || (*t == '+')) t++;
    while(isdigit(*t)) t++;
  }
  // this is actually a bug in the common strtold() definition
  *e = const_cast<char *>(t);

  return v;
}
#endif // __CYGWIN__

int OptIDLess::strcmp(const char *s, const char *t){
  if(!s && !t) return 0;
  if(!s) return -1;
  if(!t) return 1;
  int ldiff = 0;
  while(*s && *t){
    if(*s == *t){
      ++s; ++t;
      continue;
    }
    int cdiff = toupper(*s) - toupper(*t);
    if(cdiff) return cdiff;
    if(!ldiff) ldiff = cdiff;
    ++s; ++t;
  }
  if(*s) return 1;
  if(*t) return -1;
  return ldiff;
}

/*
 * implementations of ConfigOption<>::parseValue()
 *
 */

namespace mgr {
template<>
m_error_t FreeIntegerOption::parseValue(const char *s, int& v) const {
  if(!s){
    v = Default;
    return ERR_CANCEL;
  }
  char *e;
  v = strtol(s,&e,10);
  if(s == e) return ERR_PARAM_OPT;
  if(*e) return ERR_CANCEL;
  return ERR_NO_ERROR;
}

template<>
m_error_t FreeDoubleOption::parseValue(const char *s, double& v) const {
  if(!s){
    v = Default;
    return ERR_CANCEL;
  }
  char *e;
  v = strtold(s,&e);
  if(s == e) return ERR_PARAM_OPT;
  if(*e) return ERR_CANCEL;
  return ERR_NO_ERROR;
}


static bool strpcmp(const char *s, const char *t){
  if(!s || !t) return false;
  bool doBreak = false;
  while(*s && *t){
    if(*t == '_'){
      doBreak = true;
      ++t;
    }
    if(toupper(*s) != toupper(*t)) return false;
    ++s; ++t;
  }
  if(*t && doBreak) return true;
  if(*s || *t) return false;
  return true;
}

template<>
m_error_t BoolOption::parseValue(const char *s, bool& v) const {
  if(!s) {
    v = !Default;
    return ERR_CANCEL;
  }
  do{
    if(strpcmp(s,"_true")) { v = true; break; }
    if(strpcmp(s,"_false")) { v = false; break; }
    if(strpcmp(s,"_yes")) { v = true; break; }
    if(strpcmp(s,"_no")) { v = false; break; }
    if(strpcmp(s,"0")) { v = true; break; }
    if(strpcmp(s,"1")) { v = false; break; }
    if(strpcmp(s,"_set")) { v = true; break; }
    if(strpcmp(s,"_reset")) { v = false; break; }
    if(strpcmp(s,"_unset")) { v = false; break; }
    return ERR_PARAM_OPT;
  } while(0);
  return ERR_NO_ERROR;
}

} // namespace mgr for template specialization
/*
 * Main Configuration handling
 *
 */

m_error_t Configuration::__append(ConfigItem *ci){
  ConfigItem *it = firstSibling();
  if(!it){
    appendNext(ci,true);
    return ERR_NO_ERROR;
  }
  if(*it > *ci){
    // need to insert first element
    it = parent();
    insertChild(ci,true);
    return ERR_NO_ERROR;
  }
  // note it->next() is just a look ahead
  // this->next() moves the pointer in HTree
  while(it->next()) if(*(it->next()) < *ci) it = next(); else break;
  if(it->next()){
    if(*(it->next()) == *ci) return ERR_PARAM_UNIQ;
    // now we know that it->next() > ci, 
    // i.e. ci shall be inserted after it
    insertNext(ci);
  } else {
    // it has been < ciand there's no next: append it
    appendNext(ci,true);    
  }
  return ERR_NO_ERROR;
}

m_error_t Configuration::append(ConfigItem *ci){
  m_error_t err = __append(ci);
  if(err != ERR_NO_ERROR) return err;
  ItemMap[ci->id] = ci;
  return err;
}

m_error_t Configuration::createOptString(std::string& opts, 
					 std::vector<struct option>& longopts){
  ConfigItem *it = root();
  int depth = 0;
  
  while(it){
    if(it->acIf & ConfigItem::CONSOLE){
      if(isOptChar(it->optChar())){
	opts.push_back(it->optChar());
	switch(it->parameterSpec()){
	default:
	  // no_argument	  
	  break;	  
	case ConfigItem::OPTIONAL:
	  // optional_argument
	  opts.push_back(':');
	  // fall-through optional is "::"
	case ConfigItem::MANDATORY:
	  opts.push_back(':');
	  break;
	}	
      } 
      if(it->optString()){
	struct option newOpt;
	newOpt.name = it->optString();
	newOpt.val = it->optChar();
	newOpt.flag = NULL;
	switch(it->parameterSpec()){
	default:
	  // no_argument
	  newOpt.has_arg = 0;
	  break;
	case ConfigItem::OPTIONAL:
	  // optional_argument
	  newOpt.has_arg = 2;
	  break;
	case ConfigItem::MANDATORY:
	  newOpt.has_arg = 1;
	  break;
	}
	longopts.push_back(newOpt);
      }
      
    }
    it = iterate(&depth);    
  }

  // terminate longopts array
  struct option newOpt;
  memset(&newOpt,0,sizeof(newOpt));
  longopts.push_back(newOpt);

  return ERR_NO_ERROR;
}

m_error_t Configuration::parseOptions(int argc, char * const argv[],
				      ConsoleFormatter &cfm){
  programName = argv[0];
  std::string optString;
  std::vector<struct option> longOpts;
  std::map<int,const ConfigItem *> optMap;
  std::map<int,const ConfigItem *>::const_iterator optIter;

  m_error_t err = createOptString(optString, longOpts);
  if(err != ERR_NO_ERROR) return err;
  xpdbg(TRACE,"### Option String >%s<\n",optString.c_str());

  ConfigItem *it = root();
  while(it){
    if(it->optChar()){
      if((optIter = optMap.find(it->optChar())) != optMap.end()){
	pdbg("### Options %s and %s have same reference %d\n",
	     (*optIter).second->id, it->id,it->optChar());
	return ERR_PARAM_UNIQ;
      }
      optMap[it->optChar()] = it;
    }
    it = iterate();
  }

  do {
    int longIndex;
    int opt = getopt_long(argc,argv,
			  optString.c_str(),&(longOpts[0]),
			  &longIndex);
    // we're done
    if(opt == -1) {
      if (optind < argc) {
	numFiles = argc - optind;
	Files = &(argv[optind]);
      } else {
	numFiles = 0;
	Files = NULL;
      }
      break;
    }
    switch(opt){
    case '?':
      usage(cfm);
      return ERR_CANCEL;
    case ':':
      usage(cfm);
      return ERR_PARAM_UDEF;
    default:
      optIter = optMap.find(opt);
      if(optIter == optMap.end()){
	return ERR_INT_STATE;
      } else {
	// we promised optMap the ConfigItem is const,
	// this is true regarding the interesting part for the map
	// we did not commit this to our caller!
	ConfigItem *fit = const_cast<ConfigItem *>((*optIter).second);
	// readValue must handle NULL pointers
	m_error_t err = fit->readValue(optarg);
	if((err != ERR_NO_ERROR) &&
	   (err != ERR_CANCEL)) return err;
	xpdbg(TRACE,"### Read option %d (%s) from %s\n",
	      fit->optChar(),
	      (fit->optString()) ? fit->optString() : "NULL",
	      (optarg) ? optarg : "(empty)");
      }
      break;
    }
  } while(1);
  
  return ERR_NO_ERROR;
}

m_error_t Configuration::usage(ConsoleFormatter& cfm) {
  if(!programName) return ERR_PARAM_UDEF;  
  cfm.Indent(0);
  m_error_t res = cfm.print("Usage:");
  if(res != ERR_NO_ERROR) return res;
  cfm.Indent(3);
  std::string line = programName;
  line += " [options] files\n";
  res = cfm.print(line);
  if(res != ERR_NO_ERROR) return res;
  if(shortForm){
    res = cfm.print(shortForm);
    if(res != ERR_NO_ERROR) return res;
    res = cfm.lineFeed();    
    if(res != ERR_NO_ERROR) return res;
  }
  if(Copyright){
    res =cfm.print(Copyright);
    if(res != ERR_NO_ERROR) return res;
    res = cfm.lineFeed();    
    if(res != ERR_NO_ERROR) return res;
  }
  cfm.Indent(0);
  res = cfm.print("The following options are supported:");
  if(res != ERR_NO_ERROR) return res;
  return help(cfm);
}

/*! \todo The use of root() inhibits const qualifier.
          This implies that usage() calling help() cannot be const
	  either!
*/
m_error_t Configuration::help(ConsoleFormatter& cfm) {
  ConfigItem *it = root();
  int depth = 0;
  std::string optLine;

  while(it){
    if((it->acIf & ConfigItem::CONSOLE)
       && (isOptChar(it->optChar()) || it->optString())) {
      cfm.Indent(0);
      if(isOptChar(it->optChar())){
	optLine = " -";
	optLine += it->optChar();
	cfm << optLine;
	m_error_t res = cfm.readError();
	if(res != ERR_NO_ERROR) return res;
      } else {
	m_error_t res = cfm.lineFeed();
	if(res != ERR_NO_ERROR) return res;
      }
      if(it->optString()){
	optLine = "--";
	optLine += it->optString();
	cfm << optLine;
	m_error_t res = cfm.readError();
	if(res != ERR_NO_ERROR) return res;
      }
      cfm.Indent(3);
      cfm << it->helpText;
      m_error_t res = cfm.readError();
      if(res != ERR_NO_ERROR) return res;
    }    
    it = iterate(&depth);
  }
  return ERR_NO_ERROR;
}

const ConfigItem *Configuration::find( const char *long_opt, m_error_t *err){
  ConfigItem *it = root();
  int depth = 0;
  if(err) *err = ERR_NO_ERROR;
  while(it){
    if(!strcmp(long_opt, it->optString())) return it;
    it = iterate(&depth);
  }
  if(err) *err = ERR_CANCEL;
  return NULL;
}

const ConfigItem *Configuration::find( const int short_opt, m_error_t *err){
  ConfigItem *it = root();
  int depth = 0;
  if(err) *err = ERR_NO_ERROR;
  while(it){
    if(short_opt == it->optChar()) return it;
    it = iterate(&depth);
  }
  if(err) *err = ERR_CANCEL;
  return NULL;
}

const char * Configuration::VersionTag() const {
  return _VERSION_;
}

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

int main(int argc, char * const argv[]){
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  try {
    ConsoleFormatter cfm(stdout);
    printf("Test %d: Configuration::CTOR() \n",++tests);
    Configuration cfg;
    cfg.about("This program is the test suite for the mgr::Configuration "\
	      "class, which is the heir of proc_opts.c and thus my "\
	      "front-end for <getopts.h>. It enforces that no undocumented "\
	      "options can be used.");
    cfg.copyright("Copyright (c) Dr. Lars Hanke 2000-2006");
    puts("+++ CTOR() done!");
    
    printf("Test %d: Configuration::append() \n",++tests);
    const char *reason = "none";
    do {
      reason = "bool";
      BoolOption *bo =
	new BoolOption('?',"help","Print this help page.",
			  false,ConfigItem::NO_PARAM);
      if(ERR_NO_ERROR != (res = cfg.append(bo))) break;
      reason = "string";
      StringOption *so = 
	new StringOption('s',"string","A sample string option.",NULL);
      if(ERR_NO_ERROR != (res = cfg.append(so))) break;
      reason = "integer";
      IntegerOption *io = 
	new IntegerOption('i',"integer",
			  "A sample integer option with range."
			  ,0,0,10);
      if(ERR_NO_ERROR != (res = cfg.append(io))) break;
      reason = "double";
      DoubleOption *dop = 
	new DoubleOption('d',"double",
			 "A sample double option with range."
			 ,0.0,-1.0,1.0);
      if(ERR_NO_ERROR != (res = cfg.append(dop))) break;
      reason = "switch";
      SwitchOption *swo = 
	new SwitchOption('f',"switch",
			 "A sample switch option.");
      if(ERR_NO_ERROR != (res = cfg.append(swo))) break;
      // This one inserts in front of it all, chekc the _append() method
      bo = new BoolOption('b',"bool","A sample boolean option.",
			  false,ConfigItem::OPTIONAL);
      if(ERR_NO_ERROR != (res = cfg.append(bo))) break;
    } while(0);
    if(res != ERR_NO_ERROR){
      mgrException(e,res);
      e.explain(reason);      
      printf("*** %s\n",e.what());
    } else {
      puts("+++ Configuration::append() finished successfully!");
    }

    printf("Test %d: Configuration::parseOptions() \n",++tests);
    res = cfg.parseOptions(argc,argv,cfm);
    if(res != ERR_NO_ERROR){
      if(res == ERR_CANCEL){
	printf("+++ wrong option correctly handled - done!\n");
	exit(0);
      }
      mgrException(e,res);
      e.explain("Configuration::parseOptions()");
      printf("*** %s\n",e.what());
    } else {
      for(size_t i=0;i<cfg.argFiles();++i){
	printf("??? File %d: >%s<\n",i+1,cfg.getFile(i));
      }
      puts("+++ Configuration::parseOptions() finished successfully!");
    }

    printf("Test %d: Configuration::help() \n",++tests);
    res = cfg.help(cfm);
    if(res != ERR_NO_ERROR){
      mgrException(e,res);
      e.explain("Configuration::help()");
      
      printf("*** %s\n",e.what());
    } else {
      puts("+++ Configuration::help() finished successfully!");
    }
    
    printf("\n%d tests completed with %d errors.\n",tests,errors);
    printf("Used version: %s\n",cfg.VersionTag());
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
