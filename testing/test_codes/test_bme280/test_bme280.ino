/**
 * @file test_bme280.ino
 * @brief Programme de test du capteur BME280
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-21
 * 
 * @details
 * Programme de test pour validation matérielle du BME280.
 * 
 * Objectifs :
 * - Vérifier la présence du capteur sur I2C
 * - Valider le câblage et l'alimentation
 * - Observer les valeurs de température, humidité, pression
 * - Comprendre le comportement du capteur
 * - Tester le calcul du point de rosée
 * 
 * Connexions :
 * - VCC  → 3.3V (recommandé) ou 5V si module avec régulateur
 * - GND  → GND
 * - SCL  → Pin 21 (Arduino MEGA)
 * - SDA  → Pin 20 (Arduino MEGA)
 * 
 * @warning Certains modules BME280 sont en 3.3V uniquement !
 * @note Vérifier l'adresse I2C de votre module (0x76 ou 0x77)
 */

#include "BME280Sensor.h"

// ============================================
// CONFIGURATION
// ============================================
#define LED_BUILTIN   13
#define SERIAL_BAUD   115200

// Seuils de test (ajustables)
#define TEMP_MIN_WARNING      10.0    // °C
#define TEMP_MAX_WARNING      30.0    // °C
#define HUMIDITY_MAX_WARNING  70.0    // %

// ============================================
// VARIABLES GLOBALES
// ============================================
BME280Sensor bme;
unsigned long lastPrint = 0;
bool detailMode = false; // Mode affichage détaillé


// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(3000);
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   TEST BME280 - Vérification Hardware  ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // Initialiser I2C
  Serial.println(F("Initialisation I2C..."));
  Wire.begin();
  Wire.setClock(100000); // 100kHz pour BME280
  Serial.println(F("✓ I2C @ 100kHz\n"));
  
  // Scanner I2C
  scanI2C();
  
  // Initialiser capteur
  Serial.print(F("Initialisation BME280 (adresse 0x"));
  Serial.print(BME280_I2C_ADDR, HEX);
  Serial.println(F(")..."));
  Serial.println();
  
  if (!bme.begin()) {
    Serial.println(F("❌ ÉCHEC initialisation BME280"));
    Serial.println();
    Serial.println(F("Vérifications à faire :"));
    Serial.println(F("  1. Câblage VCC, GND, SDA, SCL"));
    Serial.println(F("  2. Adresse I2C (0x76 ou 0x77 ?)"));
    Serial.println(F("  3. Tension alimentation (3.3V ou 5V)"));
    Serial.println(F("  4. Pull-ups I2C présentes"));
    Serial.println();
    Serial.println(F("Programme arrêté."));
    
    while (1) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
    }
  }
  
  Serial.println(F("✓ BME280 prêt !"));
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Attendre stabilisation (première lecture peut être fausse)
  Serial.println();
  Serial.println(F("Stabilisation capteur (3 secondes)..."));
  delay(3000);
  
  // Première lecture
  bme.forceUpdate();
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      MESURES EN TEMPS RÉEL             ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  printMenu();
}

// ============================================
// LOOP
// ============================================
void loop() {
  // Mettre à jour capteur (toutes les 2 secondes par défaut)
  if (bme.update()) {
    // Affichage périodique
    if (millis() - lastPrint >= 2000) {
      lastPrint = millis();
      printValues();
      
      // Feedback LED selon conditions
      updateLED();
    }
  }
  
  // Commandes
  if (Serial.available()) {
    handleCommand(Serial.read());
    while (Serial.available()) Serial.read(); // Vider buffer
  }

  delay(10);
}

// ============================================
// GESTION COMMANDES
// ============================================
/**
 * @brief Traite les commandes série
 */
void handleCommand(char cmd) {
  Serial.println();
  
  switch (cmd) {
    case '1':
      bme.setSampleInterval(1000);
      Serial.println(F("✓ Mode rapide (1s)"));
      break;
      
    case '2':
      bme.setSampleInterval(2000);
      Serial.println(F("✓ Mode normal (2s)"));
      break;
      
    case '3':
      bme.setSampleInterval(5000);
      Serial.println(F("✓ Mode lent (5s)"));
      break;
      
    case 'd':
    case 'D':
      detailMode = !detailMode;
      Serial.print(F("✓ Affichage détaillé : "));
      Serial.println(detailMode ? F("ACTIVÉ") : F("DÉSACTIVÉ"));
      break;
      
    case 't':
    case 'T':
      testReactivity();
      break;
      
    case 'c':
    case 'C':
      testCondensation();
      break;
      
    case 'a':
    case 'A':
      adjustSeaLevelPressure();
      break;
      
    case 'h':
    case 'H':
      printMenu();
      break;
      
    case 'i':
    case 'I':
      scanI2C();
      break;
      
    case 'r':
    case 'R':
      Serial.println(F("Redémarrage..."));
      delay(100);
      asm volatile ("jmp 0");
      break;
      
    default:
      if (cmd >= 32 && cmd <= 126) {
        Serial.print(F("Commande inconnue: "));
        Serial.println(cmd);
      }
      break;
  }
}

