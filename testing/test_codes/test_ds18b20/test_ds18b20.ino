/**
 * @file test_ds18b20.ino
 * @brief Programme de test pour le capteur DS18B20
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-21
 * 
 * @details
 * Objectifs du test :
 * - Validation de la connexion matérielle
 * - Vérification de la lecture de température
 * - Test de la résolution
 * - Détection de plusieurs capteurs sur le même bus
 * 
 * Configuration matérielle :
 * - Arduino Mega
 * - DS18B20 sur bus OneWire
 * - Résistance pull-up 4.7kΩ entre DATA et VCC
 */

#include "DS18B20Sensor.h"

// ============================================
// CONFIGURATION
// ============================================
#define DS18B20_PIN       22          ///< Broche OneWire (à adapter)
#define SERIAL_BAUD       115200      ///< Vitesse série
#define UPDATE_INTERVAL   2000        ///< Intervalle de mise à jour (ms)

// ============================================
// VARIABLES GLOBALES
// ============================================
DS18B20Sensor tempSensor(DS18B20_PIN, UPDATE_INTERVAL);
bool testMode = true;         ///< Mode test interactif
unsigned long lastDisplay = 0;
const uint16_t displayInterval = 1000;


// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);  // Initialisation série
  delay(3000);
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   TEST DS18B20 - Vérification Hardware ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();

  
  // Configuration
  Serial.print(F("Broche OneWire : GPIO "));
  Serial.println(DS18B20_PIN);
  Serial.print(F("Intervalle MAJ : "));
  Serial.print(UPDATE_INTERVAL);
  Serial.println(F(" ms"));
  Serial.println();
  
  // Initialisation capteur
  Serial.print(F("Initialisation DS18B20... "));
  
  if (tempSensor.begin()) {
    Serial.println(F("✓ OK"));
    
    uint8_t count = tempSensor.getSensorCount();
    Serial.print(F("Capteurs détectés : "));
    Serial.println(count);
    
    // Afficher adresses
    for (uint8_t i = 0; i < count; i++) {
      Serial.print(F("  #"));
      Serial.print(i);
      Serial.print(F(" : "));
      
      uint8_t addr[8];
      if (tempSensor.getSensorAddress(i, addr)) {
        printAddress(addr);
      }
      Serial.println();
    }
  } else {
    Serial.println(F("✗ ÉCHEC"));
    Serial.println();
    Serial.println(F("Causes possibles :"));
    Serial.println(F("- Capteur non connecté"));
    Serial.println(F("- Résistance pull-up manquante (4.7kΩ)"));
    Serial.println(F("- Mauvaise broche OneWire"));
    Serial.println();
    Serial.println(F("Vérifiez le câblage !"));
    
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println();
  Serial.println(F("✓ Initialisation terminée"));
  
  // Test initial
  Serial.println();
  Serial.println(F("Lecture initiale..."));
  if (tempSensor.forceUpdate()) {
    for (uint8_t i = 0; i < tempSensor.getSensorCount(); i++) {
      Serial.print(F("  Capteur "));
      Serial.print(i);
      Serial.print(F(" : "));
      Serial.print(tempSensor.getTemperature(i), 2);
      Serial.println(F(" °C"));
    }
  }
  
  // Menu
  printMenu();
}

// ============================================
// LOOP
// ============================================
void loop() {
  // Mode monitoring
  if (!testMode) {
    printMonitoring();
    
    // Quitter monitoring
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'q' || c == 'Q') {
        testMode = true;
        Serial.println();
        Serial.println(F("Monitoring arrêté"));
        printMenu();
      }
      while (Serial.available()) Serial.read();
    }
    return;
  }
  
  // Commandes
  if (Serial.available()) {
    handleCommand(Serial.read());
    
    // Vider buffer
    while (Serial.available())  Serial.read();  // Vider le buffer
    
  }
}

// ============================================
// GESTION DES COMMANDES
// ============================================
/**
 * @brief Gère les commandes
 */
void handleCommand(char cmd) {
  Serial.println();
  
  switch (cmd) {
    case '1':
      testLectureSimple();
      break;

    case '2':
      testLectureContinue();
      break;

    case '3':
      testResolution9bits();
      break;

    case '4':
      testResolution12bits();
      break;

    case '5':
      testInfoCapteurs();
      break;

    case '6':
      testSeuilTemperature();
      break;

    case '7':
      testMonitoring();
      break;

    default:
      Serial.println(F("Commande inconnue (H pour aide)"));
      break;
  }
  
  if (testMode) {
    delay(500);
    printMenu();
  }
}

// ============================================
// FONCTIONS AFFICHAGE
// ============================================
/**
 * @brief Affiche le menu des tests
 */
