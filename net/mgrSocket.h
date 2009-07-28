/*
 *
 * Generic Socket Handling
 *  - You'll need specialised SocketAddress classes for the Socket template
 *    these are defined in different files
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: mgrSocket.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *   SocketAddress - mother of all socket addresses (non instantiable)
 *   RawSocket     - mother of all sockets (non instantiable)
 *   StreamSocket  - SOCK_STREAM socket for use with Socket<> template
 *   Socket        - template class for the actual socket, requires
 *                   a RawSocket heir and a SocketAddress heir
 *
 * This defines the values:
 *
 */

#ifndef _NET_SOCKET_H_
# define _NET_SOCKET_H_

#include <unistd.h>
#include <string.h>
#include <mgrError.h>
#include <mgrProcess.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>

namespace mgr {

  // SockAddr is a state machine for initialisation of socket addresses
  // this object shall always be used through inheritance
  class SocketAddress {
  public:
    enum e_addr_state {
      UNDEFINED = 0,
      DEFINED   = 1,
      BOUND     = 2,
      CONNECTED = 3,
      PRIVATE   = (1<<3)
      // following states are left for children
    };
    typedef enum e_addr_state eAddressState;
    inline eAddressState ready( void ) const {
      switch( State ){
	// default is fall through
      case BOUND:
      case DEFINED:
      case CONNECTED:
	return static_cast<eAddressState>(State);
      }
      return UNDEFINED;
    }

  protected:
    // eAddressState State;
    int State;
    // we do not want users create pure SockAddr objects
    SocketAddress() : State( UNDEFINED ) {}

    inline eAddressState cloneState( const SocketAddress* a ) const {
      eAddressState res = a->ready();
      if(res > DEFINED) res = DEFINED;
      return res;
    }
    virtual SocketAddress& clone(const SocketAddress* a) {
      State = cloneState(a);      
      return *this;
    }

  public:
    SocketAddress( const SocketAddress& a) { clone(&a); }
    SocketAddress& operator=(const SocketAddress& a){ clone(&a); return *this; }
    virtual ~SocketAddress() {}
  };

  // This is a tier for socket related functions
  // it shall always be used through inheritance
  class RawSocket {
  protected:
    int sock;
    RawSocket() : sock(-1) {}
    virtual ~RawSocket(){ 
      if(-1 != sock) ::close(sock); 
      //printf("### Closed Socket %d\n",sock);
    }

  public:
    virtual m_error_t connect( void ) = 0;
    virtual RawSocket *listen( m_error_t *err = NULL, 
			       Runnable *hook = NULL, 
			       int queue = 3 ) = 0;
    virtual m_error_t close( void ){
      if(-1 != sock) if(::close(sock)) {
	return ERR_FILE_CLOSE;
      }  else { 
	sock = -1;
      }
      return ERR_NO_ERROR;
    }
    inline int fd( m_error_t *err = NULL ){
      if(-1 != sock) {
	int dfd = dup( sock );
	if(err) *err = (dfd == -1)? ERR_FILE_OPEN : ERR_NO_ERROR;
	return dfd;
      }
      if(err) *err = ERR_INT_SEQ;
      return -1;
    }
    inline m_error_t read(void *buffer, size_t& len){
      ssize_t slen = recv(sock, buffer, len, 0);
      if(slen <= 0){
	len = 0;
	return ERR_FILE_READ;
      }
      len = static_cast<size_t>(slen);
      return ERR_NO_ERROR;
    }
    inline m_error_t write(const void *buffer, size_t& len){
      ssize_t slen = send(sock, buffer, len, 0);
      if(slen <= 0){
	len = 0;
	return ERR_FILE_READ;
      }
      len = static_cast<size_t>(slen);
      return ERR_NO_ERROR;
    }
  };

  class StreamSocket : public RawSocket {
  public:
    enum { SOCK_TYPE = SOCK_STREAM };
    FILE *file( m_error_t *err = NULL ){
      int dfd = fd(err);
      if(dfd == -1) return NULL;
      FILE *f = fdopen(dfd, "r+");
      if(err) *err=(f)? ERR_NO_ERROR : ERR_FILE_OPEN;
      return f;
    }
  };

