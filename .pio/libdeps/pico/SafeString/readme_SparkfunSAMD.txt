See issue https://github.com/PowerBroker2/SafeString/issues/73

Sparkfun's SAMD board support does not provide a define to distinguish it from Arduino's SAMD boards
This causes SafeString compile to fail with
 error: expected class-name before '{' token 73 | class SafeStringReader : public SafeString { |    

This fix is to remove the contents of these three header files
SafeStringNameSpace.h
SafeStringNameSpaceEnd.h
SafeStringNameSpaceStart.h

That is just have empty files for those three headers.