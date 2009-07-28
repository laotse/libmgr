/*
 * The testsuite
 *
 ********************************************
 *
 */

#include "bigNum.h"

#include "bigNum.tag"

const char * mgr::_BigNumber::VersionTag(void) const{
  return _VERSION_;
}

template<>
unsigned int mgr::BigNumCtx::Singleton::Counter = 0;
template<>
mgr::BigNumCtx::Object_t mgr::BigNumCtx::Singleton::Object = NULL;


#ifdef TEST

# include <cstdio>

using namespace mgr;

int main(int argc, char *argv[]){
  // m_error_t res;
  size_t tests, errors;

  tests = errors = 0;

  printf("Test %d: Simple Value CTOR\n",++tests);
  BigNumber b1(4);
  if(b1.value() != 4){
    errors++;
    printf("Assigning simple value in CTOR failed - expected 4, result: %s\n",
	   b1.toDecimal().c_str());
  } else {
    puts("+++ Simple Value CTOR OK");
  }

  printf("Test %d: Decimal stringify\n",++tests);
  if(b1.toDecimal().c_str() != std::string("4")){
    errors++;
    printf("Result is \"%s\" - expected \"4\"\n",b1.toDecimal().c_str());
  } else {
    puts("+++ Decimal stringify OK");
  }


  printf("Test %d: Multiply assign\n",++tests);
  BigNumber b2(3);
  b2 *= b1;
  if(b2.value() != 12){
    errors++;
    printf("Multiply assign failed - expected 12, result: %s\n",
	   b2.toDecimal().c_str());
  } else {
    puts("+++ Multiply assign OK");
  }

  printf("Test %d: Assignment\n",++tests);
  BigNumber b3;
  b3 = b2;
  if(b3.value() != 12){
    errors++;
    printf("Assign failed - expected 12, result: %s\n",
	   b2.toDecimal().c_str());
  } else {
    puts("+++ Assign OK");
  }

  printf("Test %d: Compare ==\n",++tests);
  if(!(b3 == b2)){
    errors++;
    puts("*** Compare == failed!");
  } else {
    puts("+++ Compare == OK");
  }

  printf("Test %d: Compare !=\n",++tests);
  if(b3 != b2){
    errors++;
    puts("*** Compare != failed!");
  } else {
    puts("+++ Compare != OK");
  }

  printf("Test %d: Compare >\n",++tests);
  if(!(b3 > b1)){
    errors++;
    puts("*** Compare > failed!");
  } else {
    puts("+++ Compare > OK");
  }

  printf("Test %d: Compare <\n",++tests);
  if(!(b1 < b3)){
    errors++;
    puts("*** Compare < failed!");
  } else {
    puts("+++ Compare < OK");
  }

  printf("Test %d: Multiply\n",++tests);
  b3 = b2 * b1;
  if(b3.value() != 48){
    errors++;
    printf("Multiply failed - expected 48, result: %s\n",
	   b3.toDecimal().c_str());
  } else {
    puts("+++ Multiply OK");
  }

  printf("Test %d: Divide assign\n",++tests);
  b3 /= b2;
  if(b3 != b1){
    errors++;
    printf("Divide assign failed - expected %s, result: %s\n",
	   b1.toDecimal().c_str(), b3.toDecimal().c_str());
  } else {
    puts("+++ Divide assign OK");
  }

  printf("Test %d: Add assign\n",++tests);
  b3 += b1;
  BigNumber b4(2);
  b2 = b1 * b4;
  if(b3 != b2){
    errors++;
    printf("Add assign failed - expected %s, result: %s\n",
	   b2.toDecimal().c_str(), b3.toDecimal().c_str());
  } else {
    puts("+++ Add assign OK");
  }

  printf("Test %d: Divide\n",++tests);
  b2 = b3 / b4;
  if(b2 != b1){
    errors++;
    printf("Divide failed - expected %s, result: %s\n",
	   b1.toDecimal().c_str(), b2.toDecimal().c_str());
  } else {
    puts("+++ Divide OK");
  }

  printf("Test %d: Implicit assignment\n",++tests);
  b2 = 15;
  if(b2.value() != 15){
    errors++;
    printf("Implicit assignment failed -- expected 15, result %s\n",
	   b2.toDecimal().c_str());
  } else {
    puts("+++ Implicit assign OK");
  }
    
  printf("Test %d: Implicit conversion\n",++tests);
  // is ambiguous, compiler does not know which side to convert
  if((long)b2 != 15){
    errors++;
    printf("Implicit conversion failed -- expected 15, result %s\n",
	   b2.toDecimal().c_str());
  } else {
    puts("+++ Implicit conversion OK");
  }

  printf("Test %d: Swap value\n",++tests);
  b1 = 217;
  b1.swap(b2);
  if(((long)b1 != 15) || ((long)b2 != 217)){
    errors++;
    printf("Swap failed -- expected 15 / 217, result %s / %s\n",
	   b1.toDecimal().c_str(), b2.toDecimal().c_str());
  } else {
    puts("+++ Swap OK");
  }

  printf("Test %d: Multiply assign with long\n",++tests);
  b1 *= 3;
  if((long)b1 != 45){
    errors++;
    printf("Multiply assign long -- expected 45, result %s\n",
	   b1.toDecimal().c_str());
  } else {
    puts("+++ Multiply assign with long OK");
  }

  printf("Test %d: Modulo assign\n",++tests);
  b1 %= BigNumber(7);
  if((long)b1 != 3){
    errors++;
    printf("Modulo assign -- expected 3, result %s\n",
	   b1.toDecimal().c_str());
  } else {
    puts("+++ Modulo assign OK");
  }

  printf("Test %d: Modulo assign with long\n",++tests);
  b1 = 45;
  b1 %= 7;
  if((long)b1 != 3){
    errors++;
    printf("Modulo assign with long -- expected 3, result %s\n",
	   b1.toDecimal().c_str());
  } else {
    puts("+++ Modulo assign with long OK");
  }

  printf("Test %d: Modulo with long\n",++tests);
  b1 = 45;
  // the simple form b1 % 7 is ambiguous
  long a = b1.operator%(7);
  if(a != 3){
    errors++;
    printf("Modulo with long -- expected 3, result %ld\n",
	   a);
  } else {
    puts("+++ Modulo with long OK");
  }

  printf("\n%d tests completed with %d errors.\n",tests,errors);
  printf("Used version: %s\n",b1.VersionTag());

  return 0;  
}


#endif // TEST
