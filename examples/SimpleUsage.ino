#include <Arduino.h>
#include "StreamCommandParser.h"

qCommand qC(Serial, "streamCommandParser");

void hi_handler(StreamCommandParser& commandParser) {
    commandParser.preferredResponseStream.println("Whazzup?");
}

void sum_handler(StreamCommandParser& commandParser) {
    int arg1 = atoi(commandParser.next());
    int arg2 = atoi(commandParser.next());

    commandParser.preferredResponseStream.println(arg1 + arg2);
}

void default_handler(const char * command, StreamCommandParser& commandParser) {
    commandParser.preferredResponseStream.println("Unrecognized Command");
}

void setup() {
    Serial.begin(9600);
    qC.addCommand("hi", hi_handler);
    qC.addCommand("sum", sum_handler);
    qC.setDefaultHandler(default_handler);
    Serial.println("ready...");
}

void loop() {
    qC.readSerial(Serial);
}
