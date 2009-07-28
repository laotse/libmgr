/*
 *
 * Tagged Data File as an exchange standard for binary data,
 * which draws on BER for tagging,
 * on ASN.1 where applicable, and
 * on RFC 1832 - Sun XDR for some storage formats
 *
 * (c) 2006 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: asn1IO.cpp,v 1.7 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * TaggedDataFile - class for organisation of data into scopes
 *
 * This defines the values:
 *
 */

#include "asn1IO.h"
#include "asn1IO.tag"

#include <string.h>

#define DEBUG_MARK 1
#define DEBUG_DUMP 2

#define DEBUG (DEBUG_MARK | DEBUG_DUMP)
#ifdef DEBUG
# include <stdio.h>
# include <HexDump.h>
#endif
#include <mgrDebug.h>

using namespace mgr;

#ifdef DEBUG
# ifdef EXT_DEBUG
extern FileDump dbgStream;
extern HexDump dbgHex;
# else
FileDump dbgStream(DEBUG_LOG);
HexDump dbgHex(&dbgStream);
# endif
#endif

// this is cygwin
#ifdef __CYGWIN__
inline static const time_t& gmtoff(const struct tm*){
  tzset();
  return _timezone;
}
inline static const time_t &gmtoff(const struct tm*, const time_t& off){
  return off;
}
#else
inline static const time_t& gmtoff(const struct tm* t){
  return t->tm_gmtoff;
}
inline static const time_t& gmtoff(struct tm* t, const time_t& off){
  t->tm_gmtoff = off;
  return off;
}
#endif

// These are non vanishing buffers for the BerTag::wtBuffer
const unsigned char ASN1IO::asn1_int[intTag::SIZE] = {
  BerContentTag::TagByte<(int)ASN1_INTEGER, 0,
  BerContentTag::BER_PRIMITIVE, BerContentTag::BER_UNIVERSAL >::VALUE };

const unsigned char ASN1IO::asn1_ia5[ia5Tag::SIZE] = {
  BerContentTag::TagByte<(int)ASN1_IA5_STRING, 0,
  BerContentTag::BER_PRIMITIVE, BerContentTag::BER_UNIVERSAL >::VALUE };

const unsigned char ASN1IO::asn1_print[printTag::SIZE] = {
  BerContentTag::TagByte<(int)ASN1_PRINTABLE_STRING, 0,
  BerContentTag::BER_PRIMITIVE, BerContentTag::BER_UNIVERSAL >::VALUE };

const unsigned char ASN1IO::asn1_utc[utcTag::SIZE] = {
  BerContentTag::TagByte<(int)ASN1_UTC_TIME, 0,
  BerContentTag::BER_PRIMITIVE, BerContentTag::BER_UNIVERSAL >::VALUE };


const char * ASN1IO::VersionTag(void) const {
  return _VERSION_;
}

m_error_t ASN1IO::readPrintableString(const BerTag& tag, char *s, const size_t& l) const {
  if(!s) return ERR_PARAM_NULL;
  if(!printTag::isEqual(tag.tag().readPtr())) return ERR_PARAM_TYP;
  size_t cl = tag.c_size();
  if(l<cl+1) return ERR_PARAM_LEN;  
  const char *c = static_cast<const char *>((void *)(tag.content().readPtr()));
  if(!cl){
    *s = 0;
    return ERR_NO_ERROR;
  }
  if(!c) return ERR_PARAM_UDEF;
  if(cl) memcpy(s,c,cl);
  s[cl] = 0;
  return ERR_NO_ERROR;
}

BerTag *ASN1IO::writePrintableString(const char *s, m_error_t *err) const {
  size_t cl = (s)? strlen(s) : 0;
  if(err) *err = ERR_MEM_AVAIL;
  BerTag *tag = new BerTag(asn1_print,printTag::SIZE);
  if(!tag) return tag;
  if(cl){
    char *c = static_cast<char *>((void *)(tag->allocate(cl)));
    if(!c){
      delete tag;
      return NULL;
    }
    memcpy(c,s,cl);
  }
  if(err) *err = ERR_NO_ERROR;
  
  return tag;
}

