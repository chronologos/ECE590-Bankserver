#include <iostream>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "parse.hpp"

using namespace xercesc;
using namespace std;

/**
 *  Constructor initializes xerces-C libraries.
 *  The XML tags and attributes which we seek are defined.
 *  The xerces-C DOM parser infrastructure is initialized.
 */


 void ParserErrorHandler::reportParseException(const xercesc::SAXParseException& ex)
 {
   char* msg = XMLString::transcode(ex.getMessage());
   fprintf(stderr, "at line %llu column %llu, %s\n",
           ex.getLineNumber(), ex.getColumnNumber(), msg);
   XMLString::release(&msg);
 }

 void ParserErrorHandler::warning(const xercesc::SAXParseException& ex)
 {
     reportParseException(ex);
 }

 void ParserErrorHandler::error(const xercesc::SAXParseException& ex)
 {
     reportParseException(ex);
 }

 void ParserErrorHandler::fatalError(const xercesc::SAXParseException& ex)
 {
     reportParseException(ex);
 }

 void ParserErrorHandler::resetErrors()
 {
 }


Parse::Parse() {
  try {
    XMLPlatformUtils::Initialize(); // Initialize Xerces infrastructure
  } catch (XMLException &e) {
    char *message = XMLString::transcode(e.getMessage());
    cerr << "XML toolkit initialization error: " << message << endl;
    XMLString::release(&message);
    // throw exception here to return ERROR_XERCES_INIT
  }

  // Tags and attributes used in XML file.
  // Can't call transcode till after Xerces Initialize()
  TAG_root = XMLString::transcode("root");
  TAG_create = XMLString::transcode("create");
  TAG_balance = XMLString::transcode("balance");
  TAG_query = XMLString::transcode("query");
  TAG_and = XMLString::transcode("and");
  TAG_or = XMLString::transcode("or");
  TAG_not = XMLString::transcode("not");
  TAG_equals = XMLString::transcode("equals");
  TAG_less = XMLString::transcode("less");
  TAG_greater = XMLString::transcode("greater");
  TAG_account = XMLString::transcode("account");
  ATTR_reset = XMLString::transcode("reset");
  ATTR_ref = XMLString::transcode("ref");
  emptyRef = XMLString::transcode("");
  resetTrue = XMLString::transcode("true");

  TAG_transfer = XMLString::transcode("transfer");
  TAG_from = XMLString::transcode("from");
  TAG_to = XMLString::transcode("to");
  TAG_amount = XMLString::transcode("amount");
  TAG_tag = XMLString::transcode("tag");

  m_FileParser = new XercesDOMParser;
  // ParserErrorHandler parserErrorHandler;

  creates = std::vector<std::tuple<long long, double, std::string>>();
  requestTuple = std::tuple<long long, double, std::string>();
  accSet = false;
  balSet = false;
  reset = false;
  balanceRef = "";
  balances = std::vector<std::tuple<long long, std::string>>();
  struct Transfer currentTransfer;
  transfers = std::vector<Transfer>();
  // vector of (ref * from * to * amout * [tags] )


  // requestTuple holds the current create request as a
  // (account no * balance) tuple.
}

/**
 *  Class destructor frees memory used to hold the XML tag and
 *  attribute definitions. It als terminates use of the xerces-C
 *  framework.
 */

Parse::~Parse() {
  // Free memory

  delete m_FileParser;
  // if(m_OptionA)   XMLString::release( &m_OptionA );
  // if(m_OptionB)   XMLString::release( &m_OptionB );

  try {
    XMLString::release(&TAG_root);
    XMLString::release(&TAG_create);
    XMLString::release(&TAG_transfer);
    XMLString::release(&TAG_balance);
    XMLString::release(&TAG_query);
    XMLString::release(&TAG_root);
    XMLString::release(&ATTR_reset);
    XMLString::release(&resetTrue);
    XMLString::release(&ATTR_ref);
    XMLString::release(&emptyRef);

    XMLString::release(&TAG_create);
    XMLString::release(&TAG_account);
    XMLString::release(&TAG_balance);

    XMLString::release(&TAG_transfer);
    XMLString::release(&TAG_from);
    XMLString::release(&TAG_to);
    XMLString::release(&TAG_amount);
    XMLString::release(&TAG_tag);

    XMLString::release(&TAG_query);
    XMLString::release(&TAG_and);
    XMLString::release(&TAG_or);
    XMLString::release(&TAG_not);
    XMLString::release(&TAG_equals);
    XMLString::release(&TAG_less);
    XMLString::release(&TAG_greater);

  } catch (...) {
    cerr << "Unknown exception encountered in TagNamesdtor" << endl;
  }

  // Terminate Xerces

  try {
    XMLPlatformUtils::Terminate(); // Terminate after release of memory
  } catch (xercesc::XMLException &e) {
    char *message = xercesc::XMLString::transcode(e.getMessage());

    cerr << "XML ttolkit teardown error: " << message << endl;
    XMLString::release(&message);
  }
}

