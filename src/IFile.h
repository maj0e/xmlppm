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

#ifndef __IFILE_H__
#define __IFILE_H__

#include <stdio.h>

typedef struct ifile_t IFILE;

IFILE* ifopen(FILE*, const char*, const char*);

void ifclose(IFILE*);

void ifflush(IFILE*);

int ifprintf(IFILE*, const char* fmt, ...);

int ifputc(int c, IFILE*);

int ifputs(const char*, IFILE*);

int ifwrite(const char*, size_t, size_t, IFILE*);


#endif
