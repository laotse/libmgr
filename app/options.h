/*
 *
 * Option parser and help system
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: options.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *
 * This defines the values:
 *
 */

#ifndef _APP_OPTIONS_H_
#define _APP_OPTIONS_H_

#include <mgrError.h>
#include <htree.h>
#include <ConsoleFormatter.h>
#include <string.h>
#include <string>
#include <map>

/*! \file options.h
    \brief Handle command line options and help texts

    The idea of the options module is to encapsulate
    getopt, which is for some reason often re-implemented
    by hand and of course far from complete and flawless, 
    and in the same footprint force the programmer to put
    at least some lines of comment for each option.

    The module will asemble all these comments and information
    about authors and command purpose to a standard man-like
    output using ConsoleFormatter and print if on screen,
    when there are usage errors. For standardization clients
    shall implement the

    BoolOption('?',"help","Print this help page.",false,ConfigItem::NO_PARAM),

    which will ensure that "--help" and "-?" are available, as well as
    option parsing errors will display the help text.

    \version  $Id: options.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
    \author Dr. Lars Hanke
    \date 2004-2007
*/


// This is what we need for protected members
struct option;

namespace mgr {
  /*! \class OptIDLess
      \brief Comparator for long option strings

      This is ::strcmp(), but strings are ordered
      as if they were all upcase. Only, if two strings
      are identical apart from case, the string containing
      the first lower case where the other has an upper case
      is considered as larger.

      The is "sea" is followed by "SEE" no matter the case
      of any character in either word, but "SEE" is followed
      by "SeE".

      The function is available as static member or as functor.
  */
  class OptIDLess {
  public:
    //! static member interface to OptIDLess sorting
    /*! \param s String1
        \param t String2
        \return Positive if s > t, negative if s < t, or zero if s == t
    */
    static int strcmp(const char *s, const char *t);
    //! functor interface to OptIDLess sorting
    /*! \param s String1
        \param t String2
        \return true if s < t
    */
    bool operator()(const char *s, const char *t) const {
      if(!t) return true;
      if(!s) return false;
      return strcmp(s,t) < 0;
    }
  };

  /*! \class ConfigItem
      \brief Generic base class for configuration values

      The idea is to put all configuration data
      imported from the command line, from configuration files,
      produced internally, imported from some GUI, etc. into
      a HTree for access and maintaining during program
      execution.

      The main benefit is actually that ConfigItem \b requires
      a help text to be specified, such that all such items
      must be documented. The Configuration class will finally
      be able to produce a documentation from all these help texts.

      Putting it into a HTree instead of a vector allows for complex
      configuration probably having identical options in different
      contexts. The HTree structure may also be used to put
      large option fields into related sections in order to have
      more than just the alphabetical order of OptIDLess.

      The class implements the sorting of ConfigItems by their
      ID string according to OptIDLess order. It furthermore
      defines an interface for the implementation of real parameters.
  */
  class ConfigItem : public HTreeNode {
    friend class Configuration;

  public:
    //! Where from can this setting be imported
    /*! \note The entries constitute a flag field to be ored. */
    enum eAccess {
      NONE    = 0x0000,  //!< Internal only
      CONSOLE = 0x0001,  //!< Commandline only
      GUI     = 0x0002   //!< Interactively only
    };
    //! Parameter import access control type
    typedef enum eAccess AccessType;

    //! Parameter requirements for options
    enum eParamCount {
      NO_PARAM,          //!< The parameter is a switch and cannot have arguments
      OPTIONAL,          //!< The parameter has arguments, but is understood without
      MANDATORY          //!< The parameter must have arguments
    };
    //! Parameter argument requirement type
    typedef enum eParamCount ParSpec;

  protected:
    const char *helpText;  //!< The help text to be displayed for this option
    const char *id;        //!< The name used for sorting the manual
    AccessType acIf;       //!< The source access control for this parameter
    ParSpec numPar;        //!< The argument requirements

