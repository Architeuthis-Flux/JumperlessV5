#ifndef MENUTREE_H

#define MENUTREE_H
#include <Arduino.h>




String menuLines[] = {
  "$Rails$",
  "-*Both* *Top* *Bottom*",
  "-->v1",


  "Output",
  "-Limits",
  "--$Min Max$",
  "---*0 _ 3V3**0 _ +5V** ~ 5V** ~ 8V*",
  "-$GPIO$",
  "--*1**2**3**4**5**6**7**8*",
  "--->n1",
  "-$UART$",
  "--*Tx* *Rx*",
  "---Nodes>n2",
  //   "----*USB 2*  *Print*",
  //   "-----*9600* *115200*",
    // "-$Buffer$",
    // "--*In* *Out*",
    // "--->n2",
    // "--DigitalOptions",
    // "---Output",
    // "----*USB 2*  *Print*",
    // "---$UART$",
    // "----$Baud$",
    // "-----*9600**19200**57600**115200*",
    // "---$I2C$",
    // "----$Speed$",
    // "-----*100 K**400 K**1   M**3.4 M*",
    "-$Voltage$",
    "--$DAC$",
    "--*DAC 0**DAC 1**Top R '**Bot R ,*",
    "--->v1",
    "---->n1",

    // "--$*Min**Max*$",
    // "--->v2",


    "Show",
    "-$Digital$",
    "--$GPIO$",
    "---*1**2**3**4**5**6**7**8*",
    "---->n1",
    "-$UART$",
    "---*Tx* *Rx*",
    "---->n2",
    "-$I2C$",
    "---*SDA* *SCL*",
    "---->n2",
    "-$Current$",
    "--*Pos* *Neg*",
    "--->n2",
    "-$Voltage$",
    "--*0**1**2**3**4*",
    "--->n1",
    //   "-$Options$",
    //   "--Analog Display",
    //   "---$Type$",
 
 
 

  "Apps",
  "-Bounce Startup",
  "-Calib  DACs",
  "-Custom App",
  // "-XLSX   GUI",
  //"-Micropython",
  "-uPythonREPL",
  "-File   Manager",
  "-Probe  Calib",
  "-JDI MIPdisplay",

  //"-Show   Image",
 

 //   "-Oscill oscope",
 //   "-MIDI   Synth", 
 //   "-I2C    Scanner",
 //   "-Self   Dstruct",
 //   "-EEPROM Dumper",
 //   "-7 Seg  Mapper",
 //   "-Rick   Roll",
 //   "-$Circuts>$",
 //   "--555",
 //   "--Op Amp",
 //   "--$7400$",
 //   "---*74x109**74x161**74x42**74x595*",
  //  "-$Games  >$",
  //  "--*DOOM*", //*Pong**Tetris**Snake*",
   //"-$Manage >$",
 //   "--Delete",
 //   "--->a3",
 //   "--Upload",
 //   "--->a4",
 //   "-Logic  Analyzr",

   "-Scan",
   "-I2C    Scan",

   "Slots",
   "-$Load$",
   "--*0**1**2**3**4**5**6**7*>s",
   "-$Clear$",
   "--*0**1**2**3**4**5**6**7*>s",
   "-$Save to$",
   "--*0**1**2**3**4**5**6**7*>s",








       "DisplayOptions",
       "-$Demo$",
       "--*On**Off*",
       "-$Colors$",
       "--*Rainbow**Shuffle*",
       "-$Jumpers$",
       "--*Wires* *Lines*",
       "-$Bright$",
       "--$Menu$",
       "---*1**2**3**4**5**6**7**8*",
       "--$Special$",
       "---*1**2**3**4**5**6**7**8*",
       "--$Rails$",
       "---*1**2**3**4**5**6**7**8*",
       "--$Wires$",
       "---*1**2**3**4**5**6**7**8*",



       "OLED",
       "-Connect",
       //"-Dis    connect",
       "-ConnectOn Boot",
       "--*On**Off*",
       "-Lock   Connect",
       "--*On**Off*",
       "-Font",
       "--Eurostl^",
       "--Jokermn^",
       "--ComicSns^",
       "--Courier^",
       "--Science^",
       "--SciExt^",
       "--AndlMno^",
       "--FreMno^",
       "--Iosevka^",
       
      //  "--IosevkaLt^",
      //  "--IosevkaRg^",
      //  "--IosevkaMd^",
      //  "--IosevkaBd^",
       "-Show in Term",
      //  "-Demo",


       "RoutingOptions",
       "-Stack",
       "--$Rails$",
       "---*0**1**2**3**4**Max *",
       "--$DACs$",
       "---*0**1**2**3**4**Max *",
       "--$Paths$",
       "---*0**1**2**3**4**Max *",
       "end"
  };


