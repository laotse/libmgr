/*
 *
 * Skeleton Server
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: mgrInet.cpp,v 1.6 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *
 * This defines the values:
 *
 */

#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "mgrInet.h"
#include "mgrInet.tag"

#define DEBUG_STATE 1
#define DEBUG (DEBUG_STATE)
#include <mgrDebug.h>

#ifdef DEBUG
# include <stdio.h>
#endif

using namespace mgr;

m_error_t mgr::InetAddress::setMask( int m ){
  xpdbg(STATE,"State is 0x%x, to set 0x%x\n",State,m);
  if(State == SocketAddress::DEFINED) return ERR_NO_ERROR;
  if(State == SocketAddress::BOUND) return ERR_PARAM_LCK;
  m &= ~(SocketAddress::PRIVATE - 1);
  State |= m;
  if((State & (PORT_MASK | IP_MASK)) == (PORT_MASK | IP_MASK))
    State = DEFINED;
  xpdbg(STATE,"State set to 0x%x (0x%x)\n",State,(PORT_MASK | IP_MASK));
  return ERR_NO_ERROR;
}

m_error_t mgr::InetAddress::port(unsigned short p){
  m_error_t err = setMask(PORT_MASK);
  if(err != ERR_NO_ERROR) return err;
  address.sin_port = htons(p);
  return ERR_NO_ERROR;
}

m_error_t mgr::InetAddress::ipv4(const char *s){
  m_error_t err = setMask(IP_MASK);
  if(err != ERR_NO_ERROR) return err;
  if(inet_aton(s,&(address.sin_addr))) return ERR_NO_ERROR;
  return ERR_PARAM_TYP;
}

m_error_t mgr::InetAddress::hostname(const char *s){
  m_error_t err = setMask(IP_MASK);
  if(err != ERR_NO_ERROR) return err;
  if(!s) return ERR_PARAM_NULL;
  const struct hostent *entry = gethostbyname(s);
  if(!entry) return ERR_PARAM_KEY;  
  address.sin_addr = (static_cast<struct in_addr *>(static_cast<void *>(entry->h_addr)))[0];
  return ERR_NO_ERROR;
}

const char * mgr::InetAddress::VersionTag(void){
  return _VERSION_;
}


/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>
#include <errno.h>

/*
 * This is the server code
 *
 */

class ServReady : public Runnable {
protected:
  int fd;
  InetStreamSocket *server;
public:
  virtual ~ServReady() {}
  ServReady(InetStreamSocket *s, int _fd) : fd(_fd), server(s) {}
  virtual m_error_t run(void){
    printf("### Server Hook called!\n");
    if(fd == -1) return ERR_FILE_OPEN;
    if(!server || !(SocketAddress::CONNECTED == server->ready())){
      if(1 != write(fd,"0",1)) return ERR_FILE_WRITE;
      return ERR_PARAM_NULL;
    }
    if(1 != write(fd,"1",1)) return ERR_FILE_WRITE;
    return ERR_NO_ERROR;
  }    
};

m_error_t do_server(InetStreamSocket& socket, int pfd, int tests, int errors, pid_t child){
  m_error_t res = ERR_NO_ERROR;

  ServReady servHook(&socket, pfd);

  printf("Test %d: InetStreamSocket::listen()\n",++tests);
  printf("use nc localhost %d to communicate\n",socket.port());
  InetStreamSocket* server = 
    static_cast<InetStreamSocket *>(socket.listen(&res,&servHook));
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: listen() failed 0x%.4x, System: %d\n",(int)res,errno);
    perror("*** System Error");
    return res;
  } else {
    puts("+++ listen() finished OK!");
  }

  printf("Test %d: InetStreamSocket::write()\n",++tests);  
  const char *Hail = "InetStreamSocket - Test.write()\n";
  size_t len = strlen(Hail);
  res = server->write(Hail,len);
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: write() failed 0x%.4x, System: %d\n",(int)res,errno);
    perror("*** System Error");
  } else {
    printf("??? wrote %u of %d\n",len,strlen(Hail));
    puts("+++ write() finished OK!");
  }
    
  const size_t InBufLen = 512;
  char InBuf[InBufLen];
  printf("Test %d: InetStreamSocket::read()\n",++tests);  
  len = InBufLen;
  res = server->read(InBuf,len);
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: read() failed 0x%.4x, System: %d\n",(int)res,errno);
    perror("*** System Error");
  } else {
    printf("??? read %u characters\n??? ",len);
    fwrite(InBuf,len,1,stdout);
    puts("+++ read() finished OK!");
  }
  
  printf("Test %d: InetStreamSocket::delete\n",++tests);  
  delete server;
  puts("+++ delete finished OK!");
  
  printf("\nServer: %d tests completed with %d errors.\n",tests,errors);

  return ERR_NO_ERROR;
}

/*
 * This is the client code
 *
 */