    //! Import OptIDLess::strcmp() as member function for convenience
    inline int strcicmp(const char *s, const char *t) const {
      return OptIDLess::strcmp(s,t);
    }

  public:
    //! CTOR
    ConfigItem(const char *help, const char *ident, const AccessType& at, const ParSpec& np)
      : helpText(help), id(ident),  acIf(at), numPar(np) {}

    //! Copy CTOR
    ConfigItem(const ConfigItem& ci){
      helpText = ci.helpText;
      id = ci.id;
      acIf = ci.acIf;
      numPar = ci.numPar;
    }
    //! DTOR virtual since this is an interface
    virtual ~ConfigItem() {}

    //! Compare by ID only
    inline bool operator!=(const char *item) const {
      if(!id || !item) return true;
      return strcmp(id,item);
    }
    //! \overload bool operator!=(const char *item) const
    inline bool operator!=(const ConfigItem& ci) const {
      return (*this != ci.id);
    }
    //! Compare by ID only
    inline bool operator==(const char *item) const {
      return !(*this != item);
    }
    //! \overload bool operator==(const char *item) const
    inline bool operator==(const ConfigItem& ci) const {
      return (*this == ci.id);
    }
    //! Compare by ID only
    inline bool operator<(const char *item) const {
      if(!item) return true;
      if(!id) return false;
      return (strcicmp(id,item) < 0);
    }
    //! \overload bool operator<(const char *item) const
    inline bool operator<(const ConfigItem& ci) const {
      return (*this < ci.id);
    }
    //! Compare by ID only
    inline bool operator>(const char *item) const {
      if(!item) return false;
      if(!id) return true;
      return (strcicmp(id,item) > 0);
    }
    //! \overload bool operator>(const char *item) const
    inline bool operator>(const ConfigItem& ci) const {
      return (*this > ci.id);
    }
    //! Set source access information
    inline void accessType(const AccessType& at){
      acIf = at;
    }
    //! Get source access information
    inline const AccessType& accessType() const {
      return acIf;
    }
    //! Set argument requirement specification
    inline void parameterSpec(const ParSpec& np){
      numPar = np;
    }
    //! Get argument requirement specification
    inline const ParSpec& parameterSpec() const {
      return numPar;
    }

    //! Get next item in sequence
    inline const ConfigItem *next() const {
      return static_cast<const ConfigItem *>(getNext());      
    }
    //! Get first child of item
    inline const ConfigItem *child() const {
      return static_cast<const ConfigItem *>(getChild());
    }

    //! Get the numerical option number. 
    /*! This is the single character option for command line options. 
        Use values > 256 for non-commandline options. Note that
	this class does not include this field and always returns 0.
	Overload in your derived classes.
    */
    virtual int optChar() const { return 0; };
    //! Get the parameter name
    /*! For command line options this is the long option string
        without the "--". For other parameters it may be the identifier
	in some file. ConfigItem just implements sorting and
	does not implement the real information. It returns
	always NULL. Overload in your derived classes.
    */
    virtual const char *optString() const { return NULL; };

    //! Get the parameter value
    //! \todo This is ugly.
    virtual m_error_t readValue(const char *s) { return ERR_INT_STATE; }
  };