BerTag *ASN1IO::writeIA5String(const char *s, m_error_t *err) const {  
  size_t cl = (s)? strlen(s) : 0;
  if(cl){
    const char *t =s;
    while(*t && !(*t & 0x80)) t++;
    if(*t){
      if(err) *err = ERR_PARAM_RANG;
      return NULL;
    }
  }
  if(err) *err = ERR_MEM_AVAIL;
  BerTag *tag = new BerTag(asn1_ia5,ia5Tag::SIZE);
  if(!tag) return tag;
  if(cl){
    char *c = static_cast<char *>((void *)(tag->allocate(cl)));
    if(!c){
      delete tag;
      return NULL;
    }
    memcpy(c,s,cl);
  }
  if(err) *err = ERR_NO_ERROR;

  return tag;
}

m_error_t ASN1IO::readIA5String(const BerTag& tag, char *s, const size_t& l) const {
  if(!s) return ERR_PARAM_NULL;
  if(!ia5Tag::isEqual(tag.tag().readPtr())) return ERR_PARAM_TYP;
  size_t cl = tag.c_size();
  if(l<cl+1) return ERR_PARAM_LEN;  
  const char *c = static_cast<const char *>((void *)(tag.content().readPtr()));
  if(!cl){
    *s = 0;
    return ERR_NO_ERROR;
  }
  if(!c) return ERR_PARAM_UDEF;
  m_error_t res = ERR_NO_ERROR;
  while(cl){
    *s = *c++;
    if(*s++ & 0x80) res = ERR_PARAM_RANG;
    cl--;
  }
  *s = 0;
  return res;
}

static unsigned int atoin(const char *a, size_t l, size_t *bl, m_error_t *err){
  if(l > *bl){
    *bl = 0;
    *err = ERR_PARAM_LEN;
    return 0;
  }
  unsigned int r = 0;
  size_t ll = l;
  while(l--){
    r *= 10;
    int d = *a++ - '0';
    if((d < 0) || (d > 9)){
      *err = ERR_PARAM_RANG;
      return 0;
    }
    r += d;
  }
  *bl -= ll;
  *err = ERR_NO_ERROR;
  return r;
}

m_error_t ASN1IO::readUTC(const BerTag& tag, struct tm *tm){
  if(!tm) return ERR_PARAM_NULL;
  memset(tm,0,sizeof(struct tm));
  if(!utcTag::isEqual(tag.tag().readPtr())) return ERR_PARAM_TYP;
  size_t cl = tag.c_size();
  if(!cl) return ERR_PARAM_UDEF;
  const char *c = static_cast<const char *>((void *)(tag.content().readPtr()));
  if(!c) return ERR_PARAM_UDEF;
  m_error_t res = ERR_NO_ERROR;
  int dv = atoin(c,4,&cl,&res);
  if(ERR_NO_ERROR != res) return res;
  if(dv < 1900) return ERR_PARAM_RANG;
  tm->tm_year = dv - 1900;
  c += 4;
  dv = atoin(c,2,&cl,&res);
  if(ERR_NO_ERROR != res) return res;
  if((dv < 1) || (dv > 12)) return ERR_PARAM_RANG;
  tm->tm_mon = dv - 1;
  c+=2;
  dv = atoin(c,2,&cl,&res);
  if(ERR_NO_ERROR != res) return res;
  if((dv < 1) || (dv > 31)) return ERR_PARAM_RANG;
  tm->tm_mday = dv;
  c+=2;
  dv = atoin(c,2,&cl,&res);
  if(ERR_NO_ERROR != res) return res;
  if(dv > 23) return ERR_PARAM_RANG;
  tm->tm_hour = dv;
  c+=2;
  // minutes are optional
  dv = atoin(c,2,&cl,&res);
  if(ERR_NO_ERROR == res){
    if(dv > 59) return ERR_PARAM_RANG;
    c+=2;
    tm->tm_min = dv;
  } else {
    tm->tm_min = 0;
  }
  bool gmt_east = true;
  switch(*c++){
  default:
    return ERR_PARAM_OPT;
  case 'Z':
    gmtoff(tm,0);
    break;
  case '-':
    gmt_east = false;
    // fall through
  case '+':
    cl--;
    dv = atoin(c,2,&cl,&res);
    if(ERR_NO_ERROR != res) return res;
    if(dv > 23) return ERR_PARAM_RANG;
    c+=2;
    dv *= 60;
    int ddv = atoin(c,2,&cl,&res);
    if(ERR_NO_ERROR == res){ 
      if(ddv > 59) return ERR_PARAM_RANG;
      c+=2;
      dv += ddv;
    }
    dv *= 60;
    if(gmt_east) dv *= -1;
    gmtoff(tm,dv);
    break;
  }
  // we have no clue about dst
  tm->tm_isdst = -1;
  if(cl) return ERR_PARAM_LEN;
  return ERR_NO_ERROR;
}

