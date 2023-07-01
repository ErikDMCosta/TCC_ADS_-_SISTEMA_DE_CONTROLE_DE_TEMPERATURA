#include <GSM.h>
#include <GSM3CircularBuffer.h>
#include <GSM3IO.h>
#include <GSM3MobileAccessProvider.h>
#include <GSM3MobileCellManagement.h>
#include <GSM3MobileClientProvider.h>
#include <GSM3MobileClientService.h>
#include <GSM3MobileDataNetworkProvider.h>
#include <GSM3MobileMockupProvider.h>
#include <GSM3MobileNetworkProvider.h>
#include <GSM3MobileNetworkRegistry.h>
#include <GSM3MobileSMSProvider.h>
#include <GSM3MobileServerProvider.h>
#include <GSM3MobileServerService.h>
#include <GSM3MobileVoiceProvider.h>
#include <GSM3SMSService.h>
#include <GSM3ShieldV1.h>
#include <GSM3ShieldV1AccessProvider.h>
#include <GSM3ShieldV1BandManagement.h>
#include <GSM3ShieldV1BaseProvider.h>
#include <GSM3ShieldV1CellManagement.h>
#include <GSM3ShieldV1ClientProvider.h>
#include <GSM3ShieldV1DataNetworkProvider.h>
#include <GSM3ShieldV1DirectModemProvider.h>
#include <GSM3ShieldV1ModemCore.h>
#include <GSM3ShieldV1ModemVerification.h>
#include <GSM3ShieldV1MultiClientProvider.h>
#include <GSM3ShieldV1MultiServerProvider.h>
#include <GSM3ShieldV1PinManagement.h>
#include <GSM3ShieldV1SMSProvider.h>
#include <GSM3ShieldV1ScanNetworks.h>
#include <GSM3ShieldV1ServerProvider.h>
#include <GSM3ShieldV1VoiceProvider.h>
#include <GSM3SoftSerial.h>
#include <GSM3VoiceCallService.h>

// incluindo a biblioteca GSM

// Número PIN para o SIM
#define PINNUMBER ""

// inicializam as instâncias da biblioteca
GSM gsmAccess;
GSM_SMS sms;

// Array para segurar o número de onde um SMS é recuperado
char senderNumber[20];

void setup() {
  // inicializar as comunicações em série e esperar que a porta se abra:
  Serial.begin(9600);
  while (!Serial) {
    ;  // esperar que a porta serial seja conectada. Necessário apenas para a porta USB nativa
  }

  Serial.println("SMS Messages Receiver");

  // estado da conexão
  boolean notConnected = true;

  // Inicia a conexão GSM
  while (notConnected) {
    if (gsmAccess.begin(PINNUMBER) == GSM_READY) {
      notConnected = false;
    } else {
      Serial.println("Not connected");
      delay(1000);
    }
  }

  Serial.println("GSM initialized");
  Serial.println("Waiting for messages");
}

void loop() {
  char c;

  // Se houver algum SMS available()
  if (sms.available()) {
    Serial.println("Message received from:");

    // Obtém número remoto
    sms.remoteNumber(senderNumber, 20);
    Serial.println(senderNumber);

    // Um exemplo de eliminação de mensagens
    // Qualquer mensagem começando com # deve ser descartada
    if (sms.peek() == '#') {
      Serial.println("Discarded SMS");
      sms.flush();
    }

    // Ler os bytes de mensagem e imprimi-los
    while (c = sms.read()) {
      Serial.print(c);
    }

    Serial.println("\nEND OF MESSAGE");

    // Apaga a mensagem da memória do modem
    sms.flush();
    Serial.println("MESSAGE DELETED");
  }

  delay(1000);
}
