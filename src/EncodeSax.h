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

/* EncodeSax.h: SAX callbacks for encoding events into bytes */
#ifndef __ENCODE_SAX_H__
#define __ENCODE_SAX_H__

#include <expat.h>
#include "XmlModel.h"

void encodeXmlDecl(void *, const char *, const char *, int);
void encodeStartDoctypeDecl(void *,const char *,const char *,const char *,int );
void encodeEndDoctypeDecl(void *);
void encodeElementDecl(void *,const char *,struct XML_cp *);
void encodeAttlistDecl(void *,const char *,const char *,const char *,const char *,int);
void encodeEntityDecl(void *,const char *,int ,const char *,int ,const char *,const char *,const char *,const char *);
void encodeNotationDecl(void *,const char *,const char *,const char *,const char *);
int encodeExternalEntityRef(XML_Parser, const char*, const char *, const char *, const char *);
void encodeStartElement(void *,const char *,const char ** );
void encodeEndElement(void *,const char *);
void encodeStartCdata(void *);
void encodeEndCdata(void *);
void encodeComment(void *,const char *);
void encodeProcessingInstruction(void*, const char *,const char *);
void encodeCharacters(void *,const char *,int);
void encodeEntityRef(void *,const char *,int);
void encodeStartNamespaceDecl(void* userData,
			      const XML_Char* prefix,
			      const XML_Char* uri);
void encodeEndNamespaceDecl(void* userData, 
			    const XML_Char* prefix);

void encodeChar(PPM_ENCODER* m,unsigned char c);
extern void preloadChar(PPM_MODEL* m, xml_state* s);

#endif
