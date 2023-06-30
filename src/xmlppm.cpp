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

/* xmlppm.cpp: an XML compressor */

#include <assert.h>
#include <errno.h>
#include <expat.h>
#include <stdio.h>
#include <string.h>
#include "Args.h"
#include "XmlModel.h"
#include "PrintSax.h"
#include "EncodeSax.h"
#include "SubAlloc.hpp"


#define BUFFSIZE	8192

char Buff[BUFFSIZE];


void
parseInput (xml_enc_state * state, FILE * infile)
{
  XML_Parser p = XML_ParserCreate (NULL);
  state->p = p;
  if (!p)
    {
      fprintf (stderr, "Couldn't allocate memory for parser\n");
      exit (-1);
    }


  XML_SetXmlDeclHandler (p, encodeXmlDecl);
  if(!state->standalone) {
    XML_SetExternalEntityRefHandler (p, encodeExternalEntityRef);
    XML_SetParamEntityParsing (p, XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
  }
  XML_SetDoctypeDeclHandler   (p, encodeStartDoctypeDecl, 
			       encodeEndDoctypeDecl);
  XML_SetElementDeclHandler   (p, encodeElementDecl);
  XML_SetAttlistDeclHandler   (p, encodeAttlistDecl);
  XML_SetEntityDeclHandler    (p, encodeEntityDecl);
  XML_SetNotationDeclHandler  (p, encodeNotationDecl);
  XML_SetNamespaceDeclHandler (p, encodeStartNamespaceDecl,
			       encodeEndNamespaceDecl);
  XML_SetElementHandler       (p, encodeStartElement, 
			       encodeEndElement);
  XML_SetCharacterDataHandler (p, encodeCharacters);
  XML_SetProcessingInstructionHandler (p, encodeProcessingInstruction);
  XML_SetCommentHandler       (p, encodeComment);
  XML_SetCdataSectionHandler  (p, encodeStartCdata, encodeEndCdata);
  XML_SetDefaultHandler       (p, encodeEntityRef);
  XML_SetUserData             (p, state);

  for (;;)
    {
      int done;
      int len;

      len = fread (Buff, 1, BUFFSIZE, infile);
      if (ferror (infile))
	{
	  fprintf (stderr, "Read error\n");
	  exit (-1);
	}
      done = feof (infile);

      if (!XML_Parse (p, Buff, len, done))
	{
	  fprintf (stderr, "Parse error at line %d:\n%s\n",
		   XML_GetCurrentLineNumber (p),
		   XML_ErrorString (XML_GetErrorCode (p)));
	  exit (-1);
	}

      if (done)
	break;
    }
  encodeChar (state->eltPPM, ENDELT);
  XML_ParserFree (p);
}

void compress(args_t args) {
  writeHeader(&args,args.outfp);
  xml_enc_state *state = new xml_enc_state ();
  struct level_settings s = settings[args.level];
  state->file = args.outfp;
  state->eltPPM = new PPM_ENCODER (s.elt.size,s.elt.order,args.outfp);
  state->symPPM = new PPM_ENCODER (s.sym.size,s.sym.order,args.outfp);
  state->charPPM = new PPM_ENCODER (s.chr.size,s.chr.order,args.outfp);
  state->attPPM = new PPM_ENCODER (s.att.size,s.att.order,args.outfp);
  
  /* compress the file */
  ariInitEncoder (args.outfp);
  parseInput (state, args.infp);
  ARI_FLUSH_ENCODER (args.outfp);
  
  /* clean up */
  fclose (args.infp);
  fclose (args.outfp);
  free (state);
}

#ifdef FIXED_MODEL
void compress_with_model(args_t args) {
  
  xml_enc_state *state = new xml_enc_state ();
  struct level_settings s = settings[args.level];
  FILE* devnull = fopen("/dev/null","w");
  assert(devnull);
  state->file = devnull;  
  state->eltPPM = new PPM_ENCODER (s.elt.size,s.elt.order,devnull);
  state->symPPM = new PPM_ENCODER (s.sym.size,s.sym.order,devnull);
  state->charPPM = new PPM_ENCODER (s.chr.size,s.chr.order,devnull);
  state->attPPM = new PPM_ENCODER (s.att.size,s.att.order,devnull);
  
  /* compress the file */
  ariInitEncoder (devnull);
  parseInput (state, args.modelfp);
  ARI_FLUSH_ENCODER (devnull);
  
  state->file = args.outfp;  
  state->eltPPM->outfile = args.outfp;
  state->symPPM->outfile = args.outfp;
  state->charPPM->outfile = args.outfp;
  state->attPPM->outfile = args.outfp;
  if(args.freeze) {
    state->eltPPM->Frozen = true;
    state->symPPM->Frozen = true;
    state->charPPM->Frozen = true;
    state->attPPM->Frozen = true;
  }
  writeHeader(&args,args.outfp);
  
  ariInitEncoder (args.outfp);
  parseInput (state, args.infp);
  ARI_FLUSH_ENCODER (args.outfp);
  
  
  /* clean up */
  fclose (args.infp);
  fclose (args.outfp);
  fclose (args.modelfp);
  free (state);
}
#endif

int
main (int argc, char **argv) {
  args_t args = getEncoderArgs (argc, argv);
  
#ifdef FIXED_MODEL
  if(args.modelfile == NULL) {
    compress(args);
    return 0;
  } else {
    compress_with_model(args);
  }
#else
  compress(args);
#endif

  return 0;
}
