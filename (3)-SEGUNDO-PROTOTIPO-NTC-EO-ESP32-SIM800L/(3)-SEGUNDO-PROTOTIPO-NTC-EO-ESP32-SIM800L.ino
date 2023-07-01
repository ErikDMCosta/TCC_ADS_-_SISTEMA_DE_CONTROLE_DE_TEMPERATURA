#include "Adafruit_FONA.h"

#define SIM800L_RX 27
#define SIM800L_TX 26
#define SIM800L_PWRKEY 4
#define SIM800L_RST 5
#define SIM800L_POWER 23

char replybuffer[255];

HardwareSerial *sim800lSerial = &Serial1;
Adafruit_FONA sim800l = Adafruit_FONA(SIM800L_PWRKEY);

uint8_t readLineFromBuffer(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

#define LED_BLUE 13

String smsContent = "";

void setup() {
  pinMode(LED_BLUE, OUTPUT);
  pinMode(SIM800L_POWER, OUTPUT);
  digitalWrite(SIM800L_POWER, HIGH);

  Serial.begin(115200);
  Serial.println(F("ESP32 com GSM SIM800L"));
  Serial.println(F("Inicializando....(pode levar mais de 10 segundos)"));

  delay(10000);

  // Torne-o lento para facilitar a leitura!
  sim800lSerial->begin(4800, SERIAL_8N1, SIM800L_TX, SIM800L_RX);
  if (!sim800l.begin(*sim800lSerial)) {
    Serial.println(F("Não foi possível encontrar GSM SIM800L"));
    while (1)
      ;
  }
  Serial.println(F("GSM SIM800L está OK"));

  char imei[16] = { 0 };  // DEVE usar um buffer de 16 caracteres para IMEI!
  uint8_t imeiLen = sim800l.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("IMEI do cartão SIM: ");
    Serial.println(imei);
  }

  // Configure o FONA para enviar uma notificação +CMTI
  // quando um SMS é recebido
  sim800lSerial->print("AT+CNMI=2,1\r\n");

  Serial.println("Pronto para GSM SIM800L");
}

long prevMillis = 0;
int interval = 1000;
char sim800lNotificationBuffer[64];  // Para notificações do FONA
char smsBuffer[250];
boolean ledState = false;

void loop() {
  // Deixa LED AZUL piscando!
  if (millis() - prevMillis > interval) {
    ledState = !ledState;
    digitalWrite(LED_BLUE, ledState);

    prevMillis = millis();
  }

  char *bufPtr = sim800lNotificationBuffer;  //ponteiro de buffer útil

  if (sim800l.available()) {
    int slot = 0;  // Este será o número do slot do SMS
    int charCount = 0;

    // Lê a notificação no fonaInBuffer
    do {
      *bufPtr = sim800l.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (sim800l.available()) && (++charCount < (sizeof(sim800lNotificationBuffer) - 1)));

    // Adiciona um terminal NULL à string de notificação
    *bufPtr = 0;

    // Verifique a string de notificação para uma notificação recebida por SMS.
    // Se for uma mensagem SMS, obteremos o número do slot em 'slot'
    if (1 == sscanf(sim800lNotificationBuffer, "+CMTI: \"SM\",%d", &slot)) {
      Serial.print("Slot: ");
      Serial.println(slot);

      char callerIDbuffer[32];  //vamos armazenar o número do remetente do SMS aqui

      // Recupera o endereço/número de telefone do remetente do SMS.
      if (!sim800l.getSMSSender(slot, callerIDbuffer, 31)) {
        Serial.println("Não encontrei mensagem SMS no slot!");
      }
      Serial.print(F("DE: "));
      Serial.println(callerIDbuffer);

      // Recupera o valor do SMS.
      uint16_t smslen;
      // Passe no buffer e max len!
      if (sim800l.readSMS(slot, smsBuffer, 250, &smslen)) {
        smsContent = String(smsBuffer);
        Serial.println(smsContent);
      }

      if (smsContent == "LED ON") {
        Serial.println("O Led está ativado.");
        digitalWrite(LED_BLUE, HIGH);
        delay(100);
        // Envia SMS para status
        if (!sim800l.sendSMS(callerIDbuffer, "O Led está ativado.")) {
          Serial.println(F("Falhou!"));
        } else {
          Serial.println(F("Enviado!"));
        }
      } else if (smsContent == "LED OFF") {
        Serial.println("O Led está desativado.");
        digitalWrite(LED_BLUE, LOW);
        delay(100);
        // Envia SMS para status
        if (!sim800l.sendSMS(callerIDbuffer, "O Led está desativado.")) {
          Serial.println(F("Falhou!"));
        } else {
          Serial.println(F("Enviado!"));
        }
      }

      if (sim800l.deleteSMS(slot)) {
        Serial.println(F("OK!"));
      } else {
        Serial.print(F("Não foi possível excluir o SMS no slot "));
        Serial.println(slot);
        sim800l.print(F("AT+CMGD=?\r\n"));
      }
    }
  }
}
