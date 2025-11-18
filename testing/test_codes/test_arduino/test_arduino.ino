/**
 * Test de base Arduino MEGA
 * Vérifie que la carte fonctionne et que l'upload est OK
 */

#define LED_BUILTIN 13

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  
  Serial.println(F("========================================"));
  Serial.println(F("   Test Arduino MEGA 2560"));
  Serial.println(F("========================================"));
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.println(F("\n✓ Test démarré"));
  Serial.println(F("La LED intégrée devrait clignoter"));
  Serial.println(F("========================================\n"));
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println(F("LED ON"));
  delay(1000);
  
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println(F("LED OFF"));
  delay(1000);
}
