// Importando Bibliotecas (Lib / Library) Necessárias
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>

esp_adc_cal_characteristics_t adc_cal;  // Estrutura que contém as informações para calibração

// Variáveis usados em Temperatura
bool isUsingEsp32 = true;  // Mude para falso ao usar o Arduino

int thermistorPin;
double adcMaxValue, voltageSupply;

String realTemperature;
double resistorValue = 10000.0;           // Resitor 10k
double betaValue = 3950.0;                // BetaValue valor
double referenceTemperature = 298.15;     // Temperatura em kelvin Kelvin
double referenceResistorValue = 10000.0;  // Resitor 10k em C

// Calibrando tensão da ESP caso seja nescessário
// const float ADC_LUT[4096] PROGMEM =

void setup() {
  pinMode(2, OUTPUT);

  Serial.begin(115200);
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0);                                                              //pin 34 esp32 devkit v1
  esp_adc_cal_value_t adc_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 1100, &adc_cal);  //Inicializa a estrutura de calibração

  // Temperatura NTC com a ESP 32
  thermistorPin = 34;    // Uma das pontas do NTC no Pino 34
  adcMaxValue = 4095.0;  // ADC 12-bit (0-4095)
  voltageSupply = 3.3;   // Voltagem do Pino da Placa
}

void loop() {
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
  Temperature = 1 / (1 / referenceTemperature + log(Rt / referenceResistorValue) / betaValue);  // Obtém o Resultado em Kelvin
  temperatureCelsius = Temperature - 273.15;                                                    // Obtém o Resultado em Celsius
  temperatureFahrenheit = temperatureCelsius * 9 / 5 + 32;                                      // Obtém o Resultado em Fahrenheit

  // Temperatura em unidade Celsius
  if (temperatureCelsius > 0) Serial.print(temperatureCelsius);
  Serial.println(" Celsius");
  delay(500);
  delay(2000);

  // Temperatura em unidade Fahrenheit
  if (temperatureFahrenheit > 0) Serial.print(temperatureFahrenheit);
  Serial.println(" Fahrenheit");
  delay(2000);

  // Caso Temperatura Maior ou Igual a 20ºC então Liga LED se não Desliga
  if (temperatureCelsius >= 20) {
    digitalWrite(2, false);
  } else {
    digitalWrite(2, true);
  }
}
