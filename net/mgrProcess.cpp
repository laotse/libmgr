/*
 *
 * Basic classes for managing processes, threads, forks, subroutines, etc.
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: mgrProcess.cpp,v 1.6 2008-05-15 20:58:24 mgr Exp $
 *
 * This defines the classes:
 *     Runnable - generic subroutine interface
 *
 * This defines the values:
 *
 */

#include "mgrProcess.h"
#include "mgrProcess.tag"
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define DEBUG_STATE 1
#define DEBUG (DEBUG_STATE)
#include <mgrDebug.h>

#ifdef DEBUG
# include <stdio.h>
#endif

using namespace mgr;

// initialise global static
ForkRoot *mgr::ForkRoot::Master = NULL;

void mgr::ForkRoot::mourn(int sig){
  pid_t pid;
  if(!Master) return;
  int status;
  while((pid = ::waitpid(-1,&status,WNOHANG)) > 0){
    Master->children.erase(pid);
  }
}

void mgr::ForkRoot::wake(int sig){
  if(!Master) return;
  Master->waiting = false;
}

m_error_t mgr::ForkRoot::start( void ){
  if(handlerInstalled) return ERR_NO_ERROR;
  struct sigaction reaper;
  bzero(&reaper,sizeof(reaper));
  reaper.sa_handler = mourn;
  sigemptyset(&(reaper.sa_mask));
  reaper.sa_flags = SA_NOCLDSTOP;
  if(-1 == ::sigaction(SIGCHLD,&reaper,&old_reaper)) return ERR_MEM_SIG;
  handlerInstalled = true;
  return ERR_NO_ERROR;
}

m_error_t mgr::ForkRoot::stop( void ){
  if(!handlerInstalled) return ERR_NO_ERROR;
  if(-1 == ::sigaction(SIGCHLD,&old_reaper,NULL)) return ERR_MEM_SIG;
  handlerInstalled = false;
  return ERR_NO_ERROR;
}

m_error_t mgr::ForkRoot::fork( pid_t& pid ){
  m_error_t res = start();
  if(res != ERR_NO_ERROR){
    pid = -1;
    return res;
  }
  pid = ::fork();
  if(-1 == pid) return ERR_MEM_FORK;
  if(!pid){
    // in the child we are freshly created
    waiting = false;
    children.clear();
    // RefCount Objects are also forked and will run out of scope
    // so no treatment for RefCounts
    return stop();
  }
  children.insert(pid);
  return ERR_NO_ERROR;
}

m_error_t mgr::ForkRoot::wait( unsigned long microseconds ){
  struct itimerval otimer,ntimer;
  struct sigaction owait, nwait;
  
  if(!hasChildren()) return ERR_NO_ERROR;
  m_error_t res = start();
  if(res != ERR_NO_ERROR) return res;
  // setup timer and signal handler
  ntimer.it_value.tv_usec = microseconds % 1000;
  ntimer.it_value.tv_sec = microseconds / 1000;
  ntimer.it_interval.tv_usec = 0;
  ntimer.it_interval.tv_sec = 0;
  bzero(&nwait,sizeof(nwait));
  nwait.sa_handler = wake;
  sigemptyset(&(nwait.sa_mask));
  nwait.sa_flags = SA_ONESHOT;
  if(-1 == ::setitimer(ITIMER_REAL,&ntimer,&otimer)) return ERR_MEM_TIME;
  waiting = true;
  if(-1 == ::sigaction(SIGALRM,&nwait,&owait)) return ERR_MEM_SIG;
  while(waiting && hasChildren())
    ::pause();
  if(-1 == ::sigaction(SIGALRM,&owait,NULL)) return ERR_MEM_SIG;
  if(-1 == ::setitimer(ITIMER_REAL,&otimer,NULL)) return ERR_MEM_TIME;
  if(hasChildren()) return ERR_CANCEL;
  return stop();
}

m_error_t mgr::ForkRoot::killChildren( void ) const {
  if(!hasChildren()) return ERR_NO_ERROR;
  for(child_set_t::const_iterator it = children.begin();
      it != children.end();
      ++it){
    // consider error handling (returns -1)
    kill(*it,SIGKILL);
  }
  return ERR_NO_ERROR;
}

m_error_t Spawner::close_fd(int& fd){
  int retry = 10;
  while((fd != -1) && (-1 == close(fd))) switch(errno){
  default:
    // this is EIO
    return ERR_FILE_CLOSE;
  case EINTR:
    // retry
    if(!(--retry)) return ERR_FILE_CLOSE;
    break;
  case EBADF:
    // fd is invalid - no reason to close
    fd = -1;
    return ERR_NO_ERROR;
  }
  // everything OK!
  fd = -1;
  return ERR_NO_ERROR;
}

