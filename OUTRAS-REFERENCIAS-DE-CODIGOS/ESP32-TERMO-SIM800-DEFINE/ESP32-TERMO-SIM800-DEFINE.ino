#include "defines.h"

// Por favor, insira seus dados sensíveis na aba Secret ou arduino_secrets.h
// Número PIN
const char PINNUMBER[] = "+555596444526";

// BaudRate para se comunicar com o modem GSM/GPRS. Se for limitado a um máximo de 115200 dentro do modem
unsigned long baudRateSerialGSM = 115200;

// Inicializam as instâncias da biblioteca
GSM gsmAccess;
GSM_SMS sms;

// Array para segurar o número do qual um SMS é recuperado
char senderNumber[20];

void setup() {
  // Inicializar as comunicações em série e esperar que a porta se abra:
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(200);
  Serial.print(F("\nIniciando o ReceiveSMS em"));
  //Serial.println(BOARD_NAME);
  //Serial.println(GSM_GENERIC_VERSION);

#if (defined(DEBUG_GSM_GENERIC_PORT) && (_GSM_GENERIC_LOGLEVEL_ > 4))
  MODEM.debug(DEBUG_GSM_GENERIC_PORT);
#endif

  // Estado da conexão
  bool connected = false;

  // Iniciar a conexão GSM
  while (!connected) {
    if (gsmAccess.begin(baudRateSerialGSM, PINNUMBER) == GSM_READY) {
      connected = true;
    } else {
      Serial.println("Não conectado");
      delay(1000);
    }
  }

  Serial.println("GSM Inicializado");
  Serial.println("Aguardando as mensagens");
}

void loop() {
  int c;

  // Se houver algum SMS disponível()
  if (sms.available()) {
    Serial.println("Mensagem recebida de: ");

    // Obtem número remoto
    sms.remoteNumber(senderNumber, 20);
    //Serial.println(senderNumber);

    // Um exemplo de eliminação de mensagens
    // Qualquer mensagem começando com # deve ser descartada
    if (sms.peek() == '#') {
      Serial.println("SMS Descartado");
      sms.flush();
    }

    // Ler bytes de mensagem e imprimi-los
    while ((c = sms.read()) != -1) {
      Serial.print((char)c);
    }

    Serial.println("\nFIM DA MENSAGEM");

    // Apagar mensagem da memória do modem
    sms.flush();
    Serial.println("MENSAGEM ELIMINADA");
  }

  delay(1000);
}