/**
 *  This function:
 *  - Tests the access and availability of the XML configuration file.
 *  - Configures the xerces-c DOM parser.
 *  - Reads and extracts the pertinent information from the XML config file.
 *
 *  @param in configFile The text string name of the HLA configuration file.
 */


void Parse::readFile(string &configFile, bool isString) throw(std::runtime_error) {
  // Test to see if the file is ok.

  struct stat fileStatus;

  errno = 0;
  if (!isString && stat(configFile.c_str(), &fileStatus) == -1) // ==0 ok; ==-1 error
  {
    if (errno == ENOENT) // errno declared by include file errno.h
      throw(std::runtime_error(
          "Path file_name does not exist, or path is an empty string."));
    else if (errno == ENOTDIR)
      throw(std::runtime_error("A component of the path is not a directory."));
    else if (errno == ELOOP)
      throw(std::runtime_error(
          "Too many symbolic links encountered while traversing the path."));
    else if (errno == EACCES)
      throw(std::runtime_error("Permission denied."));
    else if (errno == ENAMETOOLONG)
      throw(std::runtime_error("File can not be read\n"));
  }

  // Configure DOM parser.
  if (m_FileParser->loadGrammar("./bank.xsd", Grammar::SchemaGrammarType) == NULL){
        fprintf(stderr, "couldn't load schema\n");
        return;
  }

  // m_FileParser->setErrorHandler(&parserErrorHandler);
  m_FileParser->setValidationScheme(XercesDOMParser::Val_Auto);
  m_FileParser->setDoNamespaces(false);
  m_FileParser->setDoSchema(false);
  // m_FileParser->setValidationConstraintFatal(true);
  // auto m_OptionA = 1;
  // auto m_OptionB = 1;

  try {
    if (isString){
      xercesc::MemBufInputSource myxml_buf((const XMLByte*)configFile.c_str(), configFile.size(),
                                           "configFile (in memory)");
      m_FileParser->parse(myxml_buf);
    }
    else{
      m_FileParser->parse(configFile.c_str());
    }
    if (m_FileParser->getErrorCount() != 0){
      cout << "XML file does not conform to schema: " << m_FileParser->getErrorCount() << " errors found." << endl;
      return;
    }
    // no need to free this pointer - owned by the parent parser object
    DOMDocument *xmlDoc = m_FileParser->getDocument();
    DOMElement *elementRoot = xmlDoc->getDocumentElement();
    if (!elementRoot)
      throw(std::runtime_error("empty XML document"));
    if (XMLString::equals(elementRoot->getAttribute(ATTR_reset), resetTrue)){
       reset = true;
    }

    DOMNodeList *children = elementRoot->getChildNodes();
    const XMLSize_t nodeCount = children->getLength();

    for (XMLSize_t i = 0; i < nodeCount; ++i) {
      DOMNode *currentNode = children->item(i);
      cout << "current node is " << XMLString::transcode(currentNode->getNodeName()) << endl;
      if (isElem(currentNode)) {
        Parse::parseElemNode(currentNode);
      }
      cout << "------------------------" << endl;
    }
    // DEBUG PRINTS
    cout << "Done! Printing final creates vector:" << endl;
    for (auto i : creates){
      cout << "(" << std::get<0>(i) << "," << std::get<1>(i) << "," << std::get<2>(i) << ")" << endl;
    }
    cout << "Printing final balances vector:" << endl;
    for (auto i : balances){
      cout << "(" << std::get<0>(i) << "," << std::get<1>(i) << ")" << endl;
    }
    cout << "reset is " << (reset? "true" : "false") << endl;
    cout << "Printing final transfers vector:" << endl;
    for (auto i : transfers){
      cout << "ref: " << i.ref << " from: " << i.from << " to: " << i.to << " amount: " << i.amount << endl;
      cout << "tags:" << endl;
    }
  }

  catch (xercesc::XMLException &e) {
    char *message = xercesc::XMLString::transcode(e.getMessage());
    ostringstream errBuf;
    errBuf << "Error parsing file: " << message << flush;
    XMLString::release(&message);
  }
}

