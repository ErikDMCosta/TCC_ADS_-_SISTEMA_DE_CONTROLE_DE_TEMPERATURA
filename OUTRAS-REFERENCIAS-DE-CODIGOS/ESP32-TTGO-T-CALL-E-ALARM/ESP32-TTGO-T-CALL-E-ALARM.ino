// PIN do cartão SIM (deixe em branco, se não for definido)
const char simPIN[] = "";

// O seu número de telefone para enviar SMS: + (sinal de mais) e código do país, para Portugal +351, seguido do número de telefone
// Exemplo de SMS_TARGET para Portugal +351XXXXXXXXX
#define SMS_TARGET "+055999999999"

// Defina seu limite de temperatura (neste caso é 28,0 graus Celsius)
float temperatureThreshold = 28.0;
// Variável sinalizadora para rastrear se o SMS de alerta foi enviado ou não
bool smsHasBeenSent = false;

// Configurar biblioteca TinyGSM
#define TINY_GSM_MODEM_SIM800    // Modem é SIM800
#define TINY_GSM_RX_BUFFER 1024  // Define o buffer RX para 1Kb

#include <Wire.h>
#include <TinyGsmClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <stdlib.h>
#include <math.h>

// GPIO onde o DS18B20 está conectado
const int oneWireBus = 13;

// Configure uma instância oneWire para se comunicar com qualquer dispositivo OneWire
OneWire oneWire(oneWireBus);

// Passe nossa referência oneWire para o sensor de temperatura Dallas
DallasTemperature sensors(&oneWire);

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
//#define DUMP_AT_COMMANDS

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define IP5306_ADDR 0x75
#define IP5306_REG_SYS_CTL0 0x00

bool setPowerBoostKeepOn(int en) {
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  if (en) {
    Wire.write(0x37);  // Set bit1: 1 habilita 0 desabilita boost continue ligado
  } else {
    Wire.write(0x35);  // 0x37 é o valor de registro padrão
  }
  return Wire.endTransmission() == 0;
}

void setup() {
  // Define a taxa de transmissão do console
  SerialMon.begin(115200);

  // Mantém a energia ao funcionar com bateria
  Wire.begin(I2C_SDA, I2C_SCL);
  bool isOk = setPowerBoostKeepOn(1);
  SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FALHOU"));

  // Definir reinicialização do modem, habilitar, pinos de energia
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Set GSM module baud rate and UART pins
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

  // Inicia o sensor DS18B20
  sensors.begin();
}

void loop() {
  sensors.requestTemperatures();
  // Temperatura em graus Celsius
  //float temperature = sensors.getTempCByIndex(0);
  float temperature = roundf((float)rand() / RAND_MAX * 100.0f * 100.0f) / 100.0f;
  SerialMon.print(temperature);
  SerialMon.println("C");
  delay(1000);

  // Temperatura em graus Fahrenheit
  /*temperatura flutuante = sensores.getTempFByIndex(0);
   SerialMon.print(temperatura);
   SerialMon.println("*F");*/

  // Verifica se a temperatura está acima do limite e se precisa enviar o SMS de alerta
  if ((temperature > temperatureThreshold) && !smsHasBeenSent) {
    String smsMessage = String("Temperatura acima do limite: ") + String(temperature) + String("C");
    if (modem.sendSMS(SMS_TARGET, smsMessage)) {
      SerialMon.println(smsMessage);
      smsHasBeenSent = true;
    } else {
      SerialMon.println("Falha ao enviar SMS");
    }
  }
  // Verifica se a temperatura está abaixo do limite e se precisa enviar o SMS de alerta
  else if ((temperature < temperatureThreshold) && smsHasBeenSent) {
    String smsMessage = String("Temperatura abaixo do limite: ") + String(temperature) + String("C");
    if (modem.sendSMS(SMS_TARGET, smsMessage)) {
      SerialMon.println(smsMessage);
      smsHasBeenSent = false;
    } else {
      SerialMon.println("Falha ao enviar SMS");
    }
  }
  delay(5000);
}
