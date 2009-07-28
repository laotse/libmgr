/*
 *
 * Option parser and help system
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: opts.h,v 1.4 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *  cmdOption  - a single command line option
 *  cmdOptList - a list of cmdOption, with usage printer
 *
 * This defines the values:
 *  option type identifier
 *
 */

#ifndef _APP_OPTS_H_
#define _APP_OPTS_H_

#include <stdio.h>
#include <string>

#define OPT_TYPE_INVALID 0
#define OPT_TYPE_INT     1
#define OPT_TYPE_BOOL    2
#define OPT_TYPE_STRING  3
#define OPT_TYPE_DOUBLE  4
#define OPT_TYPE_MAX     4

namespace mgr {

class cmdOption {
  int optChar;
  std::string optLong;
  m_error_t error;
  unsigned short type;
  std::string sval;
  union {
    int i;
    double d;
    const char *s;
    bool b;
  } value;
  bool require;

 public:
  std::string optHelp;

  cmdOption(int s, const char *l, const char *help);
  cmdOption(int s, const char *l, const char *help, int def);
  cmdOption(int s, const char *l, const char *help, const char *def);
  cmdOption(int s, const char *l, const char *help, double def);  
  int getChar(void);
  const char *getLong(void);
  int compare(cmdOption *);
  int match(int, const char *);
  m_error_t cmdOption::getValue(unsigned short rtyp, void *val);
};

class cmdOptList {
  cmdOption **optList;
  size_t length;
  size_t allocated;
  m_error_t error;
  unsigned short indent;
  unsigned short width;
  std::string pretext;
  std::string title;

 public:
  cmdOptList();
  int add(cmdOption *opt);
  cmdOption *find(int s, const char *l);
  m_error_t usage(FILE *f);
  const char *groupTitle(void);
  m_error_t groupTitle(const char *);
};

class cmdLine {
  cmdOptList **optGroups;
  size_t length;
  size_t allocated;
  m_error_t error;
  
 public:
  cmdLine();
  // m_error_t addOpt(cmdOption *opt);
  m_error_t addGroup(cmdOptList *group);
  m_error_t parse(int argc, const char *argv[]);  
};

}; // namespace mgr

#endif // _APP_OPTS_H_