  /*! \class ConfigOption
      \brief Command line options

      This is the template for command line options with
      parameters of type T. Even options without arguments
      have type t=bool, since they can be specified or not.

      If no distinct identified is passed during creation,
      the long option name is used as identifier. Therefore,
      you should always provide a long option name, unless
      you know what you're doing.
  */
  template<typename T> class ConfigOption : public ConfigItem {
  protected:
    T Value;                 //!< Current value of option
    const T Default;         //!< Default value if option is not given or default is requested
    int shortOption;         //!< Option character or non-printable code, if no short option is available
    const char *longOption;  //!< Long option string or NULL if no long option is available
    
  public:
    //! CTOR
    /*! \param sOpt Short option character, or numerical unique sorting index
        \param lOpt Long option string, or NULL is no long option exists
        \param help Help text to be shown in the manual
        \param def  Default value
        \param np   Parameter argument requirement
        \param ident (optional) Sorting index for option, defaults to lOpt

	Options are bound to console input.
    */
    ConfigOption(int sOpt, const char *lOpt, const char *help, 
		 const T& def, const ParSpec& np = MANDATORY, 
		 const char *ident = NULL)
      : ConfigItem( help, ident, CONSOLE, np), 
        Value(def), Default(def), shortOption(sOpt), longOption(lOpt) {
      if(!ident) id = lOpt;
    }
    //! copy CTOR
    ConfigOption(const ConfigOption& co) 
      : ConfigItem(co) {
      Value = co.Value;
      Default = co.Default;
      shortOption = co.shortOption;
      longOption = co.longOption;
    }
    //! DTOR
    virtual ~ConfigOption() {}
    //! Get current optiopn value
    inline const T& value() const {
      return Value;
    }
    //! Set current option value to default
    virtual const T& setValue(){
      Value = Default;
      return Value;
    }
    //! Set current otion value
    virtual const T& setValue(const T& val){
      Value = val;
      return Value;
    }
    //! Value parser
    /*! \param s String supposed to hold serialized value
        \retval v Result of parsing
	\return Error code as defined in mgrError.h

	This class requires template specialisation for implemenation. 
    */
    virtual m_error_t parseValue(const char *s, T& v) const;

    //! Parse value from parameter string 
    /*! \sa parseValue(const char *s, T& v) const */
    virtual m_error_t readValue(const char *s){
      T v;
      m_error_t err = parseValue(s,v);
      if((ERR_NO_ERROR != err) && (ERR_CANCEL != err)) return err;
      Value = v;
      return err;
    }
      
    virtual int optChar() const { return shortOption; }
    virtual const char *optString() const { return longOption; }
  };

  /*! \class StringOption
      \brief Commandline option taking a string argument, e.g. filenames
      
      \note This class is created instead of ConfigOption<const char *>,
      which would not work and to avoid ConfigOption<std::string>, which
      should do pretty much the same.

      If no distinct identified is passed during creation,
      the long option name is used as identifier. Therefore,
      you should always provide a long option name, unless
      you know what you're doing.
  */
  class StringOption : public ConfigItem {
  protected:
    const char *Value;       //!< Current value, holding a malloc()'ed string or NULL
    const char *Default;     //!< Default value, holding a malloc()'ed string or NULL
    int shortOption;         //!< Option character or non-printable code, if no short option is available
    const char *longOption;  //!< Long option string or NULL if no long option is available
    
    //! Copy a string to the Value field
    /*! \param val String to copy
        \return Error code as defined in mgrError.h

	Frees Value, if it contained a string and
	strdup() the new val to Value, unless val
	is Default, i.e. copyVal(Default) sets the value
	to default, but copyVal("The default value.")
	does not. Value == Default means that the
	option was not provided!	
    */
    m_error_t copyVal(const char *val){
      if(Value && (Value != Default)) ::free(const_cast<char *>(Value));
      if(!val){
	Value = NULL;
	return ERR_NO_ERROR;
      }
      if(val == Default){
	Value = Default;
	return ERR_NO_ERROR;
      }
      Value = strdup(val);
      if(!Value) return ERR_MEM_AVAIL;
      return ERR_NO_ERROR;
    }

  public:
    //! CTOR
    /*! \param sOpt Short option character, or numerical unique sorting index
        \param lOpt Long option string, or NULL is no long option exists
        \param help Help text to be shown in the manual
        \param def  Default string
        \param np   Parameter argument requirement
        \param ident (optional) Sorting index for option, defaults to lOpt

	Options are bound to console input.
    */
    StringOption(int sOpt, const char *lOpt, const char *help, 
		 const char *def, const ParSpec& np = MANDATORY, 
		 const char *ident = NULL)
      : ConfigItem( help, ident, CONSOLE, np ), 
        Value(def), Default(def), shortOption(sOpt), longOption(lOpt) {
      if(!ident) id = lOpt;
    }
    //! Copy CTOR
    StringOption(const StringOption& co) 
      : ConfigItem(co) {
      Default = co.Default;
      copyVal(co.Value);
      shortOption = co.shortOption;
      longOption = co.longOption;
    }

