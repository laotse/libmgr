/*
 *
 * IP related socket addressing
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: mgrInet.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *
 * This defines the values:
 *
 */

#ifndef _NET_INET_H_
# define _NET_INET_H_

#include <mgrSocket.h>
#include <netinet/in.h>


namespace mgr {
  // Inet Sockets
  class InetAddress : public SocketAddress {
  protected:
    struct sockaddr_in address;
    enum {
      PORT_MASK = SocketAddress::PRIVATE,
      IP_MASK = SocketAddress::PRIVATE << 1
    };
    enum { NET_TYPE = PF_INET };
    m_error_t setMask( int m );
    // each SocketAddress must have protected:clone()
    virtual SocketAddress& clone(const SocketAddress* a){
      const InetAddress *ia = static_cast<const InetAddress *>(a);
      memcpy(&address,&(ia->address),sizeof(address));
      // don't forget to clone state information correctly
      SocketAddress::clone(a);
      return *this;
    }

  public:
    InetAddress() { 
      bzero(&address,sizeof(address));
      address.sin_family = AF_INET; 
      //memset(address.sin_zero,0,sizeof(address.sin_zero));
    }
    InetAddress(const InetAddress& a){
      State = cloneState( &a );
      memcpy(&address,&(a.address),sizeof(address));
    }

    unsigned short port(void) const {
      return ntohs(address.sin_port);
    }
    unsigned long ipv4(void) const {
      return ntohl(address.sin_addr.s_addr);
    }

    m_error_t port(unsigned short p);
    m_error_t ipv4(const char *s);
    m_error_t hostname(const char *s);

    const char * VersionTag(void);
  };

  typedef Socket<StreamSocket, InetAddress> InetStreamSocket;
};

#endif // _NET_INET_H_
