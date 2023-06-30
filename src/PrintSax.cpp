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

/* PrintSax.h: a "print" SAX event handler */

#include <expat.h>
#include <stdio.h>
#include <string.h>
#include "IFile.h"
#include "PrintSax.h"

/* todo: move this state into userData */
int in_internal_subset = 0;


void printContentModel (IFILE * file, XML_Content * model);

void
printQuant (IFILE * file, enum XML_Content_Quant quant)
{
  switch (quant)
    {
    case XML_CQUANT_NONE:
      break;
    case XML_CQUANT_OPT:
      ifputc ('?', file);
      break;
    case XML_CQUANT_REP:
      ifputc ('*', file);
      break;
    case XML_CQUANT_PLUS:
      ifputc ('+', file);
      break;
    }
}

void
printNameModel (IFILE * file, XML_Content * model)
{
  ifprintf (file, "%s", model->name);
  printQuant (file, model->quant);
}

void
printMixedModel (IFILE * file, XML_Content * model)
{
  if (model->quant == XML_CQUANT_NONE)
    {
      ifprintf (file, "(#PCDATA)");
    }
  else
    {
      unsigned int i;
      ifprintf (file, "(#PCDATA");
      for (i = 0; i < model->numchildren; i++)
	{
	  ifputc ('|', file);
	  printNameModel (file, model->children + i);
	}
      ifprintf (file, ")*");
    }
}



void
printContentModels (IFILE * file, XML_Content * model, char sep)
{
  unsigned int i;
  ifprintf (file, "(");
  printContentModel (file, model->children);

  for (i = 1; i < model->numchildren; i++)
    {
      ifputc (sep, file);
      printContentModel (file, model->children + i);
    }
  ifprintf (file, ")");
  printQuant (file, model->quant);
}

void
printContentModel (IFILE * file, XML_Content * model)
{
  switch (model->type)
    {
    case XML_CTYPE_EMPTY:
      ifprintf (file, "EMPTY");
      break;
    case XML_CTYPE_ANY:
      ifprintf (file, "ANY");
      break;
    case XML_CTYPE_MIXED:
      printMixedModel (file, model);
      break;
    case XML_CTYPE_NAME:
      printNameModel (file, model);
      break;
    case XML_CTYPE_CHOICE:
      printContentModels (file, model, '|');
      break;
    case XML_CTYPE_SEQ:
      printContentModels (file, model, ',');
      break;
    }
}

void
printElementDecl (void *userData, const XML_Char * name, XML_Content * model)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "<!ELEMENT %s ", name);
  printContentModel (file, model);
  ifprintf (file, ">\n");
}



void
printAttributeDecl (void *userData,
		    const XML_Char * attname,
		    const XML_Char * att_type,
		    const XML_Char * dflt, int isrequired)
{
  IFILE *file = (IFILE *) userData;
  ifflush (file);
  ifprintf (file, "\n\t %s", attname);
  if (att_type != NULL)
    ifprintf (file, " %s", att_type);

  if (dflt == NULL)
    if (isrequired == 0)
      ifprintf (file, " #IMPLIED");
    else
      ifprintf (file, " #REQUIRED");
  else if (isrequired == 0)
    ifprintf (file, " \"%s\"", dflt);
  else
    ifprintf (file, " #FIXED \"%s\"", dflt);
}

void
printStartAttlistDecl (void *userData, const XML_Char * elname)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "<!ATTLIST %s", elname);
}

void
printEndAttlistDecl (void *userData)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, ">\n");
}


void
printAttlistDecl (void *userData,
		  const XML_Char * elname,
		  const XML_Char * attname,
		  const XML_Char * att_type,
		  const XML_Char * dflt, int isrequired)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "<!ATTLIST %s", elname);
  printAttributeDecl (userData, attname, att_type, dflt, isrequired);
  ifprintf (file, ">\n");
}

void
printXmlDecl (void *userData,
	      const XML_Char * version,
	      const XML_Char * encoding, int standalone)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "<?xml version=\"%s\"", version);
  if (encoding != NULL)
    ifprintf (file, " encoding=\"%s\"", encoding);
  if (standalone != -1)
    ifprintf (file, " standalone=\"%s\"", standalone == 0?"no":"yes");
  ifprintf (file, "?>\n");
}

void
printEntityDecl (void *userData,
		 const XML_Char * entityName,
		 int is_parameter_entity,
		 const XML_Char * value,
		 int value_length,
		 const XML_Char * base,
		 const XML_Char * systemId,
		 const XML_Char * publicId, const XML_Char * notationName)
{
  IFILE *file = (IFILE *) userData;
  if (is_parameter_entity)
    ifprintf (file, "<!ENTITY %% %s ", entityName);
  else
    ifprintf (file, "<!ENTITY %s ", entityName);
  if (value != NULL)		/* internal entity */
    {
      ifputc ('"', file);
      ifwrite (value, sizeof (char), value_length, file);
      ifputc ('"', file);
    }
  else				/* external entity , systemId != NULL */
    {
      if (publicId != NULL)
	ifprintf (file, "PUBLIC \"%s\" \"%s\"", publicId, systemId);
      else
	ifprintf (file, "SYSTEM \"%s\"", systemId);
      if (!is_parameter_entity && notationName != NULL)
	ifprintf (file, " NDATA %s", notationName);
    }
  ifprintf (file, ">\n");
}