    //! DTOR frees Value, if necessary
    virtual ~StringOption() {
      if(Value && (Value != Default)) ::free(const_cast<char *>(Value));
      Value = NULL;
    }

    //! Get Value
    inline const char *value() const {
      return Value;
    }
    //! Set Value to default
    virtual const char *setValue(){
      copyVal(Default);
      return Value;
    }
    //! Set Value to string
    virtual const char *setValue(const char *val){
      copyVal(val);
      return Value;
    }
    //! Value parser
    /*! \param s String to parse
        \retval v Result of parsing
	\return Error code as defined in mgrError.h

	The parser strips leading spaces. Trailing
	spaces are left untouched. If NULL is passed
	for s, the Default value is returned.
    */
    virtual m_error_t parseValue(const char *s, const char *& v) const{
      if(!s){
	v = Default;
	return ERR_NO_ERROR;
      }
      while(isspace(*s)) s++;
      v = s;
      return ERR_NO_ERROR;
    }
    //! Parse Value from string and set it
    virtual m_error_t readValue(const char *s){
      const char *v;
      m_error_t err = parseValue(s,v);
      if((ERR_NO_ERROR != err) && (ERR_CANCEL != err)) return err;
      copyVal(v);
      return err;
    }
      
    virtual int optChar() const { return shortOption; }
    virtual const char *optString() const { return longOption; }
  };


  /*! \class RangedOption
      \brief ConfigOption, which accepts parameters in a specified range, only
  */
  template<typename T> class RangedOption : public ConfigOption<T> {
  protected:
    T minimum;   //!< Minimum value of option
    T maximum;   //!< Maximum value of option

    /*
     * gcc 3.4.4 has a more conformant scope implemenation, which
     * requires variables in templates to be template dependent to
     * be looked up on instantiation.
     * Otherwise e.g. Value is looked up in the global scope and
     * is therefore not found. To make is template dependent, we
     * define a dependent type, its base class, and explicitly
     * declare its scope.
     *
     */
    typedef ConfigOption<T> ConfigBase;

  public:
    //! CTOR
    /*! \param sOpt Short option character, or numerical unique sorting index
        \param lOpt Long option string, or NULL is no long option exists
        \param help Help text to be shown in the manual
        \param def  Default value
        \param mn   Minimum value
	\param mx   Maximum value
        \param ident (optional) Sorting index for option, defaults to lOpt

	Ranged options take mandatory arguments. Being a ConfigOption they are
	bound to console input.
    */
    RangedOption(int sOpt, const char *lOpt, const char *help, 
		 const T& def, const T& mn, const T& mx,
		 const char *ident = NULL)
      : ConfigOption<T>(sOpt, lOpt, help, def, ConfigItem::MANDATORY, ident), 
	minimum(mn), maximum(mx) {}
    //! Copy CTOR
    RangedOption(const RangedOption& ro) 
      : ConfigOption<T>(ro){
      minimum = ro.minimum;
      maximum = ro.maximum;
    }
    //! DTOR
    virtual ~RangedOption() {}

    //! Set default value
    virtual const T& setValue(){
      ConfigBase::Value = ConfigBase::Default;
      return ConfigBase::Value;
    }
    //! Set specified value
    virtual const T& setValue(const T& val){
      do{
	if(val > maximum){
	  ConfigBase::Value = maximum;
	  break;
	}
	if(val < minimum){
	  ConfigBase::Value = minimum;
	  break;
	}
	ConfigBase::Value = val;
      }while(0);
      return ConfigBase::Value;
    }
    //! Set a new range
    void setRange(const T& mn, const T& mx){
      if(mn > mx){
	minimum = mx;
	maximum = mn;
      } else {
	minimum = mn;
	maximum = mx;
      }
    }
    //! Get current range
    inline void getRange(T& mn, T& mx) const {
      mn = minimum;
      mx = maximum;
    }
    //! Check whether value is in range
    inline bool checkRange(const T& v) const {
      if(v < minimum) return false;
      if(v > maximum) return false;
      return true;
    }
    //! Parse value from string and set Value
    virtual m_error_t readValue(const char *s){
      T v;
      m_error_t err = parseValue(s,v);
      if((ERR_NO_ERROR != err) && (ERR_CANCEL != err)) return err;
      if(!checkRange(v)) return ERR_PARAM_RANG;
      ConfigBase::Value = v;
      return err;
    }
  };

