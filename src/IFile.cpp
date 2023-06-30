/* XMLPPM: an XML compressor
Copyright (C) 2003 James Cheney

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Contacting the author:
James Cheney
Computer Science Department
Cornell University
Ithaca, NY 14850

jcheney@cs.cornell.edu
*/

#include <assert.h>
#include <errno.h>
#include <iconv.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "IFile.h"

#if defined (__CYGWIN__) 
extern "C" {
    int _vsnprintf(char*, size_t, const char*, va_list);  
}
#endif
#if defined(__CYGWIN__) || defined (WIN32)
#define VSNPRINTF(a,b,c,d) _vsnprintf((a),(b),(c),(d))
#define ICONV(a,b,c,d,e) iconv((a),((const char**)b),(c),(d),(e))
#else 
#define VSNPRINTF(a,b,c,d) vsnprintf((a),(b),(c),(d))
#define ICONV(a,b,c,d,e) iconv((a),(b),(c),(d),(e))
#endif

#define BUFFSIZE 8192
#define MIN(x,y) ((x) < (y) ? (x) : (y))

struct ifile_t {
  FILE* file;
  iconv_t iconv;
  unsigned int bufsiz;
  char buf[BUFFSIZE];
};

/* Opens an ifile.  tocode = input encoding, fromcode = output encoding. */
IFILE* ifopen(FILE* fp, const char* tocode, const char* fromcode) {
  IFILE* ifile = (IFILE*)malloc(sizeof(IFILE));
  if(ifile == NULL) return NULL;
  ifile->iconv = iconv_open(tocode, fromcode);
  ifile->bufsiz = 0;
  if(ifile->iconv == (iconv_t)(-1)) {
    free(ifile);
    return NULL;
  }
  ifile->file = fp;
  return ifile;
}

/* Flushes an ifile.  Empties the to-buffer by iconving it and 
   writing the output.  Then flush the file.  */
void ifflush(IFILE* ifile) {
  static char outbuf[BUFFSIZE];
  char* inptr = ifile->buf;
  char* outptr = outbuf;
  size_t insz = ifile->bufsiz;
  size_t outsz;
  int total = 0;
  while(insz > 0) {
    size_t result;
    outsz = BUFFSIZE;
    result = ICONV(ifile->iconv, &inptr, &insz, &outptr, &outsz);
    total += fwrite(outbuf, sizeof(char), BUFFSIZE-outsz, ifile->file);
    if(result == (size_t)-1) {
      /* depending on the error code, perform the appropriate 
	 recovery action */
      switch(errno) {
	
	/* E2BIG is no problem, just continue to the next iteration */
      case E2BIG: continue;
	
	/* EINVAL: the end of the inbuf is an incomplete multibyte 
	   character code.  Until more characters are available we 
	   can't proceed.  Copy the end of the buffer to here. */
      case EINVAL:
	/* TODO: This is tricky.  Test. */
	/* copy the end of the buffer to the beginning, 
	   memmove is overkill here because BUFFSIZE >= longest 
	   multibyte character sequence.  */ 
	memmove(ifile->buf, inptr, insz);
	/* set ifile->bufsiz to insz */
	ifile->bufsiz = insz;
	insz = 0;
	/* flush ifile->file, and return */
	fflush(ifile->file);
	return; 
	
	/* EILSEQ: an illegal multibyte character sequence
	   We can't recover from this, print error message and fail. */
      case EILSEQ: 
	perror("xmlunppm:");
	exit(-1);
      }
    }
  }
  ifile->bufsiz = 0;
  fflush(ifile->file);
}

/* Closes an ifile, flushing first. */
void ifclose(IFILE* ifile) {
  ifflush(ifile);
  assert(ifile->bufsiz == 0);
  iconv_close(ifile->iconv);
  free(ifile);
}

/* Writes formatted string to ifile.  May have problems with large format 
   strings... */
int ifprintf(IFILE* ifile, const char* fmt, ...) {
  va_list l;
  int len;
  static char inbuf[BUFFSIZE];
  va_start(l,fmt);
  len = VSNPRINTF(inbuf,BUFFSIZE,fmt,l);
  return ifwrite(inbuf,sizeof(char),len,ifile);
}

/* Writes character to ifile. */
int ifputc(int c, IFILE* ifile) {
  char ch = c;
  if(ifwrite(&ch,sizeof(char),1,ifile)) return c;
  else return EOF;
}

/* Writes zero-terminated string to ifile. */
int ifputs(const char* c, IFILE* ifile) {
  if(ifwrite(c,sizeof(char),strlen(c),ifile)) return 0;
  else return EOF;
}

/* ifwrite: takes a buf, size of elements (characters), number of 
characters, and an ifile.
Logically: writes buf to file, performing internationalization.
Physically: Buffers buf, flushing to make more room if we run out of space.
*/
int ifwrite(const char* buf, size_t size, size_t nmemb, IFILE* ifile) {
  unsigned int totalsize = size*nmemb;
  if (BUFFSIZE > totalsize + ifile->bufsiz) {
    memcpy(ifile->buf+ifile->bufsiz, buf, totalsize);
    ifile->bufsiz += totalsize;
    return nmemb;
  } else {
    do {
      int blocksize;
      /* TODO: Error handling */
      ifflush(ifile);
      blocksize = MIN(BUFFSIZE-ifile->bufsiz,totalsize);
      /* stop now if buf contains incomplete character sequence */
      assert(blocksize>0);
      memcpy(ifile->buf+ifile->bufsiz, buf, blocksize);
      ifile->bufsiz += blocksize;
      totalsize -= blocksize;
    } while(totalsize > BUFFSIZE);
    return nmemb;
  }
}