/*



char menuTree[] = {"\n\
$Rails$\n\
\n\
-*Both* *Top* *Bottom*\n\
-->v1\n\
\n\
Apps\n\
\n\
-Oscill oscope\n\
-MIDI   Synth\n\
-I2C    Scanner\n\
-Self   Dstruct\n\
-EEPROM Dumper\n\
-7 Seg  Mapper\n\
-Rick   Roll\n\
-$Circuts>$\n\
--555\n\
--Op Amp\n\
--$7400$\n\
---*74x109**74x161**74x42**74x595*\n\
-$Games  >$\n\
--*DOOM**Pong**Tetris**Snake*\n\
-$Manage >$\n\
--Delete\n\
--->a3\n\
--Upload\n\
--->a4\n\
-Logic  Analyzr\n\
Slots\n\
\n\
-$Load$\n\
--*0**1**2**3**4**5**6**7*>s\n\
\n\
-$Clear$\n\
--*0**1**2**3**4**5**6**7*>s\n\
\n\
-$Save to$\n\
--*0**1**2**3**4**5**6**7*>s\n\
\n\
Show\n\
-$Digital$\n\
\n\
--$GPIO$\n\
---*5V* *3.3V*\n\
----*0* *1* *2* *3*\n\
----->n4\n\
\n\
--$UART$\n\
---*Tx* *Rx*\n\
---->n2\n\
\n\
--$I2C$\n\
---*SDA* *SCL*\n\
---->n2\n\
\n\
-$Current$\n\
--*Pos* *Neg*\n\
--->n2\n\
\n\
-Options\n\
\n\
--Analog Display\n\
---$Type$\n\
----*Mid Out**Bot Up**Bright**Color* \n\
-----$Range$\n\
------>r\n\
---$Range$\n\
---->r\n\
\n\
--DigitalOptions\n\
---Output\n\
----*USB 2*  *Print*\n\
---$UART$\n\
----$Baud$\n\
-----*9600**19200**57600**115200*\n\
---$I2C$\n\
----$Speed$\n\
-----*100 K**400 K** 1  M**3.4 M*\n\
\n\
-$Voltage$\n\
--*0* *1* *2*\n\
--->n3\n\
\n\
Output\n\
\n\
-$GPIO$\n\
--*5V* *3.3V*\n\
---*0* *1* *2* *3*\n\
---->n4\n\
-$UART$\n\
--*Tx* *Rx*\n\
---Nodes>n2\n\
----*USB 2*  *Print*\n\
-----*9600* *115200*\n\
\n\
-$Buffer$\n\
--*In* *Out*\n\
--->n2\n\
\n\
--DigitalOptions\n\
---Output\n\
----*USB 2*  *Print*\n\
---$UART$\n\
----$Baud$\n\
-----*9600**19200**57600**115200*\n\
---$I2C$\n\
----$Speed$\n\
-----*100 K**400 K**1   M**3.4 M*\n\
\n\
-$Voltage$\n\
--$Range$\n\
--*5V* *~8V*\n\
--->v2\n\
---->n1\n\
\n\
DisplayOptions\n\
-$DEFCON$\n\
--*On**Off**Fuck*\n\
-$Colors$\n\
--*Rainbow**Shuffle*\n\
-$Jumpers$\n\
--*Wires* *Lines*\n\
-$Bright$\n\
--*1**2**3**4**5**6**7**8*\n\0"};

*/

#endif