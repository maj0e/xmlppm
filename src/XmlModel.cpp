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

/* xmlmodel.c: maintains compression state of XML compressor */

#include <stdio.h>
#include <expat.h>
#include <string.h>
#include "XmlModel.h"



xml_state::xml_state()
{
  /* I think this should work without the -1, but apparently there was an off by one bug here. */
  elts = new StringArray (MAXELTS-1);
  atts = new StringArray (MAXATTS-1);
  nots = new StringArray (MAXNOTS-1);
  ents = new StringArray (MAXENTS-1);
  
  elTop = NULL;
  char_state = cs_none;
  depth = 0;
  hasDtd = 0;
  hasDecl = 0;
  lastAttlistElt = NULL;
  standalone = 1;
}

void
pushElStack (xml_state * state, int elt)
{
  elStackNode *newtop = (elStackNode *) malloc (sizeof (elStackNode));
  newtop->next = state->elTop;
  state->elTop = newtop;
  state->elTop->elem = elt;
}

void
popElStack (xml_state * state)
{
  elStackNode *oldtop = state->elTop;
  state->elTop = oldtop->next;
  free (oldtop);
}

int
getTopEl (xml_state * state)
{
  return (state->elTop->elem);
}
