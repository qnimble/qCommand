#include "qCommand.h"

qCommand qC;
double loopGain = 1.021; // global for the loop gain

void setup() {
  qC.setDefaultHandler(UnknownCommand);
  //qC.setCaseSensitive(true); //uncomment to make commands case-sensitive
  qC.addCommand("Hello", hello);
  qC.addCommand("Hi", hello);
  qC.addCommand("Mult",multiply);
  qC.addCommand("Gain", gain);
  qC.addCommand("Help", help);
}

void loop() {
  qC.readSerial(Serial);
  qC.readSerial(Serial2);
}

void hello(qCommand& qC, Stream& S) {
  if ( qC.next() == NULL) {
    S.println("Hello.");
  } else {
    S.printf("Hello %s, it is nice to meet you.\n",qC.current());
  }
}

void gain(qCommand& qC, Stream& S) {
  if ( qC.next() != NULL) {
    loopGain = atof(qC.current());
  }
  S.printf("The gain is %e\n",loopGain); //print gain in scientific notation
}

void multiply(qCommand& qC, Stream& S) {
  double a;
  int b;
  if ( qC.next() == NULL) {
    S.println("The multiply command needs two arguments, none given.");
    return;
  } else {
    a = atof(qC.current());
  }
  if ( qC.next() == NULL) {
    S.println("The multiply command needs two arguments, only one given.");
    return;
  } else {
    b = atoi(qC.current());
  }
  S.printf("%e times %d is %e\n",a,b,a*b);
}

void help(qCommand& qC, Stream& S) {
  S.println("Available commands are:");
  qC.printAvailableCommands(S);
}

void UnknownCommand(const char* command, qCommand& qC, Stream& S) {
  S.printf("I'm sorry, I didn't understand that. (You said '%s'?)\n",command);
  S.println("You can type 'help' for a list of commands");
}