  template<class SType, class AType> class Socket : public SType, public AType {
  protected:
    int Protocol;

  public:
    Socket(const int& protocol = 0) : Protocol( protocol ) {}
    Socket(const AType& a, const int& protocol = 0) : AType(a), Protocol( protocol ) {}
    Socket(const Socket& s) : SType(s), AType(s), Protocol( s.Protocol ) {}
    typedef Socket<SType,AType> SocketType;

    virtual m_error_t connect( void ){
      if(SocketAddress::DEFINED > this->ready()) return ERR_INT_SEQ;
      if(-1 == this->sock ) {
	this->sock = ::socket(AType::NET_TYPE, SType::SOCK_TYPE, Protocol);
	if(-1 == this->sock) return ERR_FILE_SOCK;
	this->State = SocketAddress::DEFINED;
      }
      if(SocketAddress::DEFINED < this->ready()) return ERR_INT_SEQ;
      if(-1 == ::connect(this->sock, 
			 (struct sockaddr *)&(this->address), 
			 sizeof(this->address)))
	 return ERR_FILE_OPEN;

      this->State = SocketAddress::CONNECTED;

      return ERR_NO_ERROR;
    }

    virtual RawSocket *listen( m_error_t *err = NULL, Runnable *hook = NULL, int queue = 3 ){
      m_error_t ferr, &ierr = (err)? *err : ferr;
      if((SocketAddress::BOUND != this->ready()) || (this->sock == -1)){
	// bypass initialisation if already connected
	if(SocketAddress::DEFINED > this->ready()){
	  ierr = ERR_INT_SEQ;
	  return NULL;
	}
	if(-1 == this->sock ) {
	  this->sock=::socket(AType::NET_TYPE, SType::SOCK_TYPE, this->Protocol);
	  if(-1 == this->sock){
	    ierr = ERR_FILE_SOCK;
	    return NULL;
	  }
	  //printf("### Created server socket %d\n",this->sock);
	  this->State = SocketAddress::DEFINED;
	}
	if(SocketAddress::BOUND > this->ready()){
	  if(-1 == ::bind(this->sock,
			  (struct sockaddr *)&(this->address),
			  sizeof(this->address))){
	    ierr = ERR_FILE_STAT;
	    return NULL;
	  }
	  this->State = SocketAddress::BOUND;
	}
      } // end of bypass for already connected sockets
      if(SocketAddress::CONNECTED > this->ready()){
	if(-1 == ::listen(this->sock, queue)){
	  ierr = ERR_FILE_OPEN;
	  return NULL;
	}
	this->State = SocketAddress::CONNECTED;
      }
      SocketType *client = new SocketType(*this);
      if(!client){
	ierr = ERR_MEM_AVAIL;
	return NULL;
      }
      if(hook){
	// notify we're falling asleep ...
	ierr = hook->run();
	if(ierr != ERR_NO_ERROR) return NULL;
      }
      do {
	socklen_t slen = sizeof(client->address);      
	client->sock = ::accept(this->sock,
				(struct sockaddr *)&(client->address),
				&slen);
	if(client->sock == -1){
	  switch(errno){
	  case EINTR:
	  case ECHILD:
	    break;
	  default:
	    delete client;
	    ierr = ERR_FILE_READ;
	    return NULL;
	  }
	}
      } while(client->sock == -1);
      //printf("### Created client socket %d\n",client->sock);
      client->State = SocketAddress::CONNECTED;
      // re-arm for next call
      this->State = SocketAddress::BOUND;
      ierr = ERR_NO_ERROR;
      return static_cast<RawSocket *>(client);
    }

    virtual m_error_t close( void ){
      m_error_t ierr = SType::close();
      if(ERR_NO_ERROR != ierr) return ierr;
      if(SocketAddress::DEFINED < AType::ready()) AType::State = SocketAddress::DEFINED;
      return ERR_NO_ERROR;
    }
  };

};

#endif // _NET_SOCKET_H_
