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
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/validators/common/Grammar.hpp>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>

#include <memory>
#include <string>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <functional>
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
  void readFile(std::string&, bool) throw(std::runtime_error);

  struct Transfer{
    std::string ref;
    long long from;
    long long to;
    double amount;
    std::vector<std::string> tags;
  };


  struct Query{
    std::string ref;
    std::vector<std::shared_ptr<Query>> andQueries;
    std::vector<std::shared_ptr<Query>> orQueries;
    std::vector<std::shared_ptr<Query>> notQueries;
    std::string query;
    bool ready;
    bool error;
    std::vector<std::string> tags;
  };

  struct Create{
    bool error;
    long long account;
    bool accountSet;
    double balance;
    std::string ref;
  };

  bool reset;
  std::vector<Create> creates;
  std::vector<std::tuple<long long, std::string>> balances;
  std::vector<Transfer> transfers;
  std::vector<std::shared_ptr<Query>> queries;
  static std::string translateQuery(std::shared_ptr<Query> q);
  static std::string translateQueryInner(std::vector<std::shared_ptr<Query>> qq, std::string res, std::string op);

private:
  xercesc::XercesDOMParser *m_FileParser;
  void parseElemNode(xercesc::DOMNode *node);
  bool isElem(xercesc::DOMNode *node);
  bool isText(xercesc::DOMNode *node);
  const XMLCh* parseLeafElem(xercesc::DOMNode *node);
  void parseTransferElemNode(xercesc::DOMNode *node);
  void parseCreateElemNode(xercesc::DOMNode *node, Create &create);
  void parseBalanceElemNode(xercesc::DOMNode *node);
  void parseQueryElemNode(xercesc::DOMNode *node, std::shared_ptr<Query> queryPtr);
  void parseQueryRelop(xercesc::DOMNode *node, std::shared_ptr<Query> q, std::string op);

  //  ParserErrorHandler parserErrorHandler;
  // Internal class use only. Hold Xerces data in UTF-16 SMLCh type.
  XMLCh* TAG_root;
  XMLCh* ATTR_reset;
  XMLCh* resetTrue;
  XMLCh* ATTR_ref;
  XMLCh* emptyRef;

  XMLCh* TAG_create;
  XMLCh* TAG_account;
  XMLCh* TAG_balance;

  XMLCh* TAG_transfer;
  XMLCh* TAG_from;
  XMLCh* TAG_to;
  XMLCh* TAG_amount;
  XMLCh* TAG_tag;
  XMLCh* TAG_info;


  XMLCh* TAG_query;
  XMLCh* TAG_and;
  XMLCh* TAG_or;
  XMLCh* TAG_not;
  XMLCh* TAG_equals;
  XMLCh* TAG_less;
  XMLCh* TAG_greater;


  std::tuple<long long, double, std::string> requestTuple;

  Transfer currentTransfer;

  // vector of (ref * from * to * amout * [tags] )

  bool accSet;
  bool balSet;

  std::string balanceRef;

  std::string transferRef;



};


#endif
