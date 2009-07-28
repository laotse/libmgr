/*
 *
 * Error tracking and reporting system
 *
 * (c) 2005 µAC - Microsystem Accessory Consult
 * Dr. Lars Hanke
 *
 * $Id: error.h,v 1.4 2008-05-15 20:58:25 mgr Exp $
 *
 * This defines the classes:
 *
 * This defines the values:
 *  error codes
 *
 */


#define ERR_NO_ERROR   0x000
#define ERR_CANCEL     0x001 // No error, just cancel action
#define ERR_FILE       0x100 // File related errors  
#define ERR_FILE_OPEN  0x101
#define ERR_FILE_CLOSE 0x102
#define ERR_FILE_READ  0x103
#define ERR_FILE_WRITE 0x104
#define ERR_FILE_STAT  0x105
#define ERR_FILE_MKDIR 0x106
#define ERR_FILE_ISDIR 0x107
#define ERR_FILE_END   0x108
#define ERR_FILE_CONT  0x109 // File not completely read
#define ERR_MEM        0x200 // Memory related errors
#define ERR_MEM_AVAIL  0x201
#define ERR_PARAM      0x300 // Parameter passing errors
#define ERR_PARAM_NULL 0x301 // NULL pointer passed
#define ERR_PARAM_RANG 0x302 // parameter out of range
#define ERR_PARAM_OPT  0x303 // option value is not of required type
#define ERR_PARAM_LEN  0x304 // Length parameter of unsuitable value
#define ERR_PARAM_SEL  0x305 // selection parameter (enum type) not supported
#define ERR_PARAM_KEY  0x306 // key or keyword unknown
#define ERR_PARAM_RUN  0x307 // runaway argument, missing delimiter
#define ERR_PARAM_XNUL 0x308 // NULL pointer expected
#define ERR_PARAM_UDEF 0x309 // required parameter is missing
#define ERR_PARAM_TYP  0x30a // parameter object is not of required type
#define ERR_PARAM_UNIQ 0x30b // parameter is required to be unique, but is not
#define ERR_INT        0x400 // Internal, i.e. programming error
#define ERR_INT_BOUND  0x401 // Array boundary out of range
#define ERR_INT_RANG   0x402 // Internal enum out of range
#define ERR_INT_COMP   0x403 // Internal compatibility problem 
#define ERR_INT_STATE  0x404 // Impossible condition encountered
#define ERR_INT_DATA   0x405 // Internal data structure corrupt
#define ERR_PARS       0x500 // Parser error
#define ERR_PARS_STX   0x501 // Syntax error

typedef unsigned long error_t;