// ============================================
// FONCTIONS AFFICHAGE
// ============================================
/**
 * @brief Affiche le menu des commandes
 */
void printMenu() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      MENU TEST BME280                  ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("  1 - Mode rapide (lectures toutes les 1s)"));
  Serial.println(F("  2 - Mode normal (lectures toutes les 2s)"));
  Serial.println(F("  3 - Mode lent (lectures toutes les 5s)"));
  Serial.println(F("  D - Toggle affichage détaillé"));
  Serial.println(F("  T - Test réactivité (souffler sur capteur)"));
  Serial.println(F("  C - Calcul condensation"));
  Serial.println(F("  A - Ajuster pression niveau mer"));
  Serial.println(F("  H - Afficher cette aide"));
  Serial.println(F("  I - Scanner I2C"));
  Serial.println(F("  R - Redémarrer"));
  Serial.println();
}

/**
 * @brief Affiche les valeurs courantes
 */
void printValues() {
  BME280Data data = bme.getData();
  
  Serial.println(F("┌─────────────────────────────────────┐"));
  
  // Température
  Serial.print(F("│ Température : "));
  printFloat(data.temperature, 1, 6);
  Serial.print(F("°C "));
  
  if (bme.isTemperatureHigh(TEMP_MAX_WARNING)) {
    Serial.print(F("🔴"));
  } else if (bme.isTemperatureLow(TEMP_MIN_WARNING)) {
    Serial.print(F("🔵"));
  } else {
    Serial.print(F("🟢"));
  }
  Serial.println(F("   │"));
  
  // Humidité
  Serial.print(F("│ Humidité    : "));
  printFloat(data.humidity, 1, 6);
  Serial.print(F("%  "));
  
  if (bme.isHumidityHigh(HUMIDITY_MAX_WARNING)) {
    Serial.print(F("⚠️ "));
  } else {
    Serial.print(F("   "));
  }
  Serial.println(F("  │"));
  
  // Pression
  Serial.print(F("│ Pression    : "));
  printFloat(data.pressure, 1, 5);
  Serial.println(F(" hPa    │"));
  
  if (detailMode) {
    Serial.println(F("├─────────────────────────────────────┤"));
    
    // Altitude
    Serial.print(F("│ Altitude    : "));
    printFloat(data.altitude, 0, 6);
    Serial.println(F("m       │"));
    
    // Point de rosée
    float dewPoint = bme.getDewPoint();
    Serial.print(F("│ Point rosée : "));
    printFloat(dewPoint, 1, 6);
    Serial.println(F("°C      │"));
    
    // Risque condensation (sur surface à 15°C par exemple)
    Serial.print(F("│ Condensation: "));
    if (bme.hasCondensationRisk(15.0)) {
      Serial.print(F("⚠️  RISQUE (surf<15°C)"));
    } else {
      Serial.print(F("✓ OK (surf>15°C)    "));
    }
    Serial.println(F(" │"));
  }
  
  Serial.println(F("└─────────────────────────────────────┘"));
  Serial.println();
}

/**
 * @brief Affiche un float avec padding
 */
void printFloat(float value, int decimals, int width) {
  char buffer[16];
  dtostrf(value, width, decimals, buffer);
  Serial.print(buffer);
}

// ============================================
// FONCTIONS DE TEST
// ============================================
/**
 * @brief Test de réactivité du capteur
 */
void testReactivity() {
  Serial.println(F("=== TEST RÉACTIVITÉ ==="));
  Serial.println(F("\nSoufflez sur le capteur pendant 10 secondes..."));
  Serial.println(F("Observez l'augmentation de l'humidité\n"));
  
  BME280Data initial = bme.getData();
  Serial.print(F("Humidité initiale : "));
  Serial.print(initial.humidity, 1);
  Serial.println(F("%"));
  
  unsigned long start = millis();
  float maxHumidity = initial.humidity;
  
  while (millis() - start < 10000) {
    if (bme.forceUpdate()) {
      float h = bme.getHumidity();
      if (h > maxHumidity) {
        maxHumidity = h;
      }
      
      Serial.print(F("Humidité actuelle : "));
      Serial.print(h, 1);
      Serial.print(F("% (max: "));
      Serial.print(maxHumidity, 1);
      Serial.println(F("%)"));
      
      delay(500);
    }
  }
  
  Serial.print(F("\n✓ Variation mesurée : +"));
  Serial.print(maxHumidity - initial.humidity, 1);
  Serial.println(F("%"));
  
  if (maxHumidity - initial.humidity > 5.0) {
    Serial.println(F("✓ Capteur réactif !"));
  } else {
    Serial.println(F("⚠️  Faible variation, vérifier capteur"));
  }
  
  Serial.println(F("\n=== Fin test réactivité ===\n"));
}