void
printProcessingInstruction (void *userData,
			    const XML_Char * target, const XML_Char * data)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "<?%s %s?>", target, data ? data : "");
}

void
printComment (void *userData, const XML_Char * data)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "<!--%s-->", data ? data : "");
}

void
printStartCdata (void *userData)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "<![CDATA[");

}

void
printEndCdata (void *userData)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "]]>");
}

void
printStartDoctypeDecl (void *userData,
		       const XML_Char * doctypeName,
		       const XML_Char * sysid,
		       const XML_Char * pubid, int has_internal_subset)
{
  IFILE *file = (IFILE *) userData;

  if (sysid != NULL)
    {
      if (pubid != NULL)
	ifprintf (file, "<!DOCTYPE %s PUBLIC \"%s\" \"%s\"",
		 doctypeName, pubid, sysid);
      else
	ifprintf (file, "<!DOCTYPE %s SYSTEM \"%s\"", doctypeName, sysid);
    }
  else
    ifprintf (file, "<!DOCTYPE %s", doctypeName);
  if (has_internal_subset)
    {
      ifprintf (file, " [\n");
      in_internal_subset = 1;
    }
}

void
printEndDoctypeDecl (void *userData)
{
  IFILE *file = (IFILE *) userData;
  if (in_internal_subset)
    {
      ifprintf (file, "]");
      in_internal_subset = 0;
    }
  ifprintf (file, ">\n");
}

void
printEntityRef (void *userData, const XML_Char * s, int len)
{
  IFILE *file = (IFILE *) userData;
  if (s[0] == '&' || s[0] == '%')
    {
      ifwrite (s, sizeof (char), len, file);
    }
}



void
printAttributes (IFILE * file, const XML_Char ** atts)
{
  int i;
  // DEBUG
  ifflush(file);
  if (atts == NULL) return;
  /* more logic here to determine correct quoting */
  for (i = 0; atts[i] != NULL; i += 2) {
    ifprintf (file, " %s=\"", atts[i]);
    if (atts[i + 1] != NULL) 
      ifprint_xml_att_value(file,atts[i+1]);
    ifprintf(file,"\"");
  }
}

void 
printNamespaces (IFILE * file, const XML_Char ** nms)
{
  int i;
  if (nms == NULL) return;
  /* more logic here to determine correct quoting */
  for (i = 0; nms[i] != NULL; i += 2)
    ifprintf (file, " xmlns:%s=\"%s\"", nms[i], nms[i + 1]);
}

void
printStartElement (void *userData,
		   const XML_Char * name, const XML_Char ** atts,
		   const XML_Char ** nms)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "<%s", name);
  printAttributes (file, atts);
  printNamespaces (file, nms);
  ifprintf (file, ">");
}

void
printEndElement (void *userData, const XML_Char * name)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "</%s>", name);
}


void
printCharacters (void *userData, const XML_Char * s, int len)
{
  IFILE *file = (IFILE *) userData;
  ifwrite (s, sizeof (char), len, file);
}


void
printNotationDecl (void *userData,
		   const XML_Char * notationName,
		   const XML_Char * base,
		   const XML_Char * systemId, const XML_Char * publicId)
{
  IFILE *file = (IFILE *) userData;
  ifprintf (file, "<!NOTATION %s ", notationName);
  if (systemId != NULL && publicId != NULL)
    ifprintf (file, "PUBLIC \"%s\" \"%s\">\n", publicId, systemId);
  else if (systemId != NULL)
    ifprintf (file, "SYSTEM \"%s\">\n", systemId);
  else if (publicId != NULL)
    ifprintf (file, "PUBLIC \"%s\">\n", publicId);
  else
    fprintf (stderr, "Notation with neither system nor public ID");
}



void ifprint_xml_string(IFILE* ifile, const char* str, int len)
{
  int i;
  for(i=0;i<len;i++)
    {
      switch(str[i])
	{
	case '&':
	  ifputs("&amp;",ifile);
	  break;
	case '<':
	  ifputs("&lt;",ifile);
	  break;
	case '>':
	  ifputs("&gt;",ifile);
	  break;
	case 0x0D:
	  ifputs("&#xD;",ifile);
	  break;
	default:
	  ifputc(str[i],ifile);
	  break;
	}
    }
}

void ifprint_xml_att_value(IFILE* ifile, const char* str)
{
  const char* c;
  for(c = str;*c != '\0';c++)
    {
      switch(*c)
	{
	case '&':
	  ifputs("&amp;",ifile);
	  break;
	case '<':
	  ifputs("&lt;",ifile);
	  break;
	case '\"':
	  ifputs("&quot;",ifile);
	  break;
	case 0x09:
	  ifputs("&#x9;",ifile);
	  break;
	case 0x0A:
	  ifputs("&#xA;",ifile);
	  break;
	case 0x0D:
	  ifputs("&#xD;",ifile);
	  break;
	default:
	  ifputc(*c,ifile);
	  break;
	}
    }
}