BerTag *ASN1IO::writeUTC(const struct tm *tm, m_error_t *err) const {
  struct tm tmb;
  if(!tm){
    time_t tt = time(NULL);    
    tm = localtime_r(&tt,&tmb);
  }
  if(!tm){
    if(err) *err = ERR_PARAM_NULL;
    return NULL;
  }
  int dv = tm->tm_year + 1900;
  char dts[20];
  sprintf(dts,"%.4d",dv);
  char *dtp=dts+4;
  dv = tm->tm_mon + 1;
  sprintf(dtp,"%.2d",dv);
  dtp += 2;
  dv = tm->tm_mday;
  sprintf(dtp,"%.2d",dv);
  dtp += 2;
  dv = tm->tm_hour;
  sprintf(dtp,"%.2d",dv);
  dtp += 2;
  dv = tm->tm_min;
  sprintf(dtp,"%.2d",dv);
  dtp += 2;
  // this is glibc dependent
  dv = gmtoff(tm);
  if(!dv){
    *dtp++ = 'Z';
  } else {
    if(dv < 0){
      *dtp++ = '-';
      dv = -dv;
    } else *dtp++ = '+';
    dv /= 60;
    sprintf(dtp,"%.2d",dv / 60);
    dtp += 2;
    sprintf(dtp,"%.2d",dv % 60);
    dtp += 2;    
  }
  *dtp = 0;

  if(err) *err = ERR_MEM_AVAIL;
  BerTag *tag = new BerTag(asn1_utc,utcTag::SIZE);
  if(!tag) return tag;
  dv = dtp - dts;
  dtp = static_cast<char *>((void *)(tag->allocate(dv)));
  if(!dtp){
    delete tag;
    return NULL;
  }
  memcpy(dtp,dts,dv);

  if(err) *err = ERR_NO_ERROR;
  return tag;
}


/*
 * The testsuite
 *
 ********************************************
 *
 */

#ifdef TEST

#include <stdio.h>
#include <HexDump.h>
#include <wtBufferDump.h>