// void Parse::parseTextNode(DOMNode *currentNode) {
//   cout << "Text detected in currentNode:" << endl;
//   DOMText *currentTextData =
//       dynamic_cast<xercesc::DOMText *>(currentNode);
//   cout << XMLString::transcode(currentTextData->getWholeText()) << endl;
// }

bool Parse::isElem(DOMNode *node){
  return (node->getNodeType() && node->getNodeType() == DOMNode::ELEMENT_NODE);
}

bool Parse::isText(DOMNode *node){
  return (node->getNodeType() && node->getNodeType() == DOMNode::TEXT_NODE);
}

void Parse::parseElemNode(DOMNode *node){
  // Found node which is an Element. Re-cast node as element
  DOMElement *currentElement = dynamic_cast<xercesc::DOMElement *>(node);
  DOMNodeList *children = node->getChildNodes();
  const XMLSize_t count = children->getLength();

  if (XMLString::equals(currentElement->getTagName(), TAG_create)) {
    const XMLCh* ref = currentElement->getAttribute(ATTR_ref);
    if (!XMLString::equals(ref, emptyRef)){
      char * refStr = XMLString::transcode(ref);
      requestTuple = std::make_tuple(0, 0, refStr);
      XMLString::release(&refStr);
    }
    for (XMLSize_t i = 0; i < count; ++i) {
      parseCreateElemNode(children->item(i));
    }
  }

  else if (XMLString::equals(currentElement->getTagName(), TAG_balance)){
    const XMLCh* ref = currentElement->getAttribute(ATTR_ref);
    balanceRef = XMLString::transcode(ref);
    for (XMLSize_t i = 0; i < count; ++i) {
      parseBalanceElemNode(children->item(i));
    }
  }

  else if (XMLString::equals(currentElement->getTagName(), TAG_transfer)) {
    const XMLCh* ref = currentElement->getAttribute(ATTR_ref);
    transferRef = XMLString::transcode(ref);
    currentTransfer.ref=transferRef;
    if (isElem(node)) {
      for (XMLSize_t i = 0; i < count; ++i) {
        parseTransferElemNode(children->item(i));
      }
      transfers.push_back(currentTransfer);
      struct Transfer currentTransfer;
    }
  }
}

void Parse::parseBalanceElemNode(DOMNode *node){
  if (isElem(node)){
    const XMLCh* accountNumber = parseLeafElem(node);
    char* accountNumberStr = XMLString::transcode(accountNumber);
    balances.push_back(std::make_tuple(stoll(accountNumberStr), balanceRef));
  }
}

