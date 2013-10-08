/*
  FILE   : interface.c
  HEADER : interface.h
  BAS    : xml.bas
  AUTHOR : *TODO*

  DATE: 

  CONTENT:
  This is the interface.c file for the ScriptBasic module xml

NTLIBS: /link /NODEFAULTLIB:"libcmt.lib" libxml2_a.lib iconv_a.lib ws2_32.lib
UXLIBS: -lxml2
*/

#include <stdio.h>
#include "../../basext.h"

#include <libxml/parser.h>

/*
*TODO*
INSERT THE BASIC CODE THAT WILL GET INTO THE FILE xml.BAS
AFTER THE LINE 'TO_BAS:' AND BEFORE THE LINE END OF THE COMMENT

NOTE THAT SUB AND COMMAND DECLARATIONS ARE CREATED AUTOMATICALLY
FROM THE FUNCTION DEFINTIONS WHEN THE MODULE IS CONFIGURED BEFORE
COMPILATION

TO_BAS:
REM """
The module xml.bas is based on the Gnome libxml2 library.
"""
*/

typedef struct _ModuleObject {
  void *HandleArray;
  }ModuleObject,*pModuleObject;


#define GET_ARGUMENT_POINTER(X,Y) \
  if( besARGNR < Y )return COMMAND_ERROR_ARGUMENT_RANGE;\
  Argument = besARGUMENT(Y);\
  besDEREFERENCE(Argument);\
  if( Argument == NULL )X=NULL; else { \
      if( Argument->vType != VTYPE_STRING ||\
          STRLEN(Argument) != sizeof(void *) ){\
    return COMMAND_ERROR_ARGUMENT_RANGE;\
    }\
  memcpy(&(X),STRINGVALUE(Argument),sizeof(void *));\
  }

#define RETURN_POINTER(X) \
   besALLOC_RETURN_STRING( sizeof( void *) );\
   memcpy(STRINGVALUE(besRETURNVALUE),&(X),sizeof(void *));

/*
*TODO*
ALTER THE VERSION NEGOTIATION CODE IF YOU NEED
*/
besVERSION_NEGOTIATE
  return (int)INTERFACE_VERSION;
besEND

/*
*TODO*
ALTER THE ERROR MESSAGE FUNCTION
*/
besSUB_ERRMSG

  switch( iError ){
    case 0x00080000: return "ERROR HAS HAPPENED";
    }
  return "Unknown xml module error.";
besEND

/*
*TODO*
ALTER THE MODULE INITIALIZATION CODE
*/
besSUB_START
  pModuleObject p;

  besMODULEPOINTER = besALLOC(sizeof(ModuleObject));
  if( besMODULEPOINTER == NULL )return 0;

  p = (pModuleObject)besMODULEPOINTER;
  return 0;
besEND

/*
*TODO*
ALTER THE MODULE FINISH CODE IF NEEDED
*/
besSUB_FINISH
  pModuleObject p;

  p = (pModuleObject)besMODULEPOINTER;
  if( p == NULL )return 0;
  return 0;
besEND

/**
=section ParseFile
=H Parse an XML file and read into memory

You should use this function to read the content of an XML file into memory.
The argument of the function is the name of the file to be read. The function
will return a handle to the structure created during the parsing of the file
content.

=verbatim
 DOC = xml::ParseFile("my_file.xml")
=noverbatim

*/
besFUNCTION(sbxmlParseFile)
  xmlDocPtr doc;
  char *pszFileName;

  besARGUMENTS("z")
    &pszFileName
  besARGEND

  doc = xmlParseFile(pszFileName);
  besFREE(pszFileName);

  besRETURN_POINTER(doc);
besEND

/**
=section NewDoc
=H Create a new empty document


=verbatim
 DOC = xml::NewDoc("1.0")
=noverbatim

*/
besFUNCTION(sbxmlNewDoc)
  xmlDocPtr doc;
  char *pszVersion;

  besARGUMENTS("z")
    &pszVersion
  besARGEND

  doc = xmlNewDoc(pszVersion);

  besFREE(pszVersion);

  besRETURN_POINTER(doc);
besEND

/**
=section FreeDoc
=H Release a document from memory

When an XML document is not needed by the BASIC program any more the
program should free the memory used up by the document calling the
function T<FreeDoc>.

=verbatim
xml::FreeDoc DOC
=noverbatim
*/
besFUNCTION(sbxmlFreeDoc)
  xmlDocPtr doc;

  besARGUMENTS("p")
    &doc
  besARGEND

  xmlFreeDoc(doc);
