void setup() {
  Serial.begin(9600);
  Serial2.begin(9600);
  delay(3000);
  test_sim800_module();
  send_SMS();
}

void loop() {
  updateSerial();
}


void test_sim800_module() {
  Serial2.println("AT");
  updateSerial();
  Serial.println();
  Serial2.println("AT+CSQ");
  updateSerial();
  Serial2.println("AT+CCID");
  updateSerial();
  Serial2.println("AT+CREG?");
  updateSerial();
  Serial2.println("ATI");
  updateSerial();
  Serial2.println("AT+CBC");
  updateSerial();
}


void updateSerial() {
  delay(500);
  while (Serial.available()) {
    Serial2.write(Serial.read());  // Encaminhar o que a Serial recebeu para a Porta Serial do Software
  }
  while (Serial2.available()) {
    Serial.write(Serial2.read());  // Encaminhar o Software Serial recebido para a Porta Serial
  }
}


void send_SMS() {
  Serial2.println("AT+CMGF=1");  // Configurando o modo TEXT
  updateSerial();
  Serial2.println("AT+CMGS=\"+919804049270\"");  // alterar ZZ com código de país e xxxxxxxxxxx com número de telefone para sms
  updateSerial();
  Serial2.print("Circuit Digest");  //text content
  updateSerial();
  Serial.println();
  Serial.println("Message Sent");
  Serial2.write(26);
}
