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

/* EncodeSax.h: SAX callbacks for encoding events into bytes and then 
 * compressing the bytes using the PPM compressors
 */

#include <stdio.h>
#include <expat.h>
#include <string.h>
#include "XmlModel.h"
#include "EncodeSax.h"

#define BUFFSIZE	8192
#define MIN(x,y) ((x)<(y)?(x):(y))

char Extbuff[BUFFSIZE];

extern "C" char *strdup (const char *s);

void
preloadChar (PPM_MODEL * m, xml_state * state)
{
#ifdef FIXED_MODEL
  m->PreloadCharFixed (state->elTop->elem); 
#else
  m->PreloadChar (state->elTop->elem); 
#endif
}

void
encodeChar (PPM_ENCODER * m, unsigned char c)
{
#ifdef FIXED_MODEL
  m->EncodeCharFixed (c);
#else
  m->EncodeChar (c);
#endif
}

void
encodeString (PPM_ENCODER * m, const XML_Char * s)
{
  int i;
  for (i = 0; s[i] != '\0'; i++)
    encodeChar (m, s[i]);
}

void
encodeStringLen (PPM_ENCODER * m, const XML_Char * s, int len)
{
  int i;
  for (i = 0; i < len; i++)
    encodeChar (m, s[i]);
}

void
writeString (PPM_ENCODER * m, const XML_Char * s)
{
  if (s != NULL) 
    encodeString(m, s);
  encodeChar (m, '\0');
}


int
writeSymbol (PPM_ENCODER * tag_m, PPM_ENCODER * sym_m,
	     StringArray * array, const XML_Char * sym)
{
  int loc;
  loc = array->lookup (sym);
  if (loc == -1)
    {
      loc = array->len;
      /* if array not full, then add symbol */
      if (!array->isFull ())
	array->add (sym);
      encodeChar (tag_m, (unsigned char) loc);
      writeString (sym_m, sym);
    }
  else
    {
      encodeChar (tag_m, (unsigned char) loc);
    }
  return loc;
}



void
encodeXmlDecl (void *userData,
	       const XML_Char * version,
	       const XML_Char * encoding, int standalone)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  writeString (state->eltPPM,  version);
  if (version != NULL)
    {
      writeString (state->eltPPM, encoding);
      encodeChar (state->eltPPM, standalone);
    }
  state->hasDecl = 1;
}



void
enter_attlist (xml_enc_state * state, const XML_Char * elname)
{
  state->lastAttlistElt = strdup (elname);
  encodeChar (state->eltPPM, DTDATTLISTDECL);
  writeSymbol (state->eltPPM, state->symPPM, state->elts, elname);
}

void
exit_attlist (xml_enc_state * state)
{
  if (state->lastAttlistElt != NULL)
    {
      free (state->lastAttlistElt);
      state->lastAttlistElt = NULL;
      encodeChar (state->eltPPM, ENDATT);
    }
}

void
continue_attlist (xml_enc_state * state, const XML_Char * elname)
{
  /* If not in attlist, start attlist. */
  if (state->lastAttlistElt == NULL)
    enter_attlist (state, elname);
  /* If in attlist with different name, end it and start new. */
  else if (strcmp (state->lastAttlistElt, elname) != 0)
    {
      exit_attlist (state);
      enter_attlist (state, elname);
    }
  /* If in attlist that has same elname as this, do nothing. */
}



void
encodeStartDoctypeDecl (void *userData,
			const XML_Char * doctypeName,
			const XML_Char * sysid,
			const XML_Char * pubid, int has_internal_subset)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;

  if (state->hasDecl == 0)
    encodeXmlDecl (state, NULL, NULL, -1);

  /*end Misc list prior to DTD start */
  encodeChar (state->eltPPM, ENDELT);

  writeString (state->eltPPM, doctypeName);
  if (doctypeName != NULL)
    {
      writeString (state->eltPPM, sysid);
      writeString (state->eltPPM, pubid);
      encodeChar (state->eltPPM, has_internal_subset);
    }
  state->hasDtd = has_internal_subset;
}

