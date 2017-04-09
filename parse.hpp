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

#include <xercesc/util/XMLString.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/validators/common/Grammar.hpp>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>

#include <string>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

// Error codes

enum {
   ERROR_ARGS = 1,
   ERROR_XERCES_INIT,
   ERROR_PARSE,
   ERROR_EMPTY_DOCUMENT
};

class ParserErrorHandler: public xercesc::ErrorHandler
{
public:
  void warning(const xercesc::SAXParseException&);
  void error(const xercesc::SAXParseException&);
  void fatalError(const xercesc::SAXParseException&);
  void resetErrors();

private:
  void reportParseException(const xercesc::SAXParseException&);
};

class Parse
{
public:
   Parse();
  ~Parse();
   void readFile(std::string&) throw(std::runtime_error);
  //  void parseTextNode(xercesc::DOMNode *currentNode);
   void parseElemNode(xercesc::DOMNode *node);
   bool isElem(xercesc::DOMNode *node);
   bool isText(xercesc::DOMNode *node);
   const XMLCh* parseLeafElem(xercesc::DOMNode *node);

   void parseTransferElemNode(xercesc::DOMNode *node);
   void parseCreateElemNode(xercesc::DOMNode *node);
   void parseBalanceElemNode(xercesc::DOMNode *node);


private:
   xercesc::XercesDOMParser *m_FileParser;
  //  ParserErrorHandler parserErrorHandler;
   // Internal class use only. Hold Xerces data in UTF-16 SMLCh type.
   XMLCh* TAG_root;
   XMLCh* ATTR_reset;
   XMLCh* resetTrue;
   XMLCh* ATTR_ref;
   XMLCh* emptyRef;


   XMLCh* TAG_create;

   XMLCh* TAG_transfer;
   XMLCh* TAG_from;
   XMLCh* TAG_to;
   XMLCh* TAG_amount;
   XMLCh* TAG_tag;


   XMLCh* TAG_balance;
   XMLCh* TAG_query;
   XMLCh* TAG_account;


   bool reset;

   std::vector<std::tuple<long long, double, std::string>> creates;
   std::tuple<long long, double, std::string> requestTuple;
   struct Transfer{
     std::string ref;
     long long from;
     long long to;
     double amount;
     std::vector<std::string> tags;
   };
   std::vector<Transfer> transfers;
   Transfer currentTransfer;

   // vector of (ref * from * to * amout * [tags] )
   bool accSet;
   bool balSet;

   std::string balanceRef;
   std::string transferRef;
   std::vector<std::tuple<long long, std::string>> balances;


};


#endif