besEND

/**
=section GetChildren
=H Get the children on a document node

This function should be used to get the handle to the 

*/
besFUNCTION(getchildren)
  xmlDocPtr doc;

  besARGUMENTS("p")
    &doc
  besARGEND

  besRETURN_POINTER(doc->children);
besEND

/**
=section SetChildren
=H Set the children on a document node

This function should be used to get the handle to the 

*/
besFUNCTION(setchildren)
  xmlDocPtr doc;
  xmlNodePtr child;

  besARGUMENTS("pp")
    &doc , &child
  besARGEND
  doc->children = child;
besEND

/**
=section GetNext
=H Get the next node of a document node

This function should be used to get the handle to the 

*/
besFUNCTION(getnext)
  xmlDocPtr doc;

  besARGUMENTS("p")
    &doc
  besARGEND

  besRETURN_POINTER(doc->next);
besEND

/**
=section SetNext
=H Set the next on a document node

This function should be used to get the handle to the 

*/
besFUNCTION(setnext)
  xmlDocPtr doc;
  xmlNodePtr nxt;

  besARGUMENTS("pp")
    &doc , &nxt
  besARGEND
  doc->next = nxt;
besEND

/**
=section GetPrev
=H Get the prev node of a document node

This function should be used to get the handle to the 

*/
besFUNCTION(getprev)
  xmlDocPtr doc;

  besARGUMENTS("p")
    &doc
  besARGEND

  besRETURN_POINTER(doc->prev);
besEND

/**
=section SetPrev
=H Set the next on a document node

This function should be used to get the handle to the 

*/
besFUNCTION(setprev)
  xmlDocPtr doc;
  xmlNodePtr prv;

  besARGUMENTS("pp")
    &doc , &prv
  besARGEND
  doc->prev = prv;
besEND

/**
=section GetParent
=H Get the Parent node of a document node

This function should be used to get the handle to the 

*/
besFUNCTION(getparent)
  xmlDocPtr doc;

  besARGUMENTS("p")
    &doc
  besARGEND

  besRETURN_POINTER(doc->parent);
besEND

/**
=section SetParent
=H Set the next on a document node

This function should be used to get the handle to the 

*/
besFUNCTION(setparent)
  xmlDocPtr doc;
  xmlNodePtr par;

  besARGUMENTS("pp")
    &doc , &par
  besARGEND
  doc->parent = par;
besEND

/**
=section SetProp
=H set the property of a node
*/
besFUNCTION(sbxmlSetProp)
  xmlNodePtr node;
  char *pszName, *pszValue;


  besARGUMENTS("pzz")
    &node, &pszName , &pszValue
  besARGEND

  xmlSetProp(node,pszName,pszValue);

  besFREE(pszName);
  besFREE(pszValue);
besEND

/**
=section GetProp
=H get the property of a node

*/
besFUNCTION(sbxmlGetProp)
  xmlNodePtr node;
  char *pszName;
  char *pszValue;

  besARGUMENTS("pz")
    &node, &pszName
  besARGEND

  pszValue = xmlGetProp(node,pszName);
  besSET_RETURN_STRING( pszValue );
  if( pszValue )xmlMemFree(pszValue);
besEND

/**
=section NewNs
=H Create a new name space defintion

Use this function to create a new name space definition.

=verbatim
  ns = xml::NewNs(node, href,prefix)
=noverbatim
*/
besFUNCTION(sbxmlNewNs)
  xmlNodePtr node;
  xmlChar *href;
  xmlChar *prefix;
  xmlNsPtr ns;

  besARGUMENTS("pzz")
    &node , &href , &prefix
  besARGEND

  ns = xmlNewNs(node,href,prefix);

  besFREE(href);
  besFREE(prefix);
  besRETURN_POINTER(ns);
besEND

/**
=section FreeNs
=H Release a name space definition

Use this function to release a name space definition created by the function
T<xml::NewNs>, which is not needed anymore.

=verbatim
  xml::FreeNs ns
=noverbatim

*/
besFUNCTION(sbxmlFreeNs)
  xmlNsPtr ns;

  besARGUMENTS("p")
    &ns
  besARGEND

  xmlFreeNs(ns);
besEND