int main(int argc, char *argv[]){
  m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  try{
    FileDump stream(stdout);
    HexDump  xdump(&stream);

    printf("Test %d: CTOR\n",++tests);
    ASN1IO asn;
    puts("+++ ASN1IO::CTOR() finished OK!");
    
    printf("Test %d: writeInt()\n",++tests);
    BerTag *tag = asn.writeInt<int>(14);
    if(!tag){
      errors++;
      printf("*** Error:writeInt() returned NULL\n");
    } else {
      tag->dump(stdout,"??? ");
      puts("+++ TaggedDataFile::writeInt() finished OK!");
      delete tag;
    }    

    printf("Test %d: writeIntPacked()\n",++tests);
    tag = asn.writeIntPacked<unsigned long long>(14ULL);
    if(!tag){
      errors++;
      printf("*** Error:writeIntPacked() returned NULL\n");
    } else {
      tag->dump(stdout,"??? ");
      puts("+++ ASN1IO::writeIntPacked() finished OK!");
    }    

    printf("Test %d: readInt()\n",++tests);
    short sv = 0;
    res = asn.readInt<short>(*tag, sv);
    if((res != ERR_NO_ERROR) ||
       (sv != 14)){
      errors++;
      printf("*** Error: readInt<short> returned 0x%.4x [%d (14)]\n",
	     (int)res,(int)sv);
    } else {
      puts("+++ ASN1IO::readInt() finished OK!");
    }

    printf("Test %d: writeIA5String()\n",++tests);
    if(tag) delete tag;
    tag = asn.writeIA5String("My test string",&res);
    if((res != ERR_NO_ERROR) || !tag){
      errors++;
      if(tag){
	tag->dump(stdout,"??? ");
      }
      printf("*** Error: writeIA5String() returned 0x%.4x [%p]\n",
	     (int)res,tag);
    } else {
      tag->dump(stdout,"??? ");
      puts("+++ ASN1IO::writeIA5String() finished OK!");
    }
    
    printf("Test %d: readIA5String()\n",++tests);
    char sbuf[128];
    res = asn.readIA5String(*tag,sbuf,128);
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: readIA5String() returned 0x%.4x\n",(int)res);
    } else {
      printf("??? %s\n",sbuf);
      puts("+++ ASN1IO::readIA5String() finishedOK");
    }

    printf("Test %d: writePrintableString()\n",++tests);
    if(tag) delete tag;
    tag = asn.writePrintableString("Hallöle!",&res);
    if((res != ERR_NO_ERROR) || !tag){
      errors++;
      if(tag){
	tag->dump(stdout,"??? ");
      }
      printf("*** Error: writePrintableString() returned 0x%.4x [%p]\n",
	     (int)res,tag);
    } else {
      tag->dump(stdout,"??? ");
      puts("+++ ASN1IO::writePrintableString() finished OK!");
    }
    
    printf("Test %d: readPrintableString()\n",++tests);
    res = asn.readPrintableString(*tag,sbuf,128);
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: readPrintableString() returned 0x%.4x\n",(int)res);
    } else {
      printf("??? %s\n",sbuf);
      puts("+++ ASN1IO::readPrintableString() finishedOK");
    }

    printf("Test %d: writeUTC()\n",++tests);
    if(tag) delete tag;
    tag = asn.writeUTC((struct tm *)NULL,&res);
    if((res != ERR_NO_ERROR) || !tag){
      errors++;
      if(tag){
	tag->dump(stdout,"??? ");
      }
      printf("*** Error: writeUTC() returned 0x%.4x [%p]\n",
	     (int)res,tag);
    } else {
      tag->dump(stdout,"??? ");
      puts("+++ ASN1IO::writeUTC() finished OK!");
    }
    
    printf("Test %d: readUTC()\n",++tests);
    struct tm t;
    res = asn.readUTC(*tag,&t);
    if(res != ERR_NO_ERROR){
      errors++;
      printf("*** Error: readUTC() returned 0x%.4x\n",(int)res);
    } else {
      time_t ct = mktime(&t);
      printf("??? %s",ctime(&ct));
      ct = time(NULL);
      printf("### %s",ctime(&ct));
      puts("+++ ASN1IO::readUTC() finishedOK");
    }

    printf("Test %d: writePrintableString(NULL)\n",++tests);
    tag = asn.writePrintableString(NULL,&res);
    if(tag){
      tag->dump(stdout,"???");
    }
    if(!tag || (res != ERR_NO_ERROR)){
      errors++;
      printf("*** Error: writePrintable String() returned 0x%.4x\n",(int)res);
    } else {
      puts("+++ ASN1IO::writePrintableString() finishedOK");
    }

    printf("\n%d tests completed with %d errors.\n",tests,errors);
    printf("Used version: %s\n",asn.VersionTag());
    
    return 0;  
  }
  // m_error_t catched as int
  catch(int err){
    printf("*** Exception thrown: 0x%.4x\n",err);
  }
}

#endif // TEST