/**
 * @brief Test calcul condensation
 */
void testCondensation() {
  Serial.println(F("=== CALCUL CONDENSATION ===\n"));
  
  BME280Data data = bme.getData();
  float dewPoint = bme.getDewPoint();
  
  Serial.print(F("Température actuelle : "));
  Serial.print(data.temperature, 1);
  Serial.println(F("°C"));
  
  Serial.print(F("Humidité actuelle    : "));
  Serial.print(data.humidity, 1);
  Serial.println(F("%"));
  
  Serial.print(F("Point de rosée       : "));
  Serial.print(dewPoint, 1);
  Serial.println(F("°C"));
  
  Serial.println(F("\nRisque condensation sur surfaces :"));
  
  float testTemps[] = {5, 10, 15, 20, 25};
  for (int i = 0; i < 5; i++) {
    Serial.print(F("  "));
    Serial.print(testTemps[i], 0);
    Serial.print(F("°C : "));
    
    if (bme.hasCondensationRisk(testTemps[i])) {
      Serial.println(F("⚠️  RISQUE"));
    } else {
      Serial.println(F("✓ OK"));
    }
  }
  
  Serial.println(F("\n💡 Conseil : Maintenir surfaces > "));
  Serial.print(dewPoint + 2, 0);
  Serial.println(F("°C pour éviter condensation"));
  
  Serial.println(F("\n=== Fin calcul condensation ===\n"));
}

/**
 * @brief Ajuster la pression au niveau de la mer
 */
void adjustSeaLevelPressure() {
  Serial.println(F("\n=== AJUSTER PRESSION NIVEAU MER ==="));
  Serial.println(F("\nPression actuelle : "));
  Serial.print(bme.getSeaLevelPressure(), 2);
  Serial.println(F(" hPa"));
  
  Serial.println(F("\nEntrer nouvelle pression (ex: 1013.25) : "));
  
  while (!Serial.available());
  float newPressure = Serial.parseFloat();
  while (Serial.available()) Serial.read();
  
  if (newPressure >= 950 && newPressure <= 1050) {
    bme.setSeaLevelPressure(newPressure);
    Serial.print(F("\n✓ Nouvelle pression : "));
    Serial.print(newPressure, 2);
    Serial.println(F(" hPa"));
    
    // Forcer lecture pour recalculer altitude
    bme.forceUpdate();
    
    Serial.print(F("✓ Nouvelle altitude : "));
    Serial.print(bme.getAltitude(), 0);
    Serial.println(F("m"));
  } else {
    Serial.println(F("\n❌ Valeur invalide (doit être 950-1050 hPa)"));
  }
  
  Serial.println();
}

// ============================================
// FONCTIONS UTILITAIRES
// ============================================
/**
 * @brief Scanne le bus I2C
 */
void scanI2C() {
  Serial.println(F("\n=== SCAN I2C ==="));
  byte count = 0;
  
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  0x"));
      if (addr < 16) Serial.print(F("0"));
      Serial.print(addr, HEX);
      
      if (addr == 0x76 || addr == 0x77) {
        Serial.print(F(" - BME280 ✓"));
      }
      Serial.println();
      count++;
    }
  }
  
  Serial.print(F("\nTotal: "));
  Serial.print(count);
  Serial.println(F(" périphérique(s)\n"));
  
  if (count == 0) {
    Serial.println(F("⚠️  Aucun périphérique I2C détecté !"));
  }
}

/**
 * @brief Met à jour la LED selon conditions
 */
void updateLED() {
  // LED fixe si tout OK
  // Clignotement si température ou humidité hors limites
  
  if (bme.isTemperatureHigh(TEMP_MAX_WARNING) || 
      bme.isTemperatureLow(TEMP_MIN_WARNING) ||
      bme.isHumidityHigh(HUMIDITY_MAX_WARNING)) {
    // Clignotement
    digitalWrite(LED_BUILTIN, (millis() / 500) % 2);
  } else {
    // Fixe
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

