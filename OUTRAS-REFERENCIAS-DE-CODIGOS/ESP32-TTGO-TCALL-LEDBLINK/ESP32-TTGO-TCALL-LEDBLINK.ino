int LED_BUILTIN = 13;

void setup() {
  Serial.begin(115200);
  // inicializa o pino digital LED_BUILTIN como saída.
  pinMode(LED_BUILTIN, OUTPUT);
}

// a função de loop é executada repetidamente para sempre
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // liga o LED (HIGH é o nível de tensão)
  //Serial.println("LED Ligado!");
  delay(1000);                     // espera um segundo
  digitalWrite(LED_BUILTIN, LOW);  // desliga o LED tornando a voltagem BAIXA
  //Serial.println("LED Desligado!");
  delay(1000);  // espera um segundo
}