void
encodeEndDoctypeDecl (void *userData)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  if (state->hasDtd)		/* == has_internal_subset */
    {
      /* end any attlist */
      exit_attlist (state);
      /* end DTD list */
      encodeChar (state->eltPPM, ENDDTD);
    }
  state->hasDtd = 1;		/* == true, apologies for hack */

}

void encodeContentModel (xml_enc_state * state, XML_Content * model);

void
encodeNameModel (xml_enc_state * state, XML_Content * model)
{
  writeSymbol (state->eltPPM, state->symPPM, state->elts, model->name);
  encodeChar (state->eltPPM, model->quant);
}

void
encodeMixedModel (xml_enc_state * state, XML_Content * model)
{
  unsigned int i;
  encodeChar (state->eltPPM, model->numchildren);
  for (i = 0; i < model->numchildren; i++)
    encodeNameModel (state, model->children + i);
}

void
encodeChoiceModel (xml_enc_state * state, XML_Content * model)
{
  unsigned int i;
  encodeChar (state->eltPPM, model->numchildren - 1);
  encodeChar (state->eltPPM, model->quant);
  for (i = 0; i < model->numchildren; i++)
    encodeContentModel (state, model->children + i);
}

void
encodeSeqModel (xml_enc_state * state, XML_Content * model)
{
  unsigned int i;
  encodeChar (state->eltPPM, model->numchildren - 1);
  encodeChar (state->eltPPM, model->quant);
  for (i = 0; i < model->numchildren; i++)
    encodeContentModel (state, model->children + i);
}

void
encodeContentModel (xml_enc_state * state, XML_Content * model)
{
  switch (model->type)
    {
    case XML_CTYPE_EMPTY:
      encodeChar (state->eltPPM, XML_CTYPE_EMPTY);
      break;
    case XML_CTYPE_ANY:
      encodeChar (state->eltPPM, XML_CTYPE_ANY);
      break;
    case XML_CTYPE_MIXED:
      encodeChar (state->eltPPM, XML_CTYPE_MIXED);
      encodeMixedModel (state, model);
      break;
    case XML_CTYPE_NAME:
      encodeChar (state->eltPPM, XML_CTYPE_NAME);
      encodeNameModel (state, model);
      break;
    case XML_CTYPE_CHOICE:
      encodeChar (state->eltPPM, XML_CTYPE_CHOICE);
      encodeChoiceModel (state, model);
      break;
    case XML_CTYPE_SEQ:
      encodeChar (state->eltPPM, XML_CTYPE_SEQ);
      encodeSeqModel (state, model);
      break;
    }
}

void
encodeElementDecl (void *userData, const XML_Char * name, XML_Content * model)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  exit_attlist (state);
  encodeChar (state->eltPPM, DTDELEMENTDECL);
  writeSymbol (state->eltPPM, state->symPPM, state->elts, name);
  encodeContentModel (state, model);
}

void
encodeAttlistDecl (void *userData,
		   const XML_Char * elname,
		   const XML_Char * attname,
		   const XML_Char * att_type,
		   const XML_Char * dflt, int isrequired)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  continue_attlist (state, elname);
  writeSymbol (state->eltPPM, state->symPPM, state->elts, attname);
  writeString (state->eltPPM, att_type);
  writeString (state->eltPPM, dflt);
  encodeChar (state->eltPPM, isrequired);
}