/**
=section NewChild
=H Create a new child under a node

Use this function to create a new child under an already existing node.

=verbatim
  newNode = xml::NewChild(parent,nameSpace,name,content)
=noverbatim

Arguments:
=itemize
=item T<parent>     is the parent node. The function will appens a new child node after
                    the last child node of the T<parent> node.
=item T<nameSpace>  is optional handler to the name space. May be T<undef>.
=item T<name>       is the name of the new node, should be a string.
=item T<content>    optional string content.
=noitemize

*/
besFUNCTION(sbxmlNewChild)
  xmlNodePtr node;
  xmlNsPtr ns;
  xmlChar *name;
  xmlChar *content;

  besARGUMENTS("p[p]z[z]")
    &node , &ns , &name , &content
  besARGEND

  node = xmlNewChild(node,ns,name,content);

  besFREE(name);
  besFREE(content);

  besRETURN_POINTER(node);
besEND

/**
=section NewTextChild
=H Create a new text child under a node

Use this function to create a new text child under an already existing node.

=verbatim
  newNode = xml::NewTextChild(parent,nameSpace,name,content)
=noverbatim

Arguments:
=itemize
=item T<parent>     is the parent node. The function will appens a new child node after
                    the last child node of the T<parent> node.
=item T<nameSpace>  is optional handler to the name space. May be T<undef>.
=item T<name>       is the name of the new node, should be a string.
=item T<content>    optional string content.
=noitemize

*/
besFUNCTION(sbxmlNewTextChild)
  xmlNodePtr node;
  xmlNsPtr ns;
  xmlChar *name;
  xmlChar *content;

  besARGUMENTS("p[p]z[z]")
    &node , &ns , &name , &content
  besARGEND

  node = xmlNewTextChild(node,ns,name,content);

  besFREE(name);
  besFREE(content);

  besRETURN_POINTER(node);
besEND

/**
=section NewDocNode
=H Create a new document node

Use this function to create a new document node.

=verbatim
  newNode = xml::NewDocNode(parent,nameSpace,name,content)
=noverbatim

Arguments:
=itemize
=item T<parent>     is the parent node. The function will appens a new child node after
                    the last child node of the T<parent> node.
=item T<nameSpace>  is optional handler to the name space. May be T<undef>.
=item T<name>       is the name of the new node, should be a string.
=item T<content>    optional string content.
=noitemize

*/
besFUNCTION(sbxmlNewDocNode)
  xmlDocPtr doc;
  xmlNodePtr node;
  xmlNsPtr ns;
  xmlChar *name;
  xmlChar *content;

  besARGUMENTS("p[p]z[z]")
    &doc , &ns , &name , &content
  besARGEND

  node = xmlNewDocNode(doc,ns,name,content);

  besFREE(name);
  besFREE(content);

  besRETURN_POINTER(node);
besEND
/**
=section Doc2XML
=H Convert a document to XML text
*/
besFUNCTION(sbxmlDocDumpMemory)
  VARIABLE Argument;
  xmlDocPtr p;
  xmlChar *mem;
  int size;

  GET_ARGUMENT_POINTER(p,1);
  xmlDocDumpMemory(p,&mem,&size);
  besALLOC_RETURN_STRING(size);
  memcpy(STRINGVALUE(besRETURNVALUE),mem,size);

besEND

/*
*TODO*
INSERT HERE THE NAME OF THE FUNCTION AND THE FUNCTION INTO THE
TABLE. THIS TABLE IS USED TO FIND THE FUNCTIONS WHEN THE MODULE
INTERFACE FILE IS COMPILED TO BE LINKED STATIC INTO A VARIATION
OF THE INTERPRETER.
*/

SLFST XML_SLFST[] ={

{ "versmodu" , versmodu },
{ "bootmodu" , bootmodu },
{ "finimodu" , finimodu },
{ "emsgmodu" , emsgmodu },
{ "sbxmlParseFile"     , sbxmlParseFile     },
{ "sbxmlNewDoc"        , sbxmlNewDoc        },
{ "sbxmlFreeDoc"       , sbxmlFreeDoc       },
{ "sbxmlNewNs"         , sbxmlNewNs         },
{ "sbxmlFreeNs"        , sbxmlFreeNs        },
{ "getchildren"        , getchildren        },
{ "setchildren"        , setchildren        },
{ "getnext"            , getnext            },
{ "setnext"            , setnext            },
{ "getprev"            , getprev            },
{ "setprev"            , setprev            },
{ "sbxmlSetProp"       , sbxmlSetProp       },
{ "sbxmlNewChild"      , sbxmlNewChild      },
{ "sbxmlNewTextChild"  , sbxmlNewTextChild  },
{ "sbxmlNewDocNode"    , sbxmlNewDocNode    },
{ "sbxmlDocDumpMemory" , sbxmlDocDumpMemory },
{ NULL , NULL }
  };
