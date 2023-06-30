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
#include <expat.h>
#include <fcntl.h>
#include <iconv.h>
#include <stdio.h>
#include <string.h>
#include "EncodeSax.h"
#include "IFile.h"
#include "PrintSax.h"
#include "XmlModel.h"
#include "DecodeSax.h"



#ifdef __CYGWIN__
extern "C" char* strdup(const char*);
#endif

#define BUFFSIZE 8192

int
decodeChar (PPM_DECODER * m)
{
#ifdef FIXED_MODEL
  int c = m->DecodeCharFixed ();
#else
  int c = m->DecodeChar ();
#endif
  return c;
}

char *
readString (PPM_DECODER * m)
{
  char* buff = (char*)malloc(BUFFSIZE*sizeof(char));
  int size = BUFFSIZE;
  int i = 0;
  int c;
  assert(buff != NULL);
  while ((c = decodeChar (m)) != EOF && c != '\0') {
    buff[i++] = c;
    if(i == size) {
      size += BUFFSIZE;
      buff = (char*)realloc(buff, size);
      assert(buff != NULL);
    }
  }
  if (i > 0)
    {
      buff[i] = '\0';
      return buff;
    }
  else
    return NULL;
}

/* Memory leak if array fills up */
char *
readSymbol (PPM_DECODER * sym_m, StringArray * array, unsigned char loc)
{
  if (loc == array->len)
    {
      /* if array full then forget symbol */
      if (array->isFull ()) 
	return readString (sym_m);
      else			/* else add it */
	array->add (readString (sym_m));
    }
  return array->arr[loc];
}

void
decodeCdata (xml_dec_state * state)
{
  char *str = readString (state->charPPM);
  printStartCdata (state->ifile);
  printCharacters (state->ifile, str, strlen (str));
  printEndCdata (state->ifile);
  free(str);
}

void
decodeEntity (xml_dec_state * state)
{
  char *str = readString (state->charPPM);
  ifprintf (state->ifile, "&%s;", str);
  free(str);
}

void
decodePEntity (xml_dec_state * state)
{
  char *str = readString (state->charPPM);
  ifprintf (state->ifile, "%%%s;", str);
  free(str);
}

void
decodeProcessingInstruction (xml_dec_state * state)
{
  char *target = readString (state->charPPM);
  char *data = readString (state->charPPM);
  printProcessingInstruction (state->ifile, target, data);
  free(target);
  free(data);
}

void
decodeComment (xml_dec_state * state)
{
  char *data = readString (state->charPPM);
  printComment (state->ifile, data);
  free(data);
}



void
decodeChars (xml_dec_state * state)
{
  char *str;
  preloadChar (state->charPPM, state);
  str = readString (state->charPPM);
  ifprint_xml_string (state->ifile, str, strlen (str));
  free(str);
}

char *attributeBuff[1000];	/* buffer overflow hazard */
int attributeLen = 0;

void
decodeAttribute (xml_dec_state * state, unsigned char tag)
{
  attributeBuff[attributeLen++] =
    readSymbol (state->symPPM, state->atts, tag);
  attributeBuff[attributeLen++] = readString (state->attPPM);
}

void
decodeAttributes (xml_dec_state * state)
{
  unsigned char tag = 0;
  int done = 0;
  int c;
  attributeLen = 0;
  while (done == 0)
    {
      preloadChar (state->attPPM, state);

      if ((c = decodeChar (state->attPPM)) == EOF)
	break;
	
      tag = c;

      switch (tag)
	{
	case ENDATT:
	  done = 1;
	  break;
	default:
	  decodeAttribute (state, tag);
	  break;
	}
    }
  attributeBuff[attributeLen] = NULL;
}

void freeAttributeList() {
  int i;
  for(i = 0; i < attributeLen; i+=2) {
    free(attributeBuff[i+1]);
  }
}



