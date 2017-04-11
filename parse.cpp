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
  TAG_info = XMLString::transcode("info");


  m_FileParser = new XercesDOMParser;
  // ParserErrorHandler parserErrorHandler;

  creates = std::vector<Create>();
  requestTuple = std::tuple<long long, double, std::string>();
  // requestTuple holds the current create request as a
  // (account no * balance) tuple.
  reset = false;
  balanceRef = "";
  balances = std::vector<std::tuple<long long, std::string>>();

  struct Transfer currentTransfer = {};

  transfers = std::vector<Transfer>();
  queries = std::vector<std::shared_ptr<Query>>();
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
    XMLString::release(&TAG_info);


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

      // // Configure DOM parser.
      // if (m_FileParser->loadGrammar("./bank.xsd", Grammar::SchemaGrammarType) == NULL){
      //   fprintf(stderr, "couldn't load schema\n");
      //   return;
      // }

      // m_FileParser->setErrorHandler(&parserErrorHandler);
      m_FileParser->setValidationScheme(XercesDOMParser::Val_Never);
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
        // if (m_FileParser->getErrorCount() != 0){
        //   cout << "XML file does not conform to schema: " << m_FileParser->getErrorCount() << " errors found." << endl;
        //   return;
        // }
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
          // cout << "current node is " << XMLString::transcode(currentNode->getNodeName()) << endl;
          if (isElem(currentNode)) {
            Parse::parseElemNode(currentNode);
          }
          // cout << "------------------------" << endl;
        }
        // DEBUG PRINTS
        cout << "Done! Printing final creates vector:" << endl;
        for (auto i : creates){
          cout << "account: " << i.account << ", balance: " << i.balance << ", ref: " << i.ref << endl;
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
        Create create = {};
        create.error = false;
        create.accountSet = false;
        create.balance = 0;
        create.account = 0;
        const XMLCh* ref = currentElement->getAttribute(ATTR_ref);
        if (!XMLString::equals(ref, emptyRef)){
          char * refStr = XMLString::transcode(ref);
          std::string s(refStr);
          create.ref = s;
        }
        for (XMLSize_t i = 0; i < count; ++i) {
          parseCreateElemNode(children->item(i), create);
        }
        if (!create.accountSet){
          create.error = true;
        }
        creates.push_back(create);
      }

      else if (XMLString::equals(currentElement->getTagName(), TAG_balance)){
        const XMLCh* ref = currentElement->getAttribute(ATTR_ref);
        balanceRef = XMLString::transcode(ref);
        for (XMLSize_t i = 0; i < count; ++i) {
          parseBalanceElemNode(children->item(i));
        }
      }

      else if (XMLString::equals(currentElement->getTagName(), TAG_transfer)) {
        currentTransfer = {}; //empty struct
        const XMLCh* ref = currentElement->getAttribute(ATTR_ref);
        transferRef = XMLString::transcode(ref);
        currentTransfer.ref=transferRef;
        if (isElem(node)) {
          for (XMLSize_t i = 0; i < count; ++i) {
            parseTransferElemNode(children->item(i));
          }
          transfers.push_back(currentTransfer);
          currentTransfer = {}; //empty struct
        }
      }

      else if (XMLString::equals(currentElement->getTagName(), TAG_query)) {
        // cout << "parsing query" << endl;
        std::shared_ptr<Query> queryPtr(new Query()); //empty struct
        const XMLCh* ref = currentElement->getAttribute(ATTR_ref);
        queryPtr->ref=XMLString::transcode(ref);
        if (isElem(node)) {
          for (XMLSize_t i = 0; i < count; ++i) {
            parseQueryElemNode(children->item(i), queryPtr);
          }
          queries.push_back(queryPtr);
          // cout << "done with query" << endl;
          // cout << Parse::translateQuery(queryPtr) << endl;
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

    void Parse::parseCreateElemNode(DOMNode *node, Parse::Create &create){
      if (isElem(node)){
        DOMElement *currentElement = dynamic_cast<xercesc::DOMElement *>(node);
        if (XMLString::equals(currentElement->getTagName(),TAG_account)) {
          const XMLCh* accountNumber = parseLeafElem(node);
          char* accountNumberStr = XMLString::transcode(accountNumber);
          create.account = std::stoll(accountNumberStr);
          create.accountSet = true;
          XMLString::release(&accountNumberStr);
        }
        else if (XMLString::equals(currentElement->getTagName(),TAG_balance)){
          const XMLCh* balance = parseLeafElem(node);
          char* balanceStr = XMLString::transcode(balance);
          create.balance = std::stod(balanceStr);
          XMLString::release(&balanceStr);
        } else{
          cout << "invalid tag" << endl;
        }
      }
    }


    void Parse::parseTransferElemNode(DOMNode *node){
      // Found node which is an Element. Re-cast node as element
      if (isElem(node)){
        DOMElement *currentElement = dynamic_cast<xercesc::DOMElement *>(node);

        if (XMLString::equals(currentElement->getTagName(), TAG_amount)) {
          const XMLCh* transferAmount = parseLeafElem(node);
          char* transferAmountStr = XMLString::transcode(transferAmount);
          currentTransfer.amount = stod(transferAmountStr);
          XMLString::release(&transferAmountStr);
        }
        else if (XMLString::equals(currentElement->getTagName(), TAG_from)) {
          const XMLCh* from = parseLeafElem(node);
          char* fromStr = XMLString::transcode(from);
          currentTransfer.from = stoll(fromStr);
          XMLString::release(&fromStr);
        }
        else if (XMLString::equals(currentElement->getTagName(), TAG_to)) {
          const XMLCh* to = parseLeafElem(node);
          char* toStr = XMLString::transcode(to);
          currentTransfer.to = stoll(toStr);
          XMLString::release(&toStr);
        }
        else if (XMLString::equals(currentElement->getTagName(), TAG_tag)) {
          const XMLCh* tag = parseLeafElem(node);
          char* tagStr = XMLString::transcode(tag);
          currentTransfer.tags.push_back(tagStr);
          XMLString::release(&tagStr);
        }
        else {
        }
      }
      else {
      }
    }

    // fn to construct query strings
    void Parse::parseQueryRelop(DOMNode *node, shared_ptr<Query> q, std::string op){
      DOMElement *currentElement = dynamic_cast<xercesc::DOMElement *>(node);
      char *fromStr = XMLString::transcode(currentElement->getAttribute(TAG_from));
      char *toStr = XMLString::transcode(currentElement->getAttribute(TAG_to));
      char *amtStr = XMLString::transcode(currentElement->getAttribute(TAG_amount));
      std::string fstr(fromStr);
      std::string tstr(toStr);
      std::string astr(amtStr);
      // XMLString::release(&fromStr);
      // XMLString::release(&toStr);
      // XMLString::release(&amtStr);
      std::string empty = "";
      if (fstr != empty){
        std::string s = "(origin " + op + " " + fstr + ") ";
        q->query = s;
      }

      else if (tstr != empty){
        std::string s = "(destination " + op + " " + tstr + ") ";
        q->query = s;
      }

      else if (astr != empty){
        std::string s = "(amount " + op + " " + amtStr + ") ";
        q->query = s;
      }
      else{
        std::string s = "";
        q->query = s;
      }
    }

    bool isParent(DOMNode *node, std::vector<XMLCh*> pp){
      for (auto p : pp){
        if (!XMLString::equals(dynamic_cast<xercesc::DOMElement *>(node->getParentNode())->getTagName(), p)){
          return true;
        }
      }
      return false;
    }

    bool isTag(DOMElement *elem, std::vector<XMLCh*> tt){
      for (auto t : tt){
        if (XMLString::equals(elem->getTagName(), t)) {
          return true;
        }
      }
      return false;
    }


    void Parse::parseQueryElemNode(DOMNode *node, shared_ptr<Parse::Query> queryPtr) {
      if (isElem(node)){

        DOMElement *currentElement = dynamic_cast<xercesc::DOMElement *>(node);
        DOMNodeList *children = node->getChildNodes();
        const XMLSize_t count = children->getLength();
        std::vector<XMLCh*> l = {TAG_greater, TAG_equals, TAG_less};
        std::vector<XMLCh*> ll = {TAG_and, TAG_or, TAG_not};
        // RELOPS
        if (isTag(currentElement, l)) {
          std::string op = "";
          if (XMLString::equals(currentElement->getTagName(), TAG_equals)){
            op = "=";
          }
          else if (XMLString::equals(currentElement->getTagName(), TAG_greater)) {
            op = ">";
          }
          else{
            op = "<";
          }

          if (!isParent(node, ll)){
            std::shared_ptr<Query> newQueryPtr(new Query());
            newQueryPtr->ready = true;
            parseQueryRelop(node, newQueryPtr, op);
            queryPtr->andQueries.push_back(newQueryPtr);
          }
          else {
            if (!queryPtr->ready){
              queryPtr->ready = true;
            }

            parseQueryRelop(node, queryPtr, op);
          }
        }

        else if (XMLString::equals(currentElement->getTagName(), TAG_tag)) {
          char *tag = XMLString::transcode(currentElement->getAttribute(TAG_info));
          if (!isParent(node, ll)){
            std::shared_ptr<Query> newQueryPtr(new Query());
            newQueryPtr->tags.push_back(tag);
            queryPtr->andQueries.push_back(newQueryPtr);
          }
          else {
            queryPtr->tags.push_back(tag);
          }
          // XMLString::release(&tagStr);
        }

        else if (XMLString::equals(currentElement->getTagName(), TAG_and)) {
          std::shared_ptr<Query> newQueryPtr(new Query()); //empty struct
          for (XMLSize_t i = 0; i < count; ++i) {
            parseQueryElemNode(children->item(i), newQueryPtr);
          }
          queryPtr->andQueries.push_back(std::move(newQueryPtr));
        }

        else if (XMLString::equals(currentElement->getTagName(), TAG_or)) {
          std::shared_ptr<Query> newQueryPtr(new Query()); //empty struct
          for (XMLSize_t i = 0; i < count; ++i) {
            parseQueryElemNode(children->item(i), newQueryPtr);
          }

          queryPtr->orQueries.push_back(std::move(newQueryPtr));
          auto test = queryPtr->orQueries[0];
        }
        else if (XMLString::equals(currentElement->getTagName(), TAG_not)) {
          std::shared_ptr<Query> newQueryPtr(new Query()); //empty struct
          for (XMLSize_t i = 0; i < count; ++i) {
            parseQueryElemNode(children->item(i), newQueryPtr);
          }
          queryPtr->notQueries.push_back(std::move(newQueryPtr));

        }
      }
    }

    const XMLCh* Parse::parseLeafElem(DOMNode *node){
      DOMNodeList *children = node->getChildNodes();
      const XMLSize_t count = children->getLength();
      for (XMLSize_t i = 0; i < count; ++i) {
        if(isText(children->item(i))){
          DOMText *currentTextData = dynamic_cast<xercesc::DOMText *>(children->item(i));
          const XMLCh* data = currentTextData->getWholeText(); // TODO need to free XMLCh from transcode
          return data;
        }
      }
      return XMLString::transcode("NO NODE INFO IN XML");
      // DOMElement *elem = dynamic_cast<xercesc::DOMElement *>(node);
    }

    std::string Parse::translateQuery(shared_ptr<Parse::Query> queryPtr){
      std::string res = "SELECT * FROM transfers WHERE ";
      // auto tsize = queryPtr->tags.size();
      // for (auto i=0; i<tsize; i++){
      //   if (i!=0){
      //     res += "AND ";
      //   }
      //   res += "(tags = " + queryPtr->tags[i] + ") ";
      // }

      // if (queryPtr->ready) {
      //   if (tsize > 0){
      //     res += "AND ";
      //   }
      //   res += queryPtr->query;
      // }

      auto andSize = queryPtr->andQueries.size();
      auto orSize = queryPtr->orQueries.size();
      auto notSize = queryPtr->notQueries.size();
      if (andSize>0){
        string s = "";
        res += "(";
        res += translateQueryInner(queryPtr->andQueries, s, "AND");
        res += ") ";
      }
      if (orSize>0){
        string s = "";
        res += "OR (";
        res += translateQueryInner(queryPtr->orQueries, s, "OR");
        res += ") ";

      }
      if (notSize>0){
        string s = "";
        res += "NOT (";
        res += translateQueryInner(queryPtr->notQueries, s, "NOT");
        res += ") ";

      }
      return res;
    }

    std::string Parse::translateQueryInner(std::vector<shared_ptr<Parse::Query>> queries, std::string res, std::string op){
      auto size = queries.size();
      if (size == 0) {
        return "";
      }
      res += "(";
      for (uint32_t i = 0; i<queries.size(); i++){
        uint32_t andSize = queries[i]->andQueries.size();
        uint32_t orSize = queries[i]->orQueries.size();
        uint32_t notSize = queries[i]->notQueries.size();
        if (!queries[i]->ready){

          // recurse!
          if (andSize != 0) {
            res += translateQueryInner(queries[i]->andQueries, "", "AND");
          }
          if (orSize != 0 && andSize != 0){
            res += "OR";
            res += translateQueryInner(queries[1]->orQueries, "", "OR");
          }
          else if (orSize != 0){
            res += translateQueryInner(queries[1]->orQueries, "", "OR");
          }
          if (notSize != 0 && (orSize != 0 || andSize != 0)){
            res += "AND (NOT";
            res += translateQueryInner(queries[1]->notQueries, "", "NOT");
            res += ")";
          }
          else if (notSize != 0){
            res += "NOT";
            res += translateQueryInner(queries[1]->notQueries, "", "NOT");
            res += "";
          }
        }

        if (queries[i]->tags.size()>0) {
          auto tsize = queries[i]->tags.size();
          if (tsize >0){
            for (uint32_t j = 0; j < tsize; j++){
              if (j==0){
                res += "( tag @> ARRAY['" + queries[i]->tags[j]+ "']";
              }
              else {
                res += " " + op + " tag @> ARRAY['" + queries[i]->tags[j]+ "']";
              }
            }
            res += ")";
          }
        }
        else {
          res += queries[i]->query;
        }
        if (i != queries.size()-1){
          res += op + " ";
        }
      }
      res += ")";
      return res;
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