m_error_t Spawner::spawn( const char *cmd ){
  if(!cmd) return ERR_PARAM_NULL;
  if(-1 != Process) return ERR_PARAM_LCK;
  int pfd[2];
  if(pipe( pfd )) return ERR_FILE_SOCK;
  Parent.read = pfd[0];
  Child.write = pfd[1];
  if(pipe( pfd )){
    close_fids(Parent.read,Child.write);
    return ERR_FILE_SOCK;
  }
  Child.read = pfd[0];
  Parent.write = pfd[1];
  m_error_t res = this->fork( Process );
  if(res != ERR_NO_ERROR ){
    Process = -1;
    close_fids(Parent);
    close_fids(Child);
    return res;
  }
  if( Process != 0 ){
    // Parent process, close child fds and we're done
    return close_fids(Child);
  }
  res = close_fids(Parent);
  if(res != ERR_NO_ERROR) ::exit(res);
  res = ERR_FILE_OPEN;
  do {
    if(STDIN_FILENO != ::dup2(Child.read,STDIN_FILENO))
      break;
    if(STDOUT_FILENO != ::dup2(Child.write,STDOUT_FILENO))
      break;	
    res = close_fids(Child);
  } while(0);
  if(res != ERR_NO_ERROR) ::exit(res);
  // re-arm Object
  Process = -1;
  /*
   * stdio is now connected to Parent
   * Signal Handler for SIGCHLD is reset by Forker
   * Redundant fds are closed
   * now replace process image
   *
   */
  const char *shopt = (Restrict)? "-rc" : "-c";
  execl(Shell,"sh",shopt,cmd,(const char *)NULL);
  // oops, execve() did fail
  ::exit(ERR_FILE_EXEC);
}      

m_error_t Spawner::sync( void ){
  if(Process == -1) return ERR_CANCEL;
  close_fids(Parent);
  pid_t cpid;
  while(Process != (cpid = ::waitpid(Process,&Status,0))){
    if(cpid != -1) continue;
    switch(errno){
    default:
      return ERR_INT_STATE;
    case EINTR:
      // retry
      break;
    case ECHILD:
      // has already been killed
      Process = -1;
      Status = (int)ERR_CANCEL;
      return ERR_NO_ERROR;
    }
  }
  Process = -1;
  return ERR_NO_ERROR;
}

m_error_t Spawner::read( void *buf, size_t& len ){
  if(!buf) return ERR_PARAM_NULL;
  if(Process == -1) return ERR_PARAM_LCK;
  if(Parent.read == -1) return ERR_FILE_STAT;
  ssize_t rd = ::read(Parent.read,buf,len);
  if(rd > 0){
    len = (size_t)rd;
    return ERR_NO_ERROR;
  }
  if(rd == 0) return ERR_FILE_END;
  return ERR_FILE_READ;
}

m_error_t Spawner::write( const void *buf, size_t& len ){
  if(!buf) return ERR_PARAM_NULL;
  if(Process == -1) return ERR_PARAM_LCK;
  if(Parent.write == -1) return ERR_FILE_STAT;
  ssize_t rd = ::write(Parent.write,buf,len);
  if(rd == (ssize_t)len) return ERR_NO_ERROR;
  if(rd >= 0){
    len = (size_t)rd;
    return ERR_CANCEL;
  }
  return ERR_FILE_WRITE;
}

const char * mgr::Spawner::VersionTag(void) const {
  return _VERSION_;
}

/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  try {
    printf("Test %d: Spawner CTOR\n",++tests);
    Spawner Proc;
    puts("+++ CTOR finished OK!");

    printf("Test %d: spawn()\n",++tests);
    //res = Proc.spawn("tee test-spawn-thru.log");
    res = Proc.spawn("sed -e 's/x/u/g'");
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: spawn() failed 0x%.4x, System: %d\n",(int)res,errno);
      perror("*** System Error");
    } else {
      puts("+++ spawn() finished OK!");
    }

    printf("Test %d: write()\n",++tests);
    const char *tstr = "Ein x für ein u vormachen.\n";
    size_t sz = strlen(tstr);
    res = Proc.write(tstr,sz);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: write() failed 0x%.4x, System: %d\n",(int)res,errno);
      perror("*** System Error");
    } else {
      puts("+++ write() finished OK!");
    }

    // this is to flush sed by sending EOF
    printf("Test %d: detachWrite()\n",++tests);
    res = Proc.detachWrite();
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: detachWrite() failed 0x%.4x, System: %d\n",
	     (int)res,errno);
      perror("*** System Error");
    } else {
      puts("+++ detachWrite() finished OK!");
    }

    printf("Test %d: read()\n",++tests);
    const size_t blen = 1024;
    char buf[blen+1];
    sz = blen;
    res = Proc.read(buf,sz);
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: %d=read() failed 0x%.4x, System: %d\n",
	     sz,(int)res,errno);
      perror("*** System Error");
    } else {
      buf[sz] = 0;
      printf("??? %s",buf);
      puts("+++ read() finished OK!");
    }

    printf("Test %d: detach()\n",++tests);
    res = Proc.detach();
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: detach() failed 0x%.4x, System: %d\n",
	     (int)res,errno);
      perror("*** System Error");
    } else {
      puts("+++ detach() finished OK!");
    }

    printf("Test %d: sync()\n",++tests);
    res = Proc.sync();
    if(ERR_NO_ERROR != res){
      errors++;
      printf("*** Error: sync() failed 0x%.4x, System: %d\n",
	     (int)res,errno);
      perror("*** System Error");
    } else {
      puts("+++ sync() finished OK!");
    }

    printf("\n%d tests completed with %d errors.\n",tests,errors);
    printf("Used version: %s\n",Proc.VersionTag());
  }
  catch(Exception& e){
    printf("*** Caught mgr::Exception: %s\n",e.what());
  }

  return 0;  
}

#endif //TEST