/* Forward reference. */
/* Namespaces are treated as ordinary attributes here. */
void
decodeElement (xml_dec_state * state, unsigned char tag)
{
  const XML_Char *elttag;
  
  elttag  = readSymbol (state->symPPM, state->elts, tag);
  pushElStack (state, tag);
  decodeAttributes (state);
  printStartElement (state->ifile, elttag, 
		     (const XML_Char**)attributeBuff, NULL);
  freeAttributeList();
  decodeElementList (state);
  printEndElement (state->ifile, elttag);
  popElStack (state);
  if(state->elTop != NULL)
    preloadChar (state->eltPPM, state);
}

/* Possible next things: 
   Entity,CDATA,PI,Comment,Characters, Element */
void
decodeElementList (xml_dec_state * state)
{
  unsigned char tag;
  int done = 0;
  int c;
  while (done == 0)
    {
      if ((c = decodeChar (state->eltPPM)) == EOF)
	break;
      tag = c;
      switch (tag)
	{
	case CDATA:
	  decodeCdata (state);
	  break;
	case ENTITY:
	  decodeEntity (state);
	  break;
	case PI:
	  decodeProcessingInstruction (state);
	  break;
	case COMMENT:
	  decodeComment (state);
	  break;
	case CHARS:
	  decodeChars (state);
	  break;
	case ENDELT:
	  done = 1;
	  break;
	default:
	  decodeElement (state, tag);
	  break;
	}
    }

}

void
decodeMisc (xml_dec_state * state)
{
  unsigned char tag;
  int done = 0;
  int c;
  while (done == 0)
    {
      if ((c = decodeChar (state->eltPPM)) == EOF)
	break;
      tag = c;
      switch (tag)
	{
	case CHARS:
	  decodeChars (state);
	  break;
	case PI:
	  decodeProcessingInstruction (state);
	  ifputc('\n',state->ifile);
	  break;
	case COMMENT:
	  decodeComment (state);
	  ifputc('\n',state->ifile);
	  break;
	case ENDELT:
	  done = 1;
	  break;
	default:
	  fprintf (stderr, "Illegal Misc token:%c", tag);
	}
    }
}

/* TODO: Fix memory leak here (not urgent) */
XML_Content *
createEmptyModel ()
{
  XML_Content *model = (XML_Content *) malloc (sizeof (XML_Content));
  model->type = XML_CTYPE_EMPTY;
  model->quant = XML_CQUANT_NONE;
  model->name = NULL;
  model->numchildren = 0;
  model->children = NULL;
  return model;
}

XML_Content *
createAnyModel ()
{
  XML_Content *model = createEmptyModel ();
  model->type = XML_CTYPE_ANY;
  return model;
}



XML_Content *
decodeNameModel (xml_dec_state * state)
{
  unsigned char elttag = decodeChar (state->eltPPM);
  const char *eltname = readSymbol (state->symPPM, state->elts, elttag);
  unsigned char quant = decodeChar (state->eltPPM);
  XML_Content *model = createEmptyModel ();
  model->type = XML_CTYPE_NAME;
  model->quant = (XML_Content_Quant) quant;
  model->name = strdup (eltname);
  return model;
}

/*restricts number of elts of list to [0..255] */

XML_Content *
decodeMixedModel (xml_dec_state * state)
{
  unsigned char contentLen;
  XML_Content *model = createEmptyModel ();
  contentLen = decodeChar (state->eltPPM);
  model->type = XML_CTYPE_MIXED;
  if (contentLen == 0)
    {
      model->quant = XML_CQUANT_NONE;
    }
  else
    {
      int i;
      model->quant = XML_CQUANT_REP;
      model->numchildren = contentLen;
      model->children =
	(XML_Content *) malloc (contentLen * sizeof (XML_Content));
      for (i = 0; i < contentLen; i++)
	{
	  model->children[i] = *decodeNameModel (state);
	}
    }
  return model;
}

