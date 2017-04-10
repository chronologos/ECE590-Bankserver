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

  creates = std::vector<std::tuple<long long, double, std::string>>();
  requestTuple = std::tuple<long long, double, std::string>();
  // requestTuple holds the current create request as a
  // (account no * balance) tuple.
  accSet = false;
  balSet = false;
  reset = false;
  balanceRef = "";
  balances = std::vector<std::tuple<long long, std::string>>();

  struct Transfer currentTransfer = {};

  transfers = std::vector<Transfer>();
  queries = std::vector<Query>();
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
        if (true){
          return;
        }
        cout << "parsing query" << endl;
        Query query = {}; //empty struct
        query.ready = false;
        query.leaf = NULL;
        const XMLCh* ref = currentElement->getAttribute(ATTR_ref);
        query.ref=XMLString::transcode(ref);
        if (isElem(node)) {
          for (XMLSize_t i = 0; i < count; ++i) {
            parseQueryElemNode(children->item(i), query);
          }
          queries.push_back(query);
          cout << "done with query" << endl;
          cout << Parse::translateQuery(query) << endl;
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

    // fn to construct query strings
    void Parse::parseQueryRelop(DOMNode *node, LeafQuery *lq, std::string op){
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
        cout << "in parseQueryRelopA" << endl;

        lq->query = "(origin " + op + " " + fstr + ") ";
        // lq.ready = true;
      }
      else if (tstr != empty){
        cout << "in parseQueryRelopB" << endl;

        lq->query = "(destination " + op + " " + tstr + ") ";
        // lq.ready = true;
        printf("lq pointer %p\n",lq);
      }
      else if (astr != empty){
        cout << "in parseQueryRelopC" << endl;

        lq->query = "(amount " + op + " " + amtStr + ") ";
        // lq.ready = true;
      }
    }

    void Parse::parseQueryElemNode(DOMNode *node, Query &query) {
      if (isElem(node)){

        DOMElement *currentElement = dynamic_cast<xercesc::DOMElement *>(node);
        DOMNodeList *children = node->getChildNodes();
        const XMLSize_t count = children->getLength();


        // RELOPS
        if (XMLString::equals(currentElement->getTagName(), TAG_greater)) {
          if (!query.ready){
            LeafQuery *lq = new LeafQuery();
            query.leaf = lq;
            lq->query = "";
            query.ready = true;
          }
          parseQueryRelop(node, query.leaf, ">");
          cout << "in parseQueryElemNode4a" << endl;
        }
        else if (XMLString::equals(currentElement->getTagName(), TAG_equals)) {
          if (!query.ready){
            query.leaf = new LeafQuery();
            query.leaf->query = "";
            query.ready = true;
            printf("or pointer in parse is %p\n",query);

          }
          printf("query.leaf->query pointer %p\n",query.leaf->query);

          parseQueryRelop(node, query.leaf, "=");
          cout << query.leaf->query << endl;
          cout << "in parseQueryElemNode4b" << endl;
        }
        else if (XMLString::equals(currentElement->getTagName(), TAG_less)) {
          if (!query.ready){
            LeafQuery *lq = new LeafQuery();
            lq->query = "";
            query.leaf = lq;
            query.ready = true;


          }
          parseQueryRelop(node, query.leaf, "<");
          cout << "in parseQueryElemNode4c" << endl;
        }

        else if (XMLString::equals(currentElement->getTagName(), TAG_tag)) {
          char *tag = XMLString::transcode(currentElement->getAttribute(TAG_info));
          query.tags.push_back(tag);
          cout << "in parseQueryElemNode4d" << endl;

          // XMLString::release(&tagStr);
        }

        else if (XMLString::equals(currentElement->getTagName(), TAG_and)) {
          Query newQuery = {}; //empty struct
          newQuery.ready = false;
          query.andQueries.push_back(std::ref(newQuery));

          cout << "in and" << endl;
          for (XMLSize_t i = 0; i < count; ++i) {
            parseQueryElemNode(children->item(i), newQuery);
          }

        }
        else if (XMLString::equals(currentElement->getTagName(), TAG_or)) {
          Query newQuery = {}; //empty struct
          newQuery.ready = false;

          query.orQueries.push_back(std::ref(newQuery));
          cout << "in or" << endl;
          printf("or pointer is %p",newQuery);
          for (XMLSize_t i = 0; i < count; ++i) {
            parseQueryElemNode(children->item(i), newQuery);
          }

        }
        else if (XMLString::equals(currentElement->getTagName(), TAG_not)) {
          Query newQuery = {}; //empty struct
          newQuery.ready = false;
          query.notQueries.push_back(std::ref(newQuery));
          cout << "in not" << endl;

          for (XMLSize_t i = 0; i < count; ++i) {
            parseQueryElemNode(children->item(i), newQuery);
          }

        }
        // query.leaf->query += ")";
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
      return XMLString::transcode("NO NODE INFO IN XML");
      // DOMElement *elem = dynamic_cast<xercesc::DOMElement *>(node);
    }

    std::string Parse::translateQuery(Parse::Query &q){
      cout << "in translateQuery" << endl;
      std::string res = "SELECT * FROM transfers WHERE ";
      auto tsize = q.tags.size();
      for (auto i=0; i<tsize; i++){
        if (i!=0){
          res += "AND ";
        }
        res += "(tags = " + q.tags[i] + ") ";
      }

      if (q.ready) {
        if (tsize > 0){
          res += "AND ";
        }
        res += q.leaf->query;
      }
      cout << "ok" << endl;

      auto andSize = q.andQueries.size();
      auto orSize = q.orQueries.size();
      auto notSize = q.notQueries.size();
      if (andSize>0){
        string s = "";
        res += "AND (";
        res += translateQueryInner(q.andQueries, s, "AND");
        res += ") ";
      }
      if (orSize>0){
        string s = "";
        res += "OR (";
        res += translateQueryInner(q.orQueries, s, "OR");
        res += ") ";

      }
      if (notSize>0){
        string s = "";
        res += "NOT (";
        res += translateQueryInner(q.notQueries, s, "NOT");
        res += ") ";

      }
      return res;
    }

    std::string Parse::translateQueryInner(std::vector<std::reference_wrapper<Parse::Query>> qq, std::string res, std::string op){
      // auto andSize = q.andQueries.size();
      // auto orSize = q.orQueries.size();
      cout << qq.size() << endl;
      cout << op << endl;
      for (auto i = 0; i<qq.size(); i++){
        printf("or pointer in translate is %p\n",qq[i].get());

        if (qq[i].get().ready) {
          cout << "?" << endl;
          printf("leaf pointer in translate is %p",qq[i].get().leaf->query);

          res += (qq[i].get().leaf)->query;

          res += "WUT";
        }
        else {
          cout << "no leaf in this array???" << endl;
        }
        if (i>0){
          res += op;
          res += " ";
        }
        return res;
      }
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