void printMenu() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      MENU TEST DS18B20                 ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("1. Lecture simple"));
  Serial.println(F("2. Lecture continue (5 sec)"));
  Serial.println(F("3. Test résolution 9 bits"));
  Serial.println(F("4. Test résolution 12 bits"));
  Serial.println(F("5. Afficher infos capteurs"));
  Serial.println(F("6. Test seuil température"));
  Serial.println(F("7. Mode monitoring continu"));
  Serial.println();
  Serial.println(F("Tapez une commande ..."));
  Serial.println();
}

// ============================================
// FONCTIONS DE TEST
// ============================================
/**
 * @brief Test 1 : Lecture simple
 */
void testLectureSimple() {
  Serial.println(F("=== TEST 1 : LECTURE SIMPLE ==="));
  
  Serial.println(F("Lecture de la température..."));
  
  if (tempSensor.forceUpdate()) {
    for (uint8_t i = 0; i < tempSensor.getSensorCount(); i++) {
      float temp = tempSensor.getTemperature(i);
      
      Serial.print(F("Capteur "));
      Serial.print(i);
      Serial.print(F(" : "));
      
      if (temp != DS18B20_INVALID_TEMP) {
        Serial.print(temp, 2);
        Serial.println(F(" °C"));
      } else {
        Serial.println(F("ERREUR"));
      }
    }
  } else {
    Serial.println(F("✗ Échec de la lecture"));
  }
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 2 : Lecture continue
 */
void testLectureContinue() {
  Serial.println(F("=== TEST 2 : LECTURE CONTINUE (5 sec) ==="));
  
  Serial.println(F("Lecture toutes les secondes..."));
  Serial.println();
  
  unsigned long startTime = millis();
  uint8_t count = 0;
  
  while (millis() - startTime < 5000) {
    if (tempSensor.forceUpdate()) {
      count++;
      Serial.print(F("#"));
      Serial.print(count);
      Serial.print(F(" ["));
      Serial.print((millis() - startTime) / 1000.0, 1);
      Serial.print(F("s] : "));
      
      for (uint8_t i = 0; i < tempSensor.getSensorCount(); i++) {
        if (i > 0) Serial.print(F(" | "));
        Serial.print(F("S"));
        Serial.print(i);
        Serial.print(F("="));
        Serial.print(tempSensor.getTemperature(i), 2);
        Serial.print(F("°C"));
      }
      Serial.println();
      
      delay(1000);
    }
  }
  
  Serial.println(F("\n✓ Test terminé"));
  Serial.print(F("Nombre de lectures : "));
  Serial.println(count);
}

/**
 * @brief Test 3 : Résolution 9 bits
 */
void testResolution9bits() {
  Serial.println(F("=== TEST 3 : RÉSOLUTION 9 BITS (0.5°C) ==="));
  
  Serial.println(F("Configuration résolution 9 bits..."));
  tempSensor.setResolution(9);
  delay(100);
  
  Serial.println(F("Lecture (conversion ~94ms)..."));
  
  unsigned long start = millis();
  if (tempSensor.forceUpdate()) {
    unsigned long duration = millis() - start;
    
    Serial.print(F("Durée conversion : "));
    Serial.print(duration);
    Serial.println(F(" ms"));
    
    Serial.print(F("Température : "));
    Serial.print(tempSensor.getTemperature(), 4);
    Serial.println(F(" °C"));
    
    Serial.println(F("Note : Précision limitée à 0.5°C"));
  }
  
  // Restaurer résolution par défaut
  tempSensor.setResolution(12);
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 4 : Résolution 12 bits
 */
void testResolution12bits() {
  Serial.println(F("=== TEST 4 : RÉSOLUTION 12 BITS (0.0625°C) ==="));
  
  Serial.println(F("Configuration résolution 12 bits..."));
  tempSensor.setResolution(12);
  delay(100);
  
  Serial.println(F("Lecture (conversion ~750ms)..."));
  
  unsigned long start = millis();
  if (tempSensor.forceUpdate()) {
    unsigned long duration = millis() - start;
    
    Serial.print(F("Durée conversion : "));
    Serial.print(duration);
    Serial.println(F(" ms"));
    
    Serial.print(F("Température : "));
    Serial.print(tempSensor.getTemperature(), 4);
    Serial.println(F(" °C"));
    
    Serial.println(F("Note : Précision maximale 0.0625°C"));
  }
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 5 : Informations capteurs
 */
void testInfoCapteurs() {
  Serial.println(F("=== TEST 5 : INFORMATIONS CAPTEURS ==="));
  
  uint8_t count = tempSensor.getSensorCount();
  
  Serial.print(F("Nombre de capteurs détectés : "));
  Serial.println(count);
  Serial.println();
  
  if (count == 0) {
    Serial.println(F("✗ Aucun capteur détecté !"));
    Serial.println(F("Vérifiez le câblage et la résistance pull-up."));
    return;
  }
  
  for (uint8_t i = 0; i < count; i++) {
    Serial.print(F("Capteur #"));
    Serial.println(i);
    Serial.println(F("------------------------"));
    
    // Adresse OneWire
    uint8_t addr[8];
    if (tempSensor.getSensorAddress(i, addr)) {
      Serial.print(F("  Adresse : "));
      printAddress(addr);
      Serial.println();
    }
    
    // Température actuelle
    float temp = tempSensor.getTemperature(i);
    Serial.print(F("  Température : "));
    if (temp != DS18B20_INVALID_TEMP) {
      Serial.print(temp, 2);
      Serial.println(F(" °C"));
    } else {
      Serial.println(F("INVALIDE"));
    }
    
    Serial.println();
  }
  
  Serial.print(F("Statut : "));
  Serial.println(statusToString(tempSensor.getStatus()));
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 6 : Seuil température
 */
void testSeuilTemperature() {
  Serial.println(F("=== TEST 6 : TEST SEUIL TEMPÉRATURE ==="));
  
  const float seuil = 25.0;
  
  Serial.print(F("Seuil défini : "));
  Serial.print(seuil);
  Serial.println(F(" °C"));
  Serial.println();
  
  if (tempSensor.forceUpdate()) {
    for (uint8_t i = 0; i < tempSensor.getSensorCount(); i++) {
      float temp = tempSensor.getTemperature(i);
      
      Serial.print(F("Capteur "));
      Serial.print(i);
      Serial.print(F(" : "));
      Serial.print(temp, 2);
      Serial.print(F(" °C -> "));
      
      if (tempSensor.isAboveThreshold(i, seuil)) {
        Serial.println(F("⚠ AU-DESSUS du seuil"));
      } else {
        Serial.println(F("✓ En-dessous du seuil"));
      }
    }
    
    // Test plage
    Serial.println();
    Serial.println(F("Test plage [15°C - 30°C] :"));
    for (uint8_t i = 0; i < tempSensor.getSensorCount(); i++) {
      Serial.print(F("Capteur "));
      Serial.print(i);
      Serial.print(F(" : "));
      
      if (tempSensor.isInRange(i, 15.0, 30.0)) {
        Serial.println(F("✓ Dans la plage"));
      } else {
        Serial.println(F("✗ Hors plage"));
      }
    }
  }
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 7 : Mode monitoring continu
 */
void testMonitoring() {
  Serial.println(F("=== TEST 7 : MONITORING CONTINU ==="));
  
  Serial.println(F("Mode monitoring actif"));
  Serial.println(F("Tapez 'q' pour quitter"));
  Serial.println();
  
  testMode = false;
  lastDisplay = 0;
}

// ============================================
// AFFICHAGE MONITORING
// ============================================
/**
 * @brief Affiche les données en mode monitoring
 */
void printMonitoring() {
  unsigned long now = millis();
  
  if (now - lastDisplay >= displayInterval) {
    lastDisplay = now;
    
    if (tempSensor.update()) {
      // Heure
      Serial.print(F("["));
      Serial.print(now / 1000);
      Serial.print(F("s] "));
      
      // Températures
      for (uint8_t i = 0; i < tempSensor.getSensorCount(); i++) {
        if (i > 0) Serial.print(F(" | "));
        
        DS18B20Data data = tempSensor.getData(i);
        
        Serial.print(F("S"));
        Serial.print(i);
        Serial.print(F(": "));
        
        if (data.valid) {
          Serial.print(data.temperature, 2);
          Serial.print(F("°C"));
        } else {
          Serial.print(F("ERR"));
        }
      }
      
      // Statut
      Serial.print(F(" ["));
      Serial.print(statusToString(tempSensor.getStatus()));
      Serial.println(F("]"));
    }
  }
}

// ============================================
// FONCTIONS UTILITAIRES
// ============================================
/**
 * @brief Convertit le statut en texte
 * @param status Statut du capteur
 * @return Texte correspondant
 */
const char* statusToString(DS18B20Status status) {
  switch (status) {
    case DS18B20Status::OK:                  return "OK";
    case DS18B20Status::NOT_INITIALIZED:     return "NON INITIALISÉ";
    case DS18B20Status::NO_SENSOR_FOUND:     return "CAPTEUR NON DÉTECTÉ";
    case DS18B20Status::READ_ERROR:          return "ERREUR LECTURE";
    case DS18B20Status::INVALID_TEMPERATURE: return "TEMPÉRATURE INVALIDE";
    default:                                 return "INCONNU";
  }
}

/**
 * @brief Affiche une adresse OneWire
 * @param address Adresse du capteur
 */
void printAddress(const uint8_t* address) {
  char buffer[24];
  DS18B20Sensor::addressToString(address, buffer);
  Serial.print(buffer);
}