XML_Content *
decodeSeqModel (xml_dec_state * state)
{
  int contentLen;
  int quant;
  int i;
  XML_Content *model = createEmptyModel ();
  contentLen = decodeChar (state->eltPPM);
  quant = decodeChar (state->eltPPM);
  model->type = XML_CTYPE_SEQ;
  model->quant = (XML_Content_Quant) quant;
  model->numchildren = ++contentLen;	/* at least 1 child */
  model->children =
    (XML_Content *) malloc (contentLen * sizeof (XML_Content));
  for (i = 0; i < contentLen; i++)
    {
      model->children[i] = *decodeContentModel (state);
    }
  return model;
}

XML_Content *
decodeChoiceModel (xml_dec_state * state)
{
  int contentLen;
  int quant;
  int i;
  XML_Content *model = createEmptyModel ();
  contentLen = decodeChar (state->eltPPM);
  quant = decodeChar (state->eltPPM);
  model->type = XML_CTYPE_CHOICE;
  model->quant = (XML_Content_Quant) quant;
  model->numchildren = ++contentLen;	/* at least 1 child */
  model->children =
    (XML_Content *) malloc (contentLen * sizeof (XML_Content));
  for (i = 0; i < contentLen; i++)
    {
      model->children[i] = *decodeContentModel (state);
    }
  return model;
}


XML_Content *
decodeContentModel (xml_dec_state * state)
{
  unsigned char type = decodeChar (state->eltPPM);
  switch (type)
    {
    case XML_CTYPE_EMPTY:
      return createEmptyModel ();
      break;
    case XML_CTYPE_ANY:
      return createAnyModel ();
      break;
    case XML_CTYPE_MIXED:
      return decodeMixedModel (state);
      break;
    case XML_CTYPE_NAME:
      return decodeNameModel (state);
      break;
    case XML_CTYPE_CHOICE:
      return decodeChoiceModel (state);
      break;
    case XML_CTYPE_SEQ:
      return decodeSeqModel (state);
      break;
    }
  return NULL;
}

void
decodeElementDecl (xml_dec_state * state)
{
  unsigned char elttag = decodeChar (state->eltPPM);
  const char *eltname = readSymbol (state->symPPM,
				    state->elts, elttag);
  XML_Content *model = decodeContentModel (state);
  printElementDecl (state->ifile, eltname, model);
}

void
decodeAttributeDecl (xml_dec_state * state, unsigned char atttag)
{
  char *attname = readSymbol (state->symPPM, state->elts, atttag);
  char *att_type = readString (state->eltPPM);
  char *dflt = readString (state->eltPPM);
  unsigned char isrequired = decodeChar (state->eltPPM);
  printAttributeDecl (state->ifile, attname, att_type, dflt, isrequired);
  free(att_type);
  free(dflt);
}

void
decodeAttlistDecl (xml_dec_state * state)
{
  unsigned char elttag = decodeChar (state->eltPPM);
  char *elname = readSymbol (state->symPPM, state->elts, elttag);
  unsigned char tag;
  printStartAttlistDecl (state->ifile, elname);
  while ((tag = decodeChar (state->eltPPM)) != ENDATT)
    {
      decodeAttributeDecl (state, tag);
    }
  printEndAttlistDecl (state->ifile);
}

void
decodeEntityDecl (xml_dec_state * state)
{
  XML_Char *entityName = NULL;
  int is_parameter_entity;
  XML_Char *value = NULL;
  int value_length = 0;
  XML_Char *base = NULL;	/* eventually a parser parameter */
  XML_Char *systemId = NULL;
  XML_Char *publicId = NULL;
  XML_Char *notationName = NULL;
  entityName = readString (state->eltPPM);
  is_parameter_entity = decodeChar (state->eltPPM);
  value = readString (state->eltPPM);
  if (value != NULL)
    value_length = strlen (value);
  else
    {
      systemId = readString (state->eltPPM);
      publicId = readString (state->eltPPM);
      notationName = readString (state->eltPPM);
    }
  printEntityDecl (state->ifile, entityName, is_parameter_entity, value,
		   value_length, base, systemId, publicId, notationName);
  free(entityName);
  free(value);
  free(systemId);
  free(publicId);
  free(notationName);
}


