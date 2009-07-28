/*
 *
 * xdrCall - basic RPC framework
 *
 * (c) 2007 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: xdrCall.h,v 1.1 2007-06-25 11:07:16 mgr Exp $
 *
 * This defines the classes:
 *
 *
 * This defines the values:
 *
 */

/*! \file xdrCall.h
    \brief A basic remote procedure call (RPC) framework

    RPC functionality is useful in a variety of fields. Starting
    with user the conventional RPC services such as NFS, through
    management of user interfaces, e.g. the signal / slot model of
    Qt, convenient unit testing, to Web-Service technologies such 
    as SOAP.

    xdrCall was written with the last two use cases in mind, but 
    should be as well for any other RPC use. The more interesting
    question is rather, why yet another RPC layer?

    The answer is quite simple. SOAP has all it takes, but it is
    by far too heavy weight, if web-service type access is not
    targetted. Otherwise, if it is targetted, security must be
    implemented on the application layer, which is cloase to
    unbearable, if many requests shall be handled.

    All other RPC schemes, which I know of, are bound too tightly
    to a special purpose. The purpose of xdrCall is to provide a
    generic interface, where frontends for all established schemes
    and all you might invent can be easily attached.

    \author Dr. Lars Hanke
    \date 2007
*/

#ifndef _XDR_XDRCALL_H_
# define _XDR_XDRCALL_H_

#include <stdlib.h>
#include <mgrError.h>
#include <Concepts.h>
#include "xdrOrder.h"

namespace mgr {

  class _RPCParameter : public sol::HTree::Node {
  protected:
    bool initialized;
    const char *name;

  public:
    _RPCParameter( const char * _name = NULL ) : initialized( false ) {
      if(_name){
	name = strdup(_name);
	if(!name) mgrThrow( ERR_MEM_AVAIL );
      } else {
	name = NULL;
      }
    }
    _RPCParameter( const _RPCParameter& r) : initialized( r.initialized ) {
      if(r.name){
	name = strdup(r.name);
	if(!name) mgrThrow( ERR_MEM_AVAIL );
      } else {
	name = NULL;
      }
    }
    virtual ~_RPCParameter() {
      if(name){
	::free(name);
	name = NULL;
      }
    }

    inline const char *Name() const { return name; }
    inline const bool is_initialized() const { return initialized; }
    
    virtual mgr_error_t serialize(void *buffer, size_t length) = 0;
    virtual mgr_error_t import(void *buffer, size_t length) = 0;

  };

  template <class T> class RPCParameter : public class _RPCParameter {
  protected:
    T value;

  public:
    RPCParameter( const char * _name = NULL ) : _RPCParemeter(_name) {}
    RPCParameter( T _v, const char * _name = NULL ) : _RPCParemeter(_name), value(_v) { initialized = true; }
    RPCParameter( const RPCParameter& r) : _RPCParameter( r ), value(r.value) {}
    virtual ~RPCParamter() {}    

    virtual sol::Cloneable *clone() {
      return new RPCParameter<T>(*this);      
    }

  };
  


  class RPCBroker {
    const char * VersionTag(void) const;
  };

}; // namespace mgr
#endif // _XDR_XDCALL_H_
