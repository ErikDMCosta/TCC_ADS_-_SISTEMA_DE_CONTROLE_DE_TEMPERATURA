// Importações para Módulo GSM
#include "Adafruit_FONA.h"

// PIN do cartão SIM (deixe em branco, se não for definido)
const char simPIN[] = "";

// O seu número de telefone para enviar SMS: + (sinal de mais) e código do país seguido do número de telefone
#define SMS_TARGET "+055999999999"

// Defina seu limite de temperatura (neste caso é 12,0 graus Celsius)
const float minTemp = 12.0;
// Defina seu limite de temperatura (neste caso é 16,0 graus Celsius)
const float maxTemp = 16.0;
// Variável sinalizadora para rastrear se o SMS de alerta foi enviado ou não
bool smsHasBeenSent = false;

// Importações para Termostato NTC
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>

esp_adc_cal_characteristics_t adc_cal;  // Estrutura que contem as informacoes para calibracao

// Temperatura
bool isUsingEsp32 = true;  // Mude para falso ao usar o Arduino

int thermistorPin;
double adcMaxValue, voltageSupply;

String realTemperature;
double resistorValue = 10000.0;           // resitor 10k
double betaValue = 3950.0;                // betaValue value
double referenceTemperature = 298.15;     // temperatura em kelvin Kelvin
double referenceResistorValue = 10000.0;  // resitor 10k em C

//////////////////
// Calibrando Tensão ESP32
//const float ADC_LUT[4096] PROGMEM =
//////////////////

// TTGO T-Call pinos
#define SIM800L_RX 27
#define SIM800L_TX 26
#define SIM800L_PWRKEY 4
#define SIM800L_RST 5
#define SIM800L_POWER 23
#define I2C_SDA 21
#define I2C_SCL 22

char replybuffer[255];

HardwareSerial *sim800lSerial = &Serial1;
Adafruit_FONA sim800l = Adafruit_FONA(SIM800L_PWRKEY);

uint8_t readLineFromBuffer(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

#define LED_BLUE 13

int temperatureExceedCount = -1;

String smsContent = "";

void setup() {
  // Define a taxa de transmissão do console
  Serial.begin(115200);

  // Setando pino do Termostato NTC
  pinMode(LED_BLUE, OUTPUT);
  pinMode(SIM800L_POWER, OUTPUT);
  digitalWrite(SIM800L_POWER, HIGH);

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
  // Quando um SMS é recebido
  sim800lSerial->print("AT+CNMI=2,1\r\n");
  Serial.println("Pronto para GSM SIM800L");

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0);                                                              // pin 34 ESP32 DEVKit V1
  esp_adc_cal_value_t adc_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 1100, &adc_cal);  // Inicializa a estrutura de calibracao

  ////////// Temperatura
  thermistorPin = 34;
  adcMaxValue = 4095.0;  // ADC 12-bit (0-4095)
  voltageSupply = 3.3;   // Voltagem
  ///////////
}

long prevMillis = 0;
int interval = 1000;
char sim800lNotificationBuffer[64];  // para notificações do FONA
char smsBuffer[250];
boolean ledState = false;