void
decodeNotationDecl (xml_dec_state * state)
{
  XML_Char *notationName;
  XML_Char *systemId;
  XML_Char *publicId;
  notationName = readString (state->eltPPM);
  systemId = readString (state->eltPPM);
  publicId = readString (state->eltPPM);
  /* no point in doing cases to avoid the single impossible case */
  /* base is eventually going to be an externally determined 
     parser field */
  printNotationDecl (state->ifile, notationName, NULL, systemId, publicId);
  free(notationName);
  free(systemId);
  free(publicId);
}

void
decodeDTD (xml_dec_state * state)
{
  unsigned char tag;
  int done = 0;
  int c;
  char *doctypeName;

  /* only do rest of doctypeName exists (otherwise, no DTD was explicitly
     declared) */
  doctypeName = readString (state->eltPPM);

  if (doctypeName != NULL)
    {
      char *sysid = readString (state->eltPPM);
      char *pubid = readString (state->eltPPM);
      int has_internal_subset = decodeChar (state->eltPPM);

      printStartDoctypeDecl (state->ifile, doctypeName,
			     sysid, pubid, has_internal_subset);
      free(doctypeName);
      free(sysid);
      free(pubid);

      if (has_internal_subset)
	{
	  while (done == 0)
	    {
	      if ((c = decodeChar (state->eltPPM)) == EOF)
		break;
	      tag = c;
	      switch (tag)
		{


		case DTDELEMENTDECL:
		  decodeElementDecl (state);
		  break;

		case DTDNOTATIONDECL:
		  decodeNotationDecl (state);
		  break;
		case DTDENTITYDECL:
		  decodeEntityDecl (state);
		  break;
		case DTDATTLISTDECL:
		  decodeAttlistDecl (state);
		  break;

		case DTDENTITY:
		  decodeEntity (state);
		  break;
		case DTDPENTITY:
		  decodePEntity (state);
		  break;

		case DTDCHARS:
		  decodeChars (state);
		  break;
		case DTDCOMMENT:
		  decodeComment (state);
		  ifprintf (state->ifile, "\n");
		  break;
		case DTDPI:
		  decodeProcessingInstruction (state);
		  ifprintf (state->ifile, "\n");
		  break;
		case ENDDTD:
		  done = 1;
		  break;
		default:
		  fprintf (stderr, "Illegal DTD token:%c", tag);
		}
	    }
	}
      printEndDoctypeDecl (state->ifile);
      decodeMisc (state);
    }
}

void
decodeXMLDecl (xml_dec_state * state)
{
  char *version = readString (state->eltPPM);
  if (version != NULL)
    {
      char *encoding = readString (state->eltPPM);
      char standalone = decodeChar (state->eltPPM);
      if(encoding == NULL) { /* use default UTF-8 */
	state->ifile = ifopen(state->file, "UTF-8", "UTF-8");
      }
      else { /* use provided encoding */
	state->ifile = ifopen(state->file, encoding, "UTF-8");
      }
      assert(state->ifile);
      printXmlDecl (state->ifile, version, encoding, standalone);
      free(encoding);
      free(version);
    } else { /* use default UTF-8 */
      state->ifile = ifopen(state->file, "UTF-8", "UTF-8");
    }
}



void
decodeXML (xml_dec_state * state)
{
  unsigned char c;
  decodeXMLDecl (state);
  decodeMisc (state);
  decodeDTD (state);
  c = decodeChar (state->eltPPM);
  decodeElement (state, c);
  ifputc('\n',state->ifile);
  decodeMisc (state);
}


