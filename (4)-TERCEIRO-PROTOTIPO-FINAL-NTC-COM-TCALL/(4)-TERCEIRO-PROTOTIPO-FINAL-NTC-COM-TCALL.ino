// Importações para Termostato NTC
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>

// Nescessário antes de importar configurar biblioteca TinyGSM
#define TINY_GSM_MODEM_SIM800    // Modem é SIM800
#define TINY_GSM_RX_BUFFER 1024  // Define o buffer RX para 1Kb

// Importações para Módulo GSM
#include <Wire.h>
#include <TinyGsmClient.h>

// O seu número de telefone para enviar SMS: + (sinal de mais) e código do país seguido do número de telefone
#define SMS_TARGET "+055999999999"

// TTGO T-Call pinos
#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 23
#define MODEM_TX 27
#define MODEM_RX 26
#define I2C_SDA 21
#define I2C_SCL 22

// Definir serial para console de depuração (para Serial Monitor, velocidade padrão 115200)
#define SerialMon Serial
// Definir serial para comandos AT (para o módulo SIM800)
#define SerialAT Serial1

// Defina o console serial para impressões de depuração, se necessário
// #define DUMP_AT_COMMANDS

#ifdef DUMP_AT_COMMANDS
#else
TinyGsm modem(SerialAT);
#endif
#define IP5306_ADDR 0x75
#define IP5306_REG_SYS_CTL0 0x00

// Declaracao da variavel do pino do botão
#define GPIO_BOTAO 35
#define TEMPO_DEBOUNCE 10  // Milissegundos

esp_adc_cal_characteristics_t adc_cal;  // Estrutura que contém informações para calibração

// Temperatura
bool isUsingEsp32 = true;  // Mude para falso ao usar o Arduino
int thermistorPin;
double adcMaxValue, voltageSupply;

String realTemperature;
double resistorValue = 10000.0;           // resitor 10k
double betaValue = 3950.0;                // valor betaValue
double referenceTemperature = 298.15;     // temperatura em Kelvin
double referenceResistorValue = 10000.0;  // resitor 10k em C

//////////////////
// Calibrando Tensão ESP32 caso for nescessário
//const float ADC_LUT[4096] PROGMEM =
//////////////////

// Defina seu limite de temperatura (neste caso é 12,0 graus Celsius)
const float minTemp = 12.0;

// Defina seu limite de temperatura (neste caso é 16,0 graus Celsius)
const float maxTemp = 16.0;

// Cria a variavel que salvara o estado atual do tolerância fora da faixa de temperatura
int temperatureExceedCount = -1;

// PIN do cartão SIM (deixe em branco, se não for definido)
const char simPIN[] = "";

// Variável sinalizadora para rastrear se o SMS de alerta foi enviado ou não
bool smsHasBeenSent = false;

// Defina o valor inicial do contador do botão e quando ao ultimo acionamento
int trigger_counter = 0;
unsigned long last_triggered_timestamp = 0;

// Defina o tempo máximo entre os cliques (em milissegundos)
const unsigned long maximumTimeBetweenClicks = 30000;  // Por exemplo, 30 segundos

// Variável para armazenar o momento do último clique
unsigned long lastClick = 0;

// Variável para controlar a repetição dos acionamentos
bool repetitive_drives = false;

bool setPowerBoostKeepOn(int en) {
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  if (en) {
    Wire.write(0x37);  // Definir bit1: 1 habilitar 0 desabilitar boost continue ligado
  } else {
    Wire.write(0x35);  // 0x37 é o valor de registro padrão
  }
  return Wire.endTransmission() == 0;
}

/* Função ISR (chamada quando há geração da
interrupção) */
void IRAM_ATTR funcao_ISR() {
  /* Conta acionamentos do botão considerando debounce */
  if ((millis() - last_triggered_timestamp) >= TEMPO_DEBOUNCE) {
    trigger_counter++;
    last_triggered_timestamp = millis();
  }
}