m_error_t do_client(InetStreamSocket& socket, int pfd, int tests, int errors){
  char c[2];
  puts("### Client forked!");
  // wait for server to come up
  do {
    if(read(pfd,c,1) != 1) *c = 'Q';
  } while((*c != '0') && (*c != '1') && (*c != 'Q'));
  c[1] = 0;
  printf("### Client sync'ed: %s\n",c);
  if(*c == 'Q') return ERR_FILE_READ;
  if(*c == '0') return ERR_CANCEL;

  // Okay, go ...
  printf("Test %dc: InetStreamSocket::connect()\n",++tests);
  m_error_t res = socket.connect();
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: connect() failed 0x%.4x, System: %d\n",(int)res,errno);
    perror("*** System Error");
    return res;
  } else {
    puts("+++ connect() finished OK!");
  }

  const size_t InBufLen = 512;
  char InBuf[InBufLen];
  printf("Test %dc: InetStreamSocket::read()\n",++tests);  
  size_t len = InBufLen;
  res = socket.read(InBuf,len);
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: read() failed 0x%.4x, System: %d\n",(int)res,errno);
    perror("*** System Error");
    return res;
  } else {
    printf("??? read %u characters\n??? ",len);
    fwrite(InBuf,len,1,stdout);
    puts("+++ read() finished OK!");
  }

  printf("Test %dc: InetStreamSocket::write()\n",++tests);  
  const char *Hail = "Yoohoo - I'm the client!\n";
  len = strlen(Hail);
  res = socket.write(Hail,len);
  if(ERR_NO_ERROR != res){
    errors++;
    printf("*** Error: write() failed 0x%.4x, System: %d\n",(int)res,errno);
    perror("*** System Error");
    return res;
  } else {
    printf("??? wrote %u of %d\n",len,strlen(Hail));
    puts("+++ write() finished OK!");
  }

  return ERR_NO_ERROR;
}

/*
 * Common code section and fork
 *
 */

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  try {
    Forker Mother;

    printf("Test %d: InetStreamSocket CTROR\n",++tests);
    InetStreamSocket socket;
    puts("+++ CTOR finished OK!");
    
    printf("Test %d: InetStreamSocket::hostname()\n",++tests);
    res = socket.hostname("localhost");
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: hostname() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ hostname() finished OK!");
    }

    const unsigned short pNum = 60179;
    printf("Test %d: InetStreamSocket::port()\n",++tests);
    res = socket.port(pNum);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: port() failed 0x%.4x\n",(int)res);
    } else {
      puts("+++ port() finished OK!");
    }
    
    printf("Test %d: InetStreamSocket::ipv4()\n",++tests);
    unsigned long ip = socket.ipv4();
    // 127.0.0.1
    const unsigned long loc_ip = (127 << (3*8)) + 1;
    if((ip != loc_ip) || (pNum != socket.port())){
      errors++;
      printf("*** Error: Wrong address %d.%d.%d.%d:%d\n",
	     (int)((ip >> (3*8)) & 0xff),
	     (int)((ip >> (2*8)) & 0xff),
	     (int)((ip >> (1*8)) & 0xff),
	     (int)((ip >> (0*8)) & 0xff),
	     socket.port());
    } else {
      printf("??? Address %d.%d.%d.%d:%d\n",
	     (int)((ip >> (3*8)) & 0xff),
	     (int)((ip >> (2*8)) & 0xff),
	     (int)((ip >> (1*8)) & 0xff),
	     (int)((ip >> (0*8)) & 0xff),
	     socket.port());
      puts("+++ ipv4() finished OK!");
    }
      
    // Setup pipe for syncronizing server and client
    int pfd[2];
    if(::pipe(pfd)){
      printf("*** Error: pipe() failed %d\n",errno);
      perror("*** System Error");
      mgrThrow(ERR_FILE_OPEN);      
    }

    pid_t child = -1;
    res = Mother.fork( child );
    if(res != ERR_NO_ERROR){
      printf("*** Error: fork() failed 0x%.4x\n",(int)res);
      perror("*** System Error");
    } else {
      if(child == 0){
	res = do_client(socket,pfd[0],tests,errors);
	printf("+++ Client exits 0x%.4x\n",(int)res);
      } else {
	res = do_server(socket,pfd[1],tests,errors,child);
	printf("+++ Server exits 0x%.4x\n",(int)res);
	res = Mother.wait(5000);
	if(ERR_CANCEL == res){
	  puts("*** Have Zombies - kill'em");
	  if(ERR_NO_ERROR == Mother.killChildren()){
	    res = Mother.wait(1000);
	    if(res != ERR_NO_ERROR)
	      printf("*** Error killing Zombies 0x%.4x\n",(int)res);
	  }
	}
	printf("+++ Mother exits 0x%.4x\n",(int)res);
	printf("Used version: %s\n",socket.VersionTag());
      }	
    }
      
  }
  catch(Exception& e){
    printf("*** Caught mgr::Exception: %s\n",e.what());
  }

  return 0;  
}

#endif //TEST
