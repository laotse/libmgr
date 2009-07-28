/*
 *
 * Basic classes for managing processes, threads, forks, subroutines, etc.
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: mgrProcess.h,v 1.5 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *     Runnable - generic subroutine interface
 *
 * This defines the values:
 *
 */

#ifndef _NET_PROCESS_H_
# define _NET_PROCESS_H_

#include <mgrError.h>
#include <set>
#include <unistd.h>
#include <csignal>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>

namespace mgr {

  class Runnable {
  protected:
    virtual ~Runnable() {}

  public:
    virtual m_error_t run(void) = 0;
  };

  /*
   * ForkRoot is a unique object
   * create by calling the static register()
   * destroy() wraps the DTOR
   *
   */
  class ForkRoot {
  private:
    typedef std::set<pid_t, std::less<pid_t> > child_set_t;
    typedef void (*sighandler_t)(int);
    child_set_t children;
    volatile bool waiting;
    bool handlerInstalled;
    struct sigaction old_reaper;
    size_t RefCount;

    static ForkRoot *Master;
    static void mourn(int sig);
    static void wake(int sig);

    m_error_t start( void );
    m_error_t stop( void );

    ForkRoot() : waiting(false), handlerInstalled(false), 
		 RefCount(1){ Master = this; }
    ~ForkRoot(){
      if(this != Master) return;
      stop();
      Master = NULL;      
    }
    ForkRoot( const ForkRoot& ) {}
    ForkRoot& operator=( const ForkRoot& ) { return *this; }

  public:
    static ForkRoot *create( void ){
      if(Master) {
	++(Master->RefCount);
      } else Master = new ForkRoot;
      return Master;
    }
    void destroy( void ){
      if(!Master) return;
      if(RefCount && --RefCount) return;
      delete this;
    }
           
    inline bool hasChildren(void) const {
      return !children.empty();
    }

    m_error_t fork( pid_t& pid );
    m_error_t wait( unsigned long microseconds );
    m_error_t killChildren( void ) const;
  };

  class Forker {
  protected:
    ForkRoot *root;
    Forker& operator=( const Forker& f ){ return *this; }
  public:
    Forker() {
      root = ForkRoot::create();
      if(!root) mgrThrow(ERR_CLS_CREATE);
    }
    ~Forker() {
      root->destroy();
    }
    Forker( const Forker& ){
      root = ForkRoot::create();
      if(!root) mgrThrow(ERR_CLS_CREATE);
    }

    inline m_error_t fork( pid_t& pid ){
      return root->fork(pid);
    }
    inline m_error_t wait( unsigned long microseconds ){
      return root->wait(microseconds);
    }
    inline m_error_t killChildren( void ) const {
      return root->killChildren();
    }    
  };

  class Spawner : protected Forker {
  protected:
    class Fid_t {
    public:
      int read;
      int write;
      Fid_t() : read(-1), write(-1) {};
    };

    pid_t Process;
    int Status;
    Fid_t Parent;
    Fid_t Child;
    bool Restrict;
    const char *Shell;

    m_error_t close_fd(int& fd);

    m_error_t close_fids(int& fd1, int& fd2){
      m_error_t resr = close_fd(fd1);
      m_error_t resw = close_fd(fd2);
      if(ERR_NO_ERROR != resr) return resr;
      return resw;
    }

    m_error_t close_fids(Fid_t& f){
      return close_fids(f.read,f.write);
    }
      
  public:
    Spawner() : Process(-1), Status(0),
		Restrict(true), Shell("/bin/sh") {}
    Spawner(const Spawner& s) : Process(-1), Status(0),
		Restrict(s.Restrict), Shell(s.Shell) {}
    Spawner& operator=(const Spawner& s){
      Restrict = s.Restrict;
      Shell = s.Shell;
      return *this;
    }

    inline bool restrict(void) { return Restrict; }
    inline bool restrict(bool r) { return (Restrict = r); }

    m_error_t spawn( const char *cmd );    
    m_error_t sync( void );
    m_error_t detach( void ){
      if(Process == -1) return ERR_PARAM_LCK;
      return close_fids(Parent);
    }
    m_error_t detachWrite( void ){
      if(Process == -1) return ERR_PARAM_LCK;
      return close_fd(Parent.write);
    }
    m_error_t kill( void ){
      if(Process == -1) return ERR_PARAM_LCK;
      return ::kill(Process, SIGKILL)? ERR_MEM_SIG : ERR_NO_ERROR;
    }
    m_error_t read( void *buf, size_t& len );
    m_error_t write( const void *buf, size_t& len );
    const char * VersionTag(void) const;
  };
};

#endif // _NET_PROCESS_H_
