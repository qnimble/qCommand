#include "qCommand.h"

/**
 * Constructor
 */
qCommand::qCommand()
  :
    commandList(NULL),
    commandCount(0),
    defaultHandler(NULL),
    term('\n'),           // default terminator for commands, newline character
    caseSensitive(false),
	cur(NULL),
	last(NULL)
{
  strcpy(delim, " "); // strtok_r needs a null-terminated string
  clearBuffer();
}

/**
 * Adds a "command" and a handler function to the list of available commands.
 * This is used for matching a found token in the buffer, and gives the pointer
 * to the handler function to deal with it.
 */
void qCommand::addCommand(const char *command, void (*function)(qCommand& streamCommandParser, Stream& stream)) {
  #ifdef SERIALCOMMAND_DEBUG
    Serial.print(" - Adding Command (");
    Serial.print(commandCount);
    Serial.print("): ");
    Serial.println(command);
  #endif

  commandList = (StreamCommandParserCallback *) realloc(commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
  strncpy(commandList[commandCount].command, command, STREAMCOMMAND_MAXCOMMANDLENGTH);
  commandList[commandCount].function.f1 = function;

  if (!caseSensitive) {
    strlwr(commandList[commandCount].command);
  }

  commandCount++;
}

void qCommand::addCommandInternal(const char *command, void (qCommand::*function)(qCommand& streamCommandParser, Stream& stream, void* variable, const char* command), void* var) {
  #ifdef SERIALCOMMAND_DEBUG
    Serial.print(" - Adding Assign Variable Command (");
    Serial.print(commandCount);
    Serial.print("): ");
    Serial.println(command);
  #endif

	commandList = (StreamCommandParserCallback *) realloc(commandList, (commandCount + 1) * sizeof(StreamCommandParserCallback));
	strncpy(commandList[commandCount].command, command, STREAMCOMMAND_MAXCOMMANDLENGTH);
	commandList[commandCount].function.f2 = function;
	commandList[commandCount].ptr = var;
	if (!caseSensitive) {
	   strlwr(commandList[commandCount].command);
	}
	commandCount++;
}

void qCommand::assignVariable(const char* command, int* variable) {
	addCommandInternal(command,&qCommand::reportInt, variable);
}

void qCommand::assignVariable(const char* command, uint* variable) {
	addCommandInternal(command,&qCommand::reportUInt, variable);
}


void qCommand::assignVariable(const char* command, double* variable) {
	addCommandInternal(command,&qCommand::reportDouble,variable);
}


void qCommand::reportInt(qCommand& qC, Stream& S, void* ptr, const char* command) {
	int* ptrint = (int*) ptr;
	if ( qC.next() != NULL) {
	   	*ptrint = atoi(qC.current());
	}
	S.printf("%s is %d\n",command,*ptrint);
}

void qCommand::reportUInt(qCommand& qC, Stream& S, void* ptr, const char* command) {
	uint* ptrint = (uint*) ptr;
	if ( qC.next() != NULL) {
	   	*ptrint = atoi(qC.current());
	}
	S.printf("%s is %u\n",command,*ptrint);
}




void qCommand::reportDouble(qCommand& qC, Stream& S, void* ptr, const char* command) {
	double* ptrdouble = (double*) ptr;
	if ( qC.next() != NULL) {
	   	*ptrdouble = atof(qC.current());
	}
	if ( ( abs(*ptrdouble) > 10 ) || (abs(*ptrdouble < .1) ) ) {
		S.printf("%s is %e\n",command,*ptrdouble); //print gain in scientific notation
	} else {
		S.printf("%s is %f\n",command,*ptrdouble); //print gain in scientific notation
	}
}





/**
 * This sets up a handler to be called in the event that the receveived command string
 * isn't in the list of commands.
 */
void qCommand::setDefaultHandler(void (*function)(const char *, qCommand& streamCommandParser, Stream& stream)) {
  defaultHandler = function;
}

/**
 * This checks the Serial stream for characters, and assembles them into a buffer.
 * When the terminator character (default '\n') is seen, it starts parsing the
 * buffer for a prefix command, and calls handlers setup by addCommand() member
 */
void qCommand::readSerial(Stream& inputStream) {
  // Serial.print("Time IN: ");
  // Serial.println(millis());
  while (inputStream.available() > 0) {
    char inChar = inputStream.read();   // Read single available character, there may be more waiting
    #ifdef SERIALCOMMAND_DEBUG
      Serial.print(inChar);   // Echo back to serial stream
    #endif

    if (inChar == term) {     // Check for the terminator (default '\n') meaning end of command
      #ifdef SERIALCOMMAND_DEBUG
        Serial.print("Received: ");
        Serial.println(buffer);
      #endif

        if (!caseSensitive) {
		  strlwr(buffer);
        }
      char *command = strtok_r(buffer, delim, &last);   // Search for command at start of buffer
      if (command != NULL) {
        boolean matched = false;
        for (int i = 0; i < commandCount; i++) {
          #ifdef SERIALCOMMAND_DEBUG
            Serial.print(" - Comparing [");
            Serial.print(command);
            Serial.print("] to [");
            Serial.print(commandList[i].command);
            Serial.println("]");
          #endif

          // Compare the found command against the list of known commands for a match
          if (strncmp(command, commandList[i].command, STREAMCOMMAND_MAXCOMMANDLENGTH) == 0) {
            #ifdef SERIALCOMMAND_DEBUG
              Serial.print("Matched Command: ");
              Serial.println(command);
            #endif

            // Execute the stored handler function for the command
              if (commandList[i].ptr == NULL) {
            	  (*commandList[i].function.f1)(*this,inputStream);
              } else {
            	  (this->*commandList[i].function.f2)(*this,inputStream,commandList[i].ptr,commandList[i].command);
              }

            matched = true;
            break;
          }
        }
        if (!matched) {
        	if (defaultHandler != NULL) {
                (*defaultHandler)(command, *this, inputStream);
        	} else {
        		inputStream.print("Unknown command: ");
        		inputStream.println(command);
        	}
        }
      }
      clearBuffer();
    } else if (isprint(inChar)) {     // Only printable characters into the buffer
      if (bufPos < STREAMCOMMAND_BUFFER) {
        buffer[bufPos++] = inChar;  // Put character into buffer
        buffer[bufPos] = '\0';      // Null terminate
      } else {
        #ifdef SERIALCOMMAND_DEBUG
          Serial.println("Line buffer is full - increase STREAMCOMMAND_BUFFER");
        #endif
      }
    }
  }
  // Serial.print("Time OUT: ");
  // Serial.println(millis());
}


/*
 * Print list of all available commands
 */
void qCommand::printAvailableCommands(Stream& outputStream) {
  for (int i = 0; i < commandCount; i++) {
    outputStream.println(commandList[i].command);
  }
}

/*
 * Set whether the commands should be case insensitive
 */
void qCommand::setCaseSensitive(bool Sensitive) {
	caseSensitive = Sensitive;
}

/*
 * Clear the input buffer.
 */
void qCommand::clearBuffer() {
  buffer[0] = '\0';
  bufPos = 0;
}

/**
 * Retrieve the next token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *qCommand::next() {
  cur = strtok_r(NULL, delim, &last);
  return cur;
}

/**
 * Retrieve the current token ("word" or "argument") from the command buffer.
 * Returns NULL if no more tokens exist.
 */
char *qCommand::current() {
  return cur;
}