void loop() {
  // Temperatura em graus Celsius
  uint32_t adcValue = 0;

  for (int i = 0; i < 100; i++) {
    adcValue += adc1_get_raw(ADC1_CHANNEL_6);  // Obtem o valor RAW do ADC
    ets_delay_us(30);
  }

  adcValue /= 100;
  double Vout, Rt = 0;
  double Temperature, temperatureCelsius, temperatureFahrenheit = 0;
  double adcCaptured = 0;
  adcCaptured = analogRead(thermistorPin);
  adcCaptured = adcValue;

  Vout = adcCaptured * voltageSupply / adcMaxValue;
  Rt = resistorValue * Vout / (voltageSupply - Vout);
  Temperature = 1 / (1 / referenceTemperature + log(Rt / referenceResistorValue) / betaValue);  // Kelvin
  temperatureCelsius = Temperature - 273.15;                                                    // Celsius

  if ((temperatureCelsius > 0) && temperatureCelsius != -273.15) {
    Serial.print(temperatureCelsius);
    Serial.println(" Celsius");
  }
  delay(500);

  // <----> COMEÇA PARTE DE RECEBER SMS <---->
  char *bufPtr = sim800lNotificationBuffer;  // ponteiro de buffer útil

  if (sim800l.available()) {
    int slot = 0;  // este será o número do slot do SMS
    int charCount = 0;

    // Lê a notificação no fonaInBuffer
    do {
      *bufPtr = sim800l.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (sim800l.available()) && (++charCount < (sizeof(sim800lNotificationBuffer) - 1)));

    // Adicione um terminal NULL à string de notificação
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
      // Passe no buffer e comprimento máximo!
      if (sim800l.readSMS(slot, smsBuffer, 250, &smslen)) {
        smsContent = String(smsBuffer);
        Serial.println(smsContent);
      }

      if (smsContent == "Temperature") {
        Serial.println("Temperatura Manual Solicitada");
        String smsMessage = String("Temperatura Manual Solicitada: ") + String(temperatureCelsius) + String("C");
        Serial.println(smsMessage);
        char *smsMessageChar = const_cast<char *>(smsMessage.c_str());  // conversão explícita para char*
        delay(100);
        // Envia SMS para status
        if (!sim800l.sendSMS(SMS_TARGET, smsMessageChar)) {
          Serial.println(F("Falhou Temperatura Manual!"));
        } else {
          Serial.println(F("Enviado Temperatura Manual!"));
        }
      }
      // <----> TERMINA PARTE DE RECEBER SMS <---->
    }
  }

  // -- Adicionado uma FLAG de 3 turnos se acima ou abaixo da temperatura
  if (((temperatureCelsius > maxTemp) || (temperatureCelsius < minTemp)) && temperatureCelsius != -273.15) {
    if (temperatureExceedCount == 3) {
      smsHasBeenSent = false;
      temperatureExceedCount = 0;
      Serial.print(temperatureExceedCount);
      Serial.println(" Turno");
    } else {
      temperatureExceedCount++;
      Serial.print(temperatureExceedCount);
      Serial.println(" Turno");
    }
  } else if ((temperatureCelsius < maxTemp) || (temperatureCelsius > minTemp)) {
    temperatureExceedCount = 0;
    Serial.print(temperatureExceedCount);
    Serial.println(" Turno");
  }

  // Verifica se a temperatura está abaixo do limite e se precisa enviar o SMS de alerta
  if (((temperatureCelsius < minTemp) && temperatureCelsius != -273.15) && !smsHasBeenSent) {
    String smsMessage = String("Temperatura abaixo do limite: ") + String(temperatureCelsius) + String("C");
    Serial.println(smsMessage);
    char *smsMessageChar = const_cast<char *>(smsMessage.c_str());  // conversão explícita para char*
    // Envia SMS para status
    if (!sim800l.sendSMS(SMS_TARGET, smsMessageChar)) {
      Serial.println(F("Enviado Temperatura Abaixo!"));
      smsHasBeenSent = true;
      temperatureExceedCount = 0;
    } else {
      Serial.println(F("Falhou Temperatura Abaixo!"));
    }
  }

  // Verifica se a temperatura está acima do limite e se precisa enviar o SMS de alerta
  else if (((temperatureCelsius > maxTemp) && temperatureCelsius != -273.15) && !smsHasBeenSent) {
    String smsMessage = String("Temperatura acima do limite: ") + String(temperatureCelsius) + String("C");
    Serial.println(smsMessage);
    char *smsMessageChar = const_cast<char *>(smsMessage.c_str());  // conversão explícita para char*
    // Envia SMS para status
    if (sim800l.sendSMS(SMS_TARGET, smsMessageChar)) {
      smsHasBeenSent = true;
      temperatureExceedCount = 0;
      Serial.println(F("Enviado Temperatura Acima!"));
    } else {
      Serial.println(F("Falhou Temperatura Acima!"));
    }
  }
  delay(5000);
}