void
encodeEntityDecl (void *userData,
		  const XML_Char * entityName,
		  int is_parameter_entity,
		  const XML_Char * value,
		  int value_length,
		  const XML_Char * base,
		  const XML_Char * systemId,
		  const XML_Char * publicId, const XML_Char * notationName)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  exit_attlist (state);
  encodeChar (state->eltPPM, DTDENTITYDECL);
  writeString (state->eltPPM, entityName);
  encodeChar (state->eltPPM, is_parameter_entity);
  encodeStringLen (state->eltPPM, value, value_length);
  encodeChar (state->eltPPM, ENDSTRING);
  if (value == NULL)
    {
      writeString (state->eltPPM, systemId);
      writeString (state->eltPPM, publicId);
      writeString (state->eltPPM, notationName);
    }
}

void
encodeNotationDecl (void *userData,
		    const XML_Char * notationName,
		    const XML_Char * base,
		    const XML_Char * systemId, const XML_Char * publicId)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  exit_attlist (state);
  encodeChar (state->eltPPM, DTDNOTATIONDECL);
  writeString (state->eltPPM, notationName);
  writeString (state->eltPPM, systemId);
  writeString (state->eltPPM, publicId);
}

/* experimental */
int
encodeExternalEntityRef (XML_Parser parser,
			 const XML_Char * context,
			 const XML_Char * base,
			 const XML_Char * systemId, const XML_Char * publicId)
{
  XML_Parser extp = XML_ExternalEntityParserCreate (parser, context, NULL);
  XML_SetXmlDeclHandler (extp, NULL);
  XML_SetDoctypeDeclHandler (extp, NULL, NULL);
  XML_SetElementDeclHandler (extp, NULL);
  XML_SetAttlistDeclHandler (extp, NULL);
  XML_SetEntityDeclHandler (extp, NULL);
  XML_SetNotationDeclHandler (extp, NULL);
  XML_SetNamespaceDeclHandler (extp, NULL, NULL);
  XML_SetElementHandler (extp, NULL, NULL);
  XML_SetCharacterDataHandler (extp, NULL);
  XML_SetProcessingInstructionHandler (extp, NULL);
  XML_SetCommentHandler (extp, NULL);
  XML_SetCdataSectionHandler (extp, NULL, NULL);
  XML_SetDefaultHandler (extp, NULL);

  FILE *file = fopen (systemId, "r");
  if (file == NULL)
    {
      fprintf (stderr, "External entity file not found : %s %s",
	       base, systemId);
      exit (1);
    }

  for (;;)
    {
      int done;
      int len;

      len = fread (Extbuff, 1, BUFFSIZE, file);
      if (ferror (file))
	{
	  fprintf (stderr, "Read error\n");
	  exit (-1);
	}
      done = feof (file);

      if (!XML_Parse (extp, Extbuff, len, done))
	{
	  fprintf (stderr, "Parse error at line %d:\n%s\n",
		   XML_GetCurrentLineNumber (extp),
		   XML_ErrorString (XML_GetErrorCode (extp)));
	  exit (-1);
	}

      if (done)
	break;
    }
  XML_ParserFree (extp);
  return 1;
}

void
enter_characters (xml_enc_state * state)
{
  if (state->char_state == cs_none)
    {
      encodeChar (state->eltPPM, CHARS);
      preloadChar (state->charPPM, state);
      state->char_state = cs_characters;
    }
}

void
exit_characters (xml_enc_state * state)
{
  if (state->char_state == cs_characters)
    {
      encodeChar (state->charPPM, ENDSTRING);
      state->char_state = cs_none;
    }
}


void
push_element (xml_enc_state * state)
{
  /* must terminate the Misc that can occur btwn DTD and root elt */
  if (state->depth == 0)
    {
      if (state->hasDecl == 0)
	encodeXmlDecl (state, NULL, NULL, -1);
      if (state->hasDtd == 0)
	{
	  encodeStartDoctypeDecl (state, NULL, NULL, NULL, 0);
	  encodeEndDoctypeDecl (state);
	}
      else /* dtd happened, need to close off any intervening Misc */
	{
	  encodeChar (state->eltPPM,  ENDELT);
	}
    }
  state->depth++;
}

