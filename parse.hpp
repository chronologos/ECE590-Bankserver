#ifndef XML_PARSER_HPP
#define XML_PARSER_HPP
/**
 *  @file
 *  Class "GetConfig" provides the functions to read the XML data.
 *  @version 1.0
 */
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>

#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

// Error codes

enum {
   ERROR_ARGS = 1,
   ERROR_XERCES_INIT,
   ERROR_PARSE,
   ERROR_EMPTY_DOCUMENT
};

class Parse
{
public:
   Parse();
  ~Parse();
   void readFile(std::string&) throw(std::runtime_error);
   void parseTextNode(xercesc::DOMNode *currentNode);
   void parseElemNode(xercesc::DOMNode *node);
   bool isElem(xercesc::DOMNode *node);
   bool isText(xercesc::DOMNode *node);
   const XMLCh* parseLeafElem(xercesc::DOMNode *node);

private:
   xercesc::XercesDOMParser *m_FileParser;

   // Internal class use only. Hold Xerces data in UTF-16 SMLCh type.
   XMLCh* TAG_root;
   XMLCh* TAG_create;
   XMLCh* TAG_transfer;
   XMLCh* TAG_balance;
   XMLCh* TAG_query;
   XMLCh* TAG_account;
   std::vector<std::pair<long long, double>> createRequests;
   std::pair<long long, double> requestTuple;
   bool accSet;
   bool balSet;
};
#endif
