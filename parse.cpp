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
  TAG_transfer = XMLString::transcode("transfer");
  TAG_balance = XMLString::transcode("balance");
  TAG_query = XMLString::transcode("query");
  TAG_account = XMLString::transcode("account");
  m_FileParser = new XercesDOMParser;
  createRequests = std::vector<std::pair<long long, double>>();
  requestTuple = std::pair<long long, double>();
  accSet = false;
  balSet = false;
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


void Parse::readFile(string &configFile) throw(std::runtime_error) {
  // Test to see if the file is ok.

  struct stat fileStatus;

  errno = 0;
  if (stat(configFile.c_str(), &fileStatus) == -1) // ==0 ok; ==-1 error
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

  m_FileParser->setValidationScheme(XercesDOMParser::Val_Never);
  m_FileParser->setDoNamespaces(false);
  m_FileParser->setDoSchema(false);
  m_FileParser->setLoadExternalDTD(false);
  // auto m_OptionA = 1;
  // auto m_OptionB = 1;

  try {
    m_FileParser->parse(configFile.c_str());

    // no need to free this pointer - owned by the parent parser object
    DOMDocument *xmlDoc = m_FileParser->getDocument();

    // Get the top-level element: Name is "transactions". No attributes for
    // "root"
    DOMElement *elementRoot = xmlDoc->getDocumentElement();
    if (!elementRoot)
      throw(std::runtime_error("empty XML document"));

    DOMNodeList *children = elementRoot->getChildNodes();
    const XMLSize_t nodeCount = children->getLength();

    cout << "nodecount is " << to_string(nodeCount) << endl;
    // For all nodes, children of "root" in the XML tree.
    for (XMLSize_t i = 0; i < nodeCount; ++i) {
      DOMNode *currentNode = children->item(i);
      cout << "currentNode is " << XMLString::transcode(currentNode->getNodeName()) << endl;
      if (isElem(currentNode)) {
        Parse::parseElemNode(currentNode);
      }
      cout << "------------------------" << endl;
    }
    cout << "Done! Printing final createRequests vector:" << endl;
    for (auto i : createRequests){
      cout << "(" << i.first << "," << i.second << ")" << endl;
    }
  }

  catch (xercesc::XMLException &e) {
    char *message = xercesc::XMLString::transcode(e.getMessage());
    ostringstream errBuf;
    errBuf << "Error parsing file: " << message << flush;
    XMLString::release(&message);
  }
}
void Parse::parseTextNode(DOMNode *currentNode) {
  cout << "Text detected in currentNode:" << endl;
  DOMText *currentTextData =
      dynamic_cast<xercesc::DOMText *>(currentNode);
  cout << XMLString::transcode(currentTextData->getWholeText()) << endl;
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
  cout << "current Element is " << XMLString::transcode(currentElement->getTagName()) << endl;
  if (XMLString::equals(currentElement->getTagName(), TAG_create)) {
    cout << "TAG_create found" << endl;
    DOMNodeList *createChildren = node->getChildNodes();
    const XMLSize_t createNodeCount = createChildren->getLength();
    for (XMLSize_t j = 0; j < createNodeCount; ++j) {
      DOMNode *currentSubnode = createChildren->item(j);
      if (isElem(currentSubnode)) // is element
      {
        DOMElement *currentsubElement = dynamic_cast<xercesc::DOMElement *>(currentSubnode);
        if (XMLString::equals(currentsubElement->getTagName(),TAG_account)) {
          cout << "TAG_account found" << endl;
          // TODO
          const XMLCh* accountNumber = parseLeafElem(currentSubnode);
          static char* accountNumberStr = XMLString::transcode(accountNumber);

          if (accSet == true){
            cout << "ERROR, accountNumber was previously set." << endl;
            balSet = false;
            accSet = false;

          }
          else {
            requestTuple = std::make_pair(std::stoll(accountNumberStr), requestTuple.second);
            accSet = true;
            if (balSet && accSet) {
              cout << "requestTuple is " << requestTuple.first << " " << requestTuple.second << endl;
              createRequests.push_back(requestTuple);
              balSet = false;
              accSet = false;
            }
          }
          // XMLString::release(&accountNumberStr);
        }
        else if (XMLString::equals(currentsubElement->getTagName(), TAG_balance)) {
          cout << "TAG_balance found" << endl;
          // TODO
          const XMLCh* balance = parseLeafElem(currentSubnode);
          static char* balanceStr = XMLString::transcode(balance);
          if (balSet == true){
            cout << "ERROR, balance was previously set." << endl;
            balSet = false;
            accSet = false;
          }
          else {
            requestTuple = std::make_pair(requestTuple.first, std::stod(balanceStr));
            balSet = true;
            if (balSet && accSet) {
              cout << "requestTuple is " << requestTuple.first << " " << requestTuple.second << endl;
              createRequests.push_back(requestTuple);
              balSet = false;
              accSet = false;
            }
          }
          // XMLString::release(&balanceStr);
        }
      }
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
      // cout << XMLString::transcode(data) << endl;
      return data;
    }
  }
  // DOMElement *elem = dynamic_cast<xercesc::DOMElement *>(node);
}


#ifdef MAIN_TEST
/* This main is provided for unit test of the class. */

int main() {
  string configFile = "./testfiles/primer.xml"; // file to parse. Get ambigious
                                                // segfault otherwise.
  Parse parser;
  parser.readFile(configFile);
  return 0;
}
#endif
