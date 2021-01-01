/*
 *void setup() {
 *Serial.begin(9600);
 *Serial.println();

 *printSentenceWithChecksum("$POV,P," + String(1018.35) + "*", false); // 39
 *printSentenceWithChecksum("test$test*", false); // 16
 *printSentenceWithChecksum("$test*", false); // 16
 *printSentenceWithChecksum("$POV,P,951.78*", false); // 05
 *printSentenceWithChecksum("$POV,P,951.84*", false); // 06
 *}
*/

void printSentenceWithChecksum(String sentence, bool printLogs) {
  String sentenceWithChecksum = withChecksum(sentence, printLogs);
  Serial.println(sentenceWithChecksum);
}

String withChecksum(String sentence, bool printLogs) {
  // http://www.hhhh.org/wiml/proj/nmeaxor.html
  bool started = false;
  char checksum = 0;
  for (int index = 0; index < sentence.length(); index++) {
    if (index > 0 && sentence[index - 1] == '$') {
      if (printLogs) Serial.println("Found first checksum char:");
      if (printLogs) Serial.println(sentence[index]);
      if (printLogs) Serial.println(sentence[index], HEX);
      if (printLogs) Serial.println("Set as initial 'last step result'.");
      if (printLogs) Serial.println();
      checksum = sentence[index];
      started = true;
      continue; // Skip the rest of this loop iteration.
    }
    
    if (sentence[index] == '*') {
      if (printLogs) Serial.println("Reached the end of checksum chars.");
      if (printLogs) Serial.println("Final checksum:");
      if (printLogs) Serial.println(checksum, HEX);
      if (printLogs) Serial.println();
      break; // Exit the loop.
    }
    
    // Ignore everything preceeding '$'.
    if (!started) {
      continue; // Skip the rest of this loop iteration.
    }
    
    if (printLogs) Serial.println("Xoring last step result and current char.");
    if (printLogs) Serial.println(checksum, HEX);
    if (printLogs) Serial.println(sentence[index]);
    if (printLogs) Serial.println(sentence[index], HEX);
    
    checksum = checksum xor sentence[index];
    if (printLogs) Serial.println("Got new last step result:");
    if (printLogs) Serial.println(checksum, HEX);
    if (printLogs) Serial.println();
  }
  
  String sentenceWithChecksum = sentence + (checksum < 10 ? "0" : "") + String(checksum, HEX);
  if (printLogs) Serial.println("Sentence with checksum:");
  if (printLogs) Serial.println(sentenceWithChecksum);
  return sentenceWithChecksum;
}
