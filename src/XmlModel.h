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

#ifndef __XMLMODEL_H__
#define __XMLMODEL_H__

/* XmlModel.c: maintains compression state of XML compressor */

/* XML encoding of SAX events into bytecode, 
   using context to disambiguate. */

#include "IFile.h"
#include "StringArray.h"
#include "Model.h"

/* States: START, DTD, MISC, STRING, ELTLIST, ELEMENT, ATTLIST  */

/* STRING: strings of CDATA, eltnames, attnames, attvalues, PIs, comments)
   encoded as null terminated strings.  */
#define ENDSTRING 0

/* ELTLIST: a number of reasonable things might come next, including
   a PI, comment, entity ref, characters, or element.
   Elements are encoded using range [0-MAXELTS-1]; ENDELT signals the end
   of the sequence of immediate children of an element list (ie
   a matching </elt>).
   At most 250 distinct element names.
   ENTITY,PI,CHARS,COMMENT,CDATA are all raw strings.

   MISC: a sequence of PIs and COMMENTS only, terminated by ENDELT
*/
#define MAXELTS 250
#define CDATA 250
#define ENTITY 251
#define PI 252
#define COMMENT 253
#define CHARS 254
#define ENDELT 255

/* ELEMENT: Elements are stored by first storing elttag, a reference into 
   the element symbol table, followed by the string that goes there if this 
   is the first time we use it, followed by an ELTLIST.
*/

/* ATTLIST: Attributes stored in a list of (atttag,attval) pairs terminated 
   by ENDATT.
   atttag is a pointer into attribute name table, followed by string if
   that table entry needs to be filled in.
   attval is a string. 
   At most 254 distinct attribute names.
*/
#define MAXATTS 255
#define ENDATT 255

/* START : This state expects
   XMLDECL
   then MISC (a list containing only PIs and comments, same code as ELTLIST)
   then a DTD (= list of DTD stuff, maybe empty )
        followed by MISC
   then a ELEMENT
   then MISC
*/

/* DTD */
#define DTDPENTITY      245
#define DTDSTRING       246
#define DTDELEMENTDECL  247
#define DTDATTLISTDECL  248
#define DTDENTITYDECL   249
#define DTDNOTATIONDECL 250
#define DTDENTITY       (ENTITY) /* these MUST be == ordinary ones */
#define DTDCHARS        (CHARS)
#define DTDCOMMENT      (COMMENT)
#define DTDPI           (PI)
#define ENDDTD          255

/* NOTATIONS (list of notations in attribute) */

#define ENDNOT 255
#define MAXNOTS 255

/* ENTITIES (list of notations in attribute) */
#define ENDENT 255
#define MAXENTS 255

typedef struct elStackNode {
  int elem;
  struct elStackNode* next;
} elStackNode;


enum char_state {cs_none, cs_characters, cs_cdata};

typedef struct xml_state {
  xml_state();
  StringArray* elts;
  StringArray* atts;
  StringArray* nots;
  StringArray* ents;
  struct elStackNode* elTop;
  char* lastAttlistElt;
  int depth;
  enum char_state char_state;
  int hasDtd;
  int hasDecl;
  FILE* file;
  int standalone;
}xml_state;

typedef struct _xml_enc_state : public xml_state {
  PPM_ENCODER* charPPM;
  PPM_ENCODER* symPPM;
  PPM_ENCODER* eltPPM;
  PPM_ENCODER* attPPM;
  XML_Parser p;
} xml_enc_state;

typedef struct _xml_dec_state : public xml_state {
  PPM_DECODER* charPPM;
  PPM_DECODER* symPPM;
  PPM_DECODER* eltPPM;
  PPM_DECODER* attPPM;
  IFILE* ifile;
} xml_dec_state;


void pushElStack(xml_state* state, int);
int getTopEl(xml_state* state);
void popElStack(xml_state* state);

#endif
