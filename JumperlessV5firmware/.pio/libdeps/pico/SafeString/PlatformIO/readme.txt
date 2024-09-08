PlatformIO support

This library is primarily an Arduino IDE library, but users have had success using it with PlatformIO.  
See the PlatformIO subdirectory for the two PlatformIO versions.  One for boards that use the arduino namespace, e.g. Arduino Zero, and one for boards that don't, e.g. UNO.
To see which board yours is, check the Print.h file and see if it includes the lines
namespace arduino {  
class Print  
If it does, then unzip the SafeStringIO_namespace.zip and use that library.  If it does not, then unzip the SafeStringIO.zip and use that library.
