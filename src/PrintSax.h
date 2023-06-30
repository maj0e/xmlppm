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

/* print_sax.h: declarations for "print" SAX event handler */

#include <expat.h>
#include "IFile.h"

void printElementDecl(void *userData,
		      const XML_Char *name,
		      XML_Content *model);

void printAttlistDecl(void		*userData,
		      const XML_Char	*elname,
		      const XML_Char	*attname,
		      const XML_Char	*att_type,
		      const XML_Char	*dflt,
		      int		isrequired);
void printAttributeDecl(void		*userData,
		      const XML_Char	*attname,
		      const XML_Char	*att_type,
		      const XML_Char	*dflt,
		      int		isrequired);
void printStartAttlistDecl(void		*userData,
		      const XML_Char	*elname);

void printEndAttlistDecl(void		*userData);

void printXmlDecl(void		*userData,
		  const XML_Char	*version,
		  const XML_Char	*encoding,
		  int			standalone);

void printEntityDecl(void *userData,
		     const XML_Char *entityName,
		     int is_parameter_entity,
		     const XML_Char *value,
		     int value_length,
		     const XML_Char *base,
		     const XML_Char *systemId,
		     const XML_Char *publicId,
		     const XML_Char *notationName);

void printNotationDecl(void *userData,
		       const XML_Char *notationName,
		       const XML_Char *base,
		       const XML_Char *systemId,
		       const XML_Char *publicId);

void printProcessingInstruction(void *userData,
				const XML_Char *target,
				const XML_Char *data);

void printComment(void *userData, const XML_Char *data);

void printStartCdata(void *userData);

void printEndCdata(void *userData);

void printStartDoctypeDecl(void *userData,
			   const XML_Char *doctypeName,
			   const XML_Char *sysid,
			   const XML_Char *pubid,
			   int has_internal_subset);

void printEndDoctypeDecl(void *userData);

void printStartNamespaceDecl(void *userData,
			     const XML_Char *prefix,
			     const XML_Char *uri);

void printEndNamespaceDecl(void *userData,
			   const XML_Char *prefix);

void printEntityRef(void *userData,
		    const XML_Char *s,
		    int len);

int printExternalEntityRef();

void printStartElement(void *userData,
		       const XML_Char *name,
		       const XML_Char **atts,
		       const XML_Char **nms) ; 

void printEndElement(void *userData,
		     const XML_Char *name); 

void printCharacters(void *userData,
		     const XML_Char *s,
		     int len); 

void ifprint_xml_string(IFILE* ifile, const char* c, int len);
void ifprint_xml_att_value(IFILE* ifile, const char* c);