void setup() {
  // Define a taxa de transmissão do console
  SerialMon.begin(115200);

  // Mantém a energia ao funcionar com bateria
  Wire.begin(I2C_SDA, I2C_SCL);
  bool isOk = setPowerBoostKeepOn(1);
  SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FALHOU"));

  // Setando pino do Termostato NTC
  pinMode(2, OUTPUT);
  // Definir reinicialização do modem, habilitar, pinos de energia GSM
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  /*
  Configura o GPIO do botão como entrada
  e configura interrupção externa no modo
  RISING para ele.
  */
  pinMode(GPIO_BOTAO, INPUT);
  attachInterrupt(GPIO_BOTAO, funcao_ISR, RISING);

  // Defina a taxa de baud do módulo GSM e os pinos UART
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Reinicia o módulo SIM800, leva algum tempo
  // Para ignorá-lo, chame init() ao invés de restart()
  SerialMon.println("Inicializando modem...");
  modem.restart();
  // use modem.init() se você não precisar da reinicialização completa

  // Desbloqueie seu cartão SIM com um PIN, se necessário
  if (strlen(simPIN) && modem.getSimStatus() != 3) {
    modem.simUnlock(simPIN);
  }

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0);                                                              // Pin 34 ESP32 DEVKit V1
  esp_adc_cal_value_t adc_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 1100, &adc_cal);  // Inicializa a estrutura de calibracao

  ////////// Temperatura

  thermistorPin = 34;
  adcMaxValue = 4095.0;  // ADC 12-bit (0-4095)
  voltageSupply = 3.3;   // Voltagem

  ///////////
}

void loop() {
  // Temperatura em graus Celsius
  uint32_t adcValue = 0;
  for (int i = 0; i < 100; i++) {
    adcValue += adc1_get_raw(ADC1_CHANNEL_6);  // Obtém o valor RAW do ADC
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

  // -- Adicionado umz FLAG de 3 turnos se acima ou abaixo da temperatura
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
    if (modem.sendSMS(SMS_TARGET, smsMessage)) {
      SerialMon.println(smsMessage);
      smsHasBeenSent = true;
      temperatureExceedCount = 0;
    } else {
      SerialMon.println("Falha ao enviar SMS de Temperatura Abaixo");
    }
  }

  // Verifica se a temperatura está acima do limite e se precisa enviar o SMS de alerta
  else if (((temperatureCelsius > maxTemp) && temperatureCelsius != -273.15) && !smsHasBeenSent) {
    String smsMessage = String("Temperatura acima do limite: ") + String(temperatureCelsius) + String("C");
    if (modem.sendSMS(SMS_TARGET, smsMessage)) {
      SerialMon.println(smsMessage);
      smsHasBeenSent = true;
      temperatureExceedCount = 0;
    } else {
      SerialMon.println("Falha ao enviar SMS de Temperatura Acima");
    }
    // Se o botao estiver pressionado aguarda 30 milisegundos, faz nova leitura e altera o estado atual salvando na variavel flagBotao o novo estado.
  } else if (trigger_counter) {
    // Verifica se já passou tempo suficiente desde o último clique
    if (millis() - lastClick >= maximumTimeBetweenClicks || !repetitive_drives) {
      // Aguarda um pequeno atraso antes de prosseguir
      delay(30);
      Serial.println("Botão de Temperatura Manual Acionado!");
      String smsMessage = String("Temperatura Manual Solicitada: ") + String(temperatureCelsius) + String("C");
      Serial.println(smsMessage);
      char *smsMessageChar = const_cast<char *>(smsMessage.c_str());  // conversão explícita para char*
      delay(100);

      do {
        // Envia SMS para status
        if (modem.sendSMS(SMS_TARGET, smsMessage)) {
          Serial.println("Enviado a Temperatura Manual!");
          trigger_counter = 0;
        } else {
          Serial.println("Falhou a Temperatura Manual!");
        }
      } while (trigger_counter != 0);  // fica aqui aguardando soltar o botao
      // Atualiza o momento do último clique
      lastClick = millis();
      // Define a variável repetitive_drives como verdadeira
      repetitive_drives = true;
    }
  }
  delay(5000);
}
