#ifndef DECODESAX_H_
#define DECODESAX_H_

#include <expat.h>
#include "XmlModel.h"

XML_Content *
decodeChoiceModel (xml_dec_state * state);

XML_Content *
decodeContentModel (xml_dec_state * state);

void 
decodeElementList (xml_dec_state * state);

void
decodeXML (xml_dec_state * state);


#endif /*DECODESAX_H_*/
