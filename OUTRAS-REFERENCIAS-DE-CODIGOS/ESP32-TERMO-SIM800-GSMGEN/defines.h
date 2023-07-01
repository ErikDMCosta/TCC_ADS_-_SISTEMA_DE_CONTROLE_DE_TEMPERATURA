#ifndef defines_h
#define defines_h

#define DEBUG_GSM_GENERIC_PORT Serial

// Nível de depuração de 0 a 5. Nível 5 é para imprimir comandos e respostas AT
#define _GSM_GENERIC_LOGLEVEL_ 5

#define SECRET_PINNUMBER ""
#define SECRET_GPRS_APN "zap.vivo.com.br"       // Substitua seu GPRS APN
#define SECRET_GPRS_LOGIN "vivo"        // Substitua por seu login GPRS
#define SECRET_GPRS_PASSWORD "vivo"  // Substitua por sua senha GPRS

//////////////////////////////////////////////

#if defined(ARDUINO_SAMD_MKRGSM1400)

// Para MKRGSM1400 original => GSM_MODEM_UBLOX == true, GSM_MODEM_LARAR2 == false
// Para MKRGSM1400 modificado usando LARA R2 => GSM_MODEM_UBLOX == false, GSM_MODEM_LARARAR2 == true
#define GSM_MODEM_UBLOX false

#if GSM_MODEM_UBLOX
#define GSM_MODEM_LARAR2 false
#else
#define GSM_MODEM_LARAR2 true
#endif

#define UBLOX_USING_RESET_PIN true
#define UBLOX_USING_LOW_POWER_MODE true

#if GSM_MODEM_UBLOX
#warning Using MKRGSM1400 Configuration with SARA U201
#elif GSM_MODEM_LARAR2
#warning Using MKRGSM1400 Configuration with LARA R2xx
#else
#error Must select either GSM_MODEM_UBLOX or GSM_MODEM_LARAR2
#endif

#else

// Uso opcional de GSM_RESETN e GSM_DTR. Só precisa estar aqui quando for verdade. A inadimplência é falsa.
#define UBLOX_USING_RESET_PIN true
#define UBLOX_USING_LOW_POWER_MODE true

// Substituir os pinos e porta padrão (e certamente não bons)
// Somente para placas que não ARDUINO_SAMD_MKRGSM1400
#if (ESP32)
#define GSM_RESETN (33u)
#define GSM_DTR (34u)
#elif (ESP8266)
#define GSM_RESETN (D3)
#define GSM_DTR (D4)
#else
#define GSM_RESETN (10u)
#define GSM_DTR (11u)
#endif

#if ESP8266
// Usando Software Serial para ESP8266, já que Serial1 é apenas TX
#define GSM_USING_SOFTWARE_SERIAL true
#else
// Software Serial Opcional aqui para outras placas, mas não aconselhado se HW Serial estiver disponível
#define GSM_USING_SOFTWARE_SERIAL false
#endif

#if GSM_USING_SOFTWARE_SERIAL
#warning Using default SerialGSM = SoftwareSerial

#define D8 (34)
#define D7 (35)

#include <SoftwareSerial.h>

SoftwareSerial swSerial(RX, TX, false, 256);  // (D7, D8, falso, 256); // (RX, TX, falso, 256);

#define SerialGSM swSerial
#else
#warning Using default SerialGSM = HardwareSerial Serial1
#define SerialGSM Serial1
#endif  // GSM_USING_SOFTWARE_SERIAL

#warning You must connect the Modem correctly and modify the pins / Serial port here

//////////////////////////////////////////////

#define GSM_MODEM_UBLOX false
#define GSM_MODEM_SARAR4 false
#define GSM_MODEM_LARAR2 false

//////////////////////////////////////////////
// Not supported yet
#define GSM_MODEM_SIM800 true
#define GSM_MODEM_SIM808 false
#define GSM_MODEM_SIM868 false
#define GSM_MODEM_SIM900 false
#define GSM_MODEM_SIM5300 false
#define GSM_MODEM_SIM5320 false
#define GSM_MODEM_SIM5360 false
#define GSM_MODEM_SIM7000 false
#define GSM_MODEM_SIM7100 false
#define GSM_MODEM_SIM7500 false
#define GSM_MODEM_SIM7600 false
#define GSM_MODEM_SIM7800 false
#define GSM_MODEM_M95 false
#define GSM_MODEM_BG96 false
#define GSM_MODEM_A6 false
#define GSM_MODEM_A7 false
#define GSM_MODEM_M590 false
#define GSM_MODEM_MC60 false
#define GSM_MODEM_MC60E false
#define GSM_MODEM_XBEE false
#define GSM_MODEM_SEQUANS_MONARCH false
//////////////////////////////////////////////

#endif

// Bibliotecas usadas
#include <GSM_Generic_Main.h>

#endif  //defines_h
