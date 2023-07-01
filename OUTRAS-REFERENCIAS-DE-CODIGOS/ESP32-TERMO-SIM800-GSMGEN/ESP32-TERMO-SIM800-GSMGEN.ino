
void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  delay(3000);
  test_sim800_module();
  send_SMS();
}

void loop() {
  //////////////////////////////////////////////////
  while (sim800.available()) {
    parseData(sim800.updateSerial());
  }
  //////////////////////////////////////////////////
  while (Serial.available()) {
    sim800.println(Serial.updateSerial());
  }
  //////////////////////////////////////////////////
}  //main loop ends

void test_sim800_module() {
  Serial2.println("AT\r");
  updateSerial();
  Serial.println();
  Serial2.println("AT+CSQ\r");
  updateSerial();
  Serial2.println("AT+CCID\r");
  updateSerial();
  Serial2.println("AT+CREG?\r");
  updateSerial();
  Serial2.println("ATI\r");
  updateSerial();
  Serial2.println("AT+CBC\r");
  updateSerial();
}

void updateSerial() {
  delay(500);
  while (Serial.available()) {
    Serial2.write(Serial.read());  //Forward what Serial received to Software Serial Port
  }
  while (Serial2.available()) {
    Serial.write(Serial2.read());  //Forward what Software Serial received to Serial Port
  }
}

void send_SMS() {
  Serial2.println("AT+CMGF=1\r");  // Configuring TEXT mode
  updateSerial();
  Serial2.println("AT+CMGS=\"+5555996965230\"\r");  //change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();
  Serial2.print("Circuit Digest");  //text content
  updateSerial();
  Serial.println();
  Serial.println("Message Sent");
  Serial2.write(26);
}

/*
//sender phone number with country code
const String PHONE = "+5555996965230";

//GSM Module RX pin to ESP32 Pin 2
//GSM Module TX pin to ESP32 Pin 4
#define rxPin 4
#define txPin 2
#define BAUD_RATE 9600
HardwareSerial sim800(1);

#define RELAY_1 17
#define RELAY_2 15

String smsStatus, senderNumber, receivedDate, msg;
boolean isReply = false;

void setup() {
  pinMode(RELAY_1, OUTPUT);  //Relay 1
  pinMode(RELAY_2, OUTPUT);  //Relay 2

  digitalWrite(RELAY_1, HIGH);
  digitalWrite(RELAY_2, HIGH);
  //delay(7000);

  Serial.begin(9600);
  Serial.println("esp32 serial initialize");

  sim800.begin(BAUD_RATE, SERIAL_8N1, rxPin, txPin);
  Serial.println("SIM800L serial initialize");

  smsStatus = "";
  senderNumber = "";
  receivedDate = "";
  msg = "";

  sim800.print("AT+CMGF=1");  //SMS text mode
  delay(1000);
}

void loop() {
  //////////////////////////////////////////////////
  while (sim800.available()) {
    parseData(sim800.readString());
  }
  //////////////////////////////////////////////////
  while (Serial.available()) {
    sim800.println(Serial.readString());
  }
  //////////////////////////////////////////////////
}  //main loop ends

//***************************************************
void parseData(String buff) {
  Serial.println(buff);

  unsigned int len, index;
  //////////////////////////////////////////////////
  //Remove sent "AT Command" from the response string.
  index = buff.indexOf("\r");
  buff.remove(0, index + 2);
  buff.trim();
  //////////////////////////////////////////////////

  //////////////////////////////////////////////////
  if (buff != "OK") {
    index = buff.indexOf(":");
    String cmd = buff.substring(0, index);
    cmd.trim();

    buff.remove(0, index + 2);

    if (cmd == "+CMTI") {
      //get newly arrived memory location and store it in temp
      index = buff.indexOf(",");
      String temp = buff.substring(index + 1, buff.length());
      temp = "AT+CMGR=" + temp + "\r";
      //get the message stored at memory location "temp"
      sim800.println(temp);
    } else if (cmd == "+CMGR") {
      extractSms(buff);


      if (senderNumber == PHONE) {
        doAction();
      }
    }
    //////////////////////////////////////////////////
  } else {
    //The result of AT Command is "OK"
  }
}

//************************************************************
void extractSms(String buff) {
  unsigned int index;

  index = buff.indexOf(",");
  smsStatus = buff.substring(1, index - 1);
  buff.remove(0, index + 2);

  senderNumber = buff.substring(0, 13);
  buff.remove(0, 19);

  receivedDate = buff.substring(0, 20);
  buff.remove(0, buff.indexOf("\r"));
  buff.trim();

  index = buff.indexOf("\n\r");
  buff = buff.substring(0, index);
  buff.trim();
  msg = buff;
  buff = "";
  msg.toLowerCase();
}

void doAction() {
  if (msg == "relay1 off") {
    digitalWrite(RELAY_1, HIGH);
    Reply("Relay 1 has been OFF");
  } else if (msg == "relay1 on") {
    digitalWrite(RELAY_1, LOW);
    Reply("Relay 1 has been ON");
  } else if (msg == "relay2 off") {
    digitalWrite(RELAY_2, HIGH);
    Reply("Relay 2 has been OFF");
  } else if (msg == "relay2 on") {
    digitalWrite(RELAY_2, LOW);
    Reply("Relay 2 has been ON");
  }


  smsStatus = "";
  senderNumber = "";
  receivedDate = "";
  msg = "";
}

void Reply(String text) {
  sim800.print("AT+CMGF=1\r");
  delay(1000);
  sim800.print("AT+CMGS=\"" + PHONE + "\"\r");
  delay(1000);
  sim800.print(text);
  delay(100);
  sim800.write(0x1A);  //ascii code for ctrl-26 //sim800.println((char)26); //ascii code for ctrl-26
  delay(1000);
  Serial.println("SMS Sent Successfully.");
}
*/