  //! Boolean option with parameters
  typedef ConfigOption<bool> BoolOption;
  //! Integer typed option
  typedef ConfigOption<int> FreeIntegerOption;
  //! Integer typed option with allowed range
  typedef RangedOption<int> IntegerOption;
  //! Double typed option
  typedef ConfigOption<double> FreeDoubleOption;
  //! Double typed option with allowed range
  typedef RangedOption<double> DoubleOption;

  //! Boolean option without parameters
  class SwitchOption : public BoolOption {
  public:
    SwitchOption(int sOpt, const char *lOpt, const char *help, 
		 const char *ident = NULL)
      : BoolOption(sOpt, lOpt, help, false, NO_PARAM, ident) {}
  };

  /*! \class Configuration
      \brief Hierarchical database of configuration items
  */
  class Configuration : public XTree<ConfigItem> {
  protected:
    const char *programName;      //!< argv[0]
    const char *shortForm;        //!< Short help - purpose of the program
    const char *Copyright;        //!< Author and coypright information
    int numFiles;                 //!< Number of non-option arguments
    const char * const *  Files;  //!< Array of non-option arguments

    //! Index by option ID to the database
    std::map <const char *, ConfigItem *, OptIDLess > ItemMap;

    //! this does only append to HTree, but not to ItemMap
    m_error_t __append(ConfigItem *ci);

    //! create the option string for getopt()
    m_error_t createOptString(std::string& opts, 
			      std::vector<struct option>& longopts);

  public:
    //! CTOR initializing internals
    Configuration() : programName(NULL), shortForm(NULL), Copyright(NULL),
      numFiles(0), Files(NULL) {}

    //! Check whether a character is valid as short option
    static inline bool isOptChar(const int& c){
      if(c == '?') return true;
      return isalnum(c);
    }

    //! Parse options passed to the program
    m_error_t parseOptions(int argc, char * const argv[],
			   ConsoleFormatter& cfm);
    //! Print help text
    m_error_t help(ConsoleFormatter& cfm);
    //! Print usage information (no explanation of options)
    m_error_t usage(ConsoleFormatter& cfm);
    //! Version information string
    const char *VersionTag() const;
    //! Get the program name
    /*! \return argv[0]
      
        Requires parseOptions() to be run before.
    */
    inline const char *name() const {
      return programName;
    }
    //! Set short-form information
    void about(const char *s){
      shortForm = s;
    }
    //! Get short-form information
    const char *about() const {
      return shortForm;
    }
    //! Set copyright information
    void copyright(const char *s){
      Copyright = s;
    }
    //! Get copyright information
    const char *copyright() const {
      return Copyright;
    }
    //! Get non-option argument by index
    const char *getFile(const int& i) const {
      if(i>=numFiles) return NULL;
      return Files[i];      
    }
    //! Get number of non-option arguments
    const size_t argFiles() const {
      return numFiles;
    }
    //! Append a configuration item to the database
    m_error_t append(ConfigItem *ci);      
    //! Look-Up database by long-option name
    const ConfigItem *find( const char *long_opt, m_error_t *err = NULL);
    //! Look-Up database by short-option name
    const ConfigItem *find( const int short_opt, m_error_t *err = NULL);
  };

};

#endif // _APP_OPTIONS_H_