void
pop_element (xml_enc_state * state)
{
  state->depth--;
}




void
encodeStartElement (void *userData, const char *fullname, const char **atts)
{
  xml_enc_state *state;
  int i;
  int elttag;
  state = (xml_enc_state *) userData;
  push_element (state);
  exit_characters (state);
  elttag = writeSymbol (state->eltPPM, state->symPPM, state->elts, fullname);
  pushElStack (state, elttag);

  if (atts != NULL) {
    /* take care to only encode attributes that were explicitly specified */
    int end = XML_GetSpecifiedAttributeCount(state->p);
    for (i = 0; i < end; i += 2)
      {
	preloadChar (state->attPPM, state);
	writeSymbol (state->attPPM, state->symPPM, state->atts, atts[i]);
	writeString (state->attPPM, atts[i + 1]);
      }
  }
  preloadChar (state->attPPM, state);
  encodeChar (state->attPPM,  ENDATT);
}

void
encodeEndElement (void *userData, const char *name)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  exit_characters (state);
  pop_element (state);
  encodeChar (state->eltPPM,  ENDELT);

  popElStack (state);
  if(state->elTop != NULL)
    preloadChar (state->eltPPM, state);
}

void
encodeStartCdata (void *userData)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  /* if in middle of string, end it */
  exit_characters (state);
  /* set state to cs_cdata, so that encodeCharacters will behave */
  state->char_state = cs_cdata;
  /* emit CDATA token */
  encodeChar (state->eltPPM,  CDATA);
}

void
encodeEndCdata (void *userData)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  /* can only be in state cs_cdata; revert to cs_none */
  state->char_state = cs_none;
  /* emit ENDSTRING token */
  encodeChar (state->charPPM,  ENDSTRING);
}

void
encodeComment (void *userData, const XML_Char * data)
{
  xml_enc_state *state = (xml_enc_state *) userData;
  exit_attlist (state);
  exit_characters (state);
  if (state->hasDecl == 0)
    encodeXmlDecl (state, NULL, NULL, -1);
  encodeChar (state->eltPPM,  COMMENT);
  writeString (state->charPPM, data);
}

void
encodeProcessingInstruction (void *userData,
			     const XML_Char * target, const XML_Char * data)
{
  xml_enc_state *state = (xml_enc_state *) userData;
  exit_attlist (state);
  exit_characters (state);
  if (state->hasDecl == 0)
    encodeXmlDecl (state, NULL, NULL, -1);
  encodeChar (state->eltPPM,  PI);
  writeString (state->charPPM, target);
  writeString (state->charPPM, data);
}

void
encodeCharacters (void *userData, const char *ch, int len)
{
  /* only do this when inside root element somewhere, ie depth >0 */
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  enter_characters (state);
  encodeStringLen (state->charPPM,  ch, len);
}

void
encodeEntityRef (void *userData, const XML_Char * s, int len)
{
  xml_enc_state *state;
  state = (xml_enc_state *) userData;
  if (len < 3)
    return;			/* ref needs at least 3 chars: [%&].; */
  exit_characters (state);
  if (s[0] == '&')
    {
      encodeChar (state->eltPPM,  ENTITY);
      encodeStringLen (state->charPPM,  s + 1, len - 2);
      encodeChar (state->charPPM,  ENDSTRING);
    }
  else if (s[0] == '%')
    {
      encodeChar (state->eltPPM,  DTDPENTITY);
      encodeStringLen (state->charPPM,  s + 1, len - 2);
      encodeChar (state->charPPM,  ENDSTRING);
    }
}

void encodeStartNamespaceDecl(void* userData,
			      const XML_Char* prefix,
			      const XML_Char* uri) {
  /* Namespaces are ignored by XMLPPM */
}

void encodeEndNamespaceDecl(void* userData, 
			    const XML_Char* prefix) {
  /* Namespaces are ignored by XMLPPM */
}