void Parse::parseCreateElemNode(DOMNode *node){
  if (isElem(node)){
    DOMElement *currentElement = dynamic_cast<xercesc::DOMElement *>(node);
    if (XMLString::equals(currentElement->getTagName(),TAG_account)) {
      cout << "TAG_account found" << endl;

      // TODO
      const XMLCh* accountNumber = parseLeafElem(node);
      char* accountNumberStr = XMLString::transcode(accountNumber);
      if (accSet == true){
        cout << "ERROR, accountNumber was previously set." << endl;
        balSet = false;
        accSet = false;
        requestTuple = std::make_tuple(0, 0, "");
      }
      else {
        double second = std::get<1>(requestTuple);
        requestTuple = std::make_tuple(std::stoll(accountNumberStr), second, std::get<2>(requestTuple));
        // cout << "accountNumberStr is " << accountNumberStr << endl;

        accSet = true;
        if (balSet && accSet) {
          cout << "requestTuple is " << std::get<0>(requestTuple) << " " << std::get<1>(requestTuple) << endl;
          creates.push_back(requestTuple);
          balSet = false;
          accSet = false;
          requestTuple = std::make_tuple(0, 0, "");

        }
      }
      cout << "releasing" << endl;
      XMLString::release(&accountNumberStr);

    }
    else if (XMLString::equals(dynamic_cast<xercesc::DOMElement *>(node->getParentNode())->getTagName(), TAG_create)){
      const XMLCh* balance = parseLeafElem(node);
      char* balanceStr = XMLString::transcode(balance);
      if (balSet == true){
        cout << "ERROR, balance was previously set." << endl;
        balSet = false;
        accSet = false;
        requestTuple = std::make_tuple(0, 0, "");
      }
      else {
        long long first = std::get<0>(requestTuple);
        requestTuple = std::make_tuple(first, std::stod(balanceStr), std::get<2>(requestTuple));
        balSet = true;
        if (balSet && accSet) {
          cout << "requestTuple is " << std::get<0>(requestTuple) << " " << std::get<1>(requestTuple)<< endl;
          creates.push_back(requestTuple);
          balSet = false;
          accSet = false;
          requestTuple = std::make_tuple(0, 0, "");
        }
      }
      cout << "releasing" << endl;
      XMLString::release(&balanceStr);
    }
  }
}

void Parse::parseTransferElemNode(DOMNode *node){
  // Found node which is an Element. Re-cast node as element
  cout << "im in " << endl;
  if (isElem(node)){
    DOMElement *currentElement = dynamic_cast<xercesc::DOMElement *>(node);

    if (XMLString::equals(currentElement->getTagName(), TAG_amount)) {
      const XMLCh* transferAmount = parseLeafElem(node);
      char* transferAmountStr = XMLString::transcode(transferAmount);
      cout << "transferring: " << transferAmountStr << endl;
      currentTransfer.amount = stod(transferAmountStr);
      XMLString::release(&transferAmountStr);
    }
    else if (XMLString::equals(currentElement->getTagName(), TAG_from)) {
      const XMLCh* from = parseLeafElem(node);
      char* fromStr = XMLString::transcode(from);
      cout << "from:" << fromStr << endl;
      currentTransfer.from = stoll(fromStr);
      XMLString::release(&fromStr);
    }
    else if (XMLString::equals(currentElement->getTagName(), TAG_to)) {
      const XMLCh* to = parseLeafElem(node);
      char* toStr = XMLString::transcode(to);
      cout << "to: " << toStr << endl;
      currentTransfer.to = stoll(toStr);
      XMLString::release(&toStr);
    }
    else if (XMLString::equals(currentElement->getTagName(), TAG_tag)) {
      const XMLCh* tag = parseLeafElem(node);
      char* tagStr = XMLString::transcode(tag);
      cout << "tag: " << tagStr << endl;
      currentTransfer.tags.push_back(tagStr);
      XMLString::release(&tagStr);
    }
    else {
      cout << "??" << endl;
    }
  }
  else {
    cout << "?" << endl;
  }
}


const XMLCh* Parse::parseLeafElem(DOMNode *node){
  DOMNodeList *children = node->getChildNodes();
  const XMLSize_t count = children->getLength();
  for (XMLSize_t i = 0; i < count; ++i) {
    if(isText(children->item(i))){
      DOMText *currentTextData = dynamic_cast<xercesc::DOMText *>(children->item(i));
      const XMLCh* data = currentTextData->getWholeText(); // TODO need to free XMLCh from transcode
      // cout << XMLString::transcode(data) << endl;
      return data;
    }
  }
  // DOMElement *elem = dynamic_cast<xercesc::DOMElement *>(node);
}

// #ifdef MAIN_TEST
// /* This main is provided for unit test of the class. */
//
//
// int main() {
//   string configFile = "./testfiles/x0.xml"; // file to parse. Get ambigious
//                                                 // segfault otherwise.
//   Parse parser;
//   parser.readFile(configFile);
//   return 0;
// }
// #endif
