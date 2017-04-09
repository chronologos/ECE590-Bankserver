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