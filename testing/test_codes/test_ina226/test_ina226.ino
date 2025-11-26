/**
 * @file test_ina226.ino
 * @brief Programme de test pour les capteurs INA226 (12V et 5V)
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-21
 * 
 * @details
 * Objectifs du test :
 * - Validation de la connexion matérielle des deux capteurs
 * - Vérification des mesures de tension et courant
 * - Test des seuils d'alerte
 * - Surveillance indépendante des rails 12V et 5V
 * - Calcul de puissance et énergie
 * 
 * Configuration matérielle :
 * - ESP32
 * - 2x INA226 sur bus I2C
 * - INA226 #1 (12V) : adresse 0x40 (A1=GND, A0=GND)
 * - INA226 #2 (5V)  : adresse 0x41 (A1=GND, A0=VCC)
 * - Résistance shunt : 100 mΩ (0.1Ω)
 */

#include "INA226Sensor.h"

// ============================================
// CONFIGURATION
// ============================================
// Broches I2C pour Arduino Mega R3
// SDA = Pin 20 (fixe)
// SCL = Pin 21 (fixe)
// Note : Les broches I2C sont fixes sur le Mega, pas besoin de les définir
#define SERIAL_BAUD       115200  ///< Vitesse série

// Seuils personnalisés
#define RAIL_12V_MIN      10.5f   ///< Tension min 12V
#define RAIL_12V_MAX      14.5f   ///< Tension max 12V
#define RAIL_12V_CURRENT  20.0f   ///< Courant max 12V (avec shunt 2mΩ, max théorique 40A)

#define RAIL_5V_MIN       4.5f    ///< Tension min 5V
#define RAIL_5V_MAX       5.5f    ///< Tension max 5V
#define RAIL_5V_CURRENT   3.0f    ///< Courant max 5V (avec shunt 10mΩ, max théorique 8A)

// ============================================
// VARIABLES GLOBALES
// ============================================
INA226Sensor sensor12V(PowerRailType::RAIL_12V, INA226_I2C_ADDR_12V);
INA226Sensor sensor5V(PowerRailType::RAIL_5V, INA226_I2C_ADDR_5V);

bool testMode = true;
bool monitoringActive = false;
unsigned long energyStartTime = 0;
float totalEnergy12V = 0.0f;
float totalEnergy5V = 0.0f;

// ============================================
// SETUP
// ============================================
void setup() {
  // Initialisation série
  Serial.begin(SERIAL_BAUD);
  delay(3000);
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   TEST INA226 - Vérification Hardware  ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  // Initialisation I2C (Arduino Mega R3)
  // Broches I2C fixes : SDA=20, SCL=21
  Wire.begin();
  Serial.println(F("Bus I2C initialisé (SDA=20, SCL=21)"));
  Serial.println();
  
  // Initialisation capteur 12V
  Serial.print(F("Init capteur 12V (0x"));
  Serial.print(INA226_I2C_ADDR_12V, HEX);
  Serial.print(F(")... "));
  
  if (sensor12V.begin()) {
    Serial.println(F("✓ OK"));
    
    // Configurer seuils
    sensor12V.setVoltageThresholds(RAIL_12V_MIN, RAIL_12V_MAX);
    sensor12V.setCurrentThreshold(RAIL_12V_CURRENT);
  } else {
    Serial.println(F("✗ ÉCHEC"));
    Serial.println(F("Vérifiez :"));
    Serial.println(F("- Connexion I2C"));
    Serial.println(F("- Adresse capteur (A1/A0)"));
    Serial.println(F("- Alimentation capteur"));
  }
  
  // Initialisation capteur 5V
  Serial.print(F("Init capteur 5V (0x"));
  Serial.print(INA226_I2C_ADDR_5V, HEX);
  Serial.print(F(")... "));
  
  if (sensor5V.begin()) {
    Serial.println(F("✓ OK"));
    
    // Configurer seuils
    sensor5V.setVoltageThresholds(RAIL_5V_MIN, RAIL_5V_MAX);
    sensor5V.setCurrentThreshold(RAIL_5V_CURRENT);
  } else {
    Serial.println(F("✗ ÉCHEC"));
    Serial.println(F("Vérifiez :"));
    Serial.println(F("- Connexion I2C"));
    Serial.println(F("- Adresse capteur (A1/A0)"));
    Serial.println(F("- Alimentation capteur"));
  }
  
  Serial.println();
  
  // Vérifier si au moins un capteur fonctionne
  if (!sensor12V.isInitialized() && !sensor5V.isInitialized()) {
    Serial.println(F("✗ AUCUN CAPTEUR DÉTECTÉ"));
    Serial.println(F("Impossible de continuer les tests"));
    while (1) delay(1000);
  }
  
  Serial.println(F("✓ Initialisation terminée"));
  
  // Lecture initiale
  Serial.println();
  Serial.println(F("Lectures initiales :"));
  if (sensor12V.isInitialized()) {
    sensor12V.forceUpdate();
    Serial.print(F("  12V : "));
    Serial.print(sensor12V.getBusVoltage(), 2);
    Serial.print(F(" V, "));
    Serial.print(sensor12V.getCurrent(), 3);
    Serial.println(F(" A"));
  }
  
  if (sensor5V.isInitialized()) {
    sensor5V.forceUpdate();
    Serial.print(F("  5V  : "));
    Serial.print(sensor5V.getBusVoltage(), 2);
    Serial.print(F(" V, "));
    Serial.print(sensor5V.getCurrent(), 3);
    Serial.println(F(" A"));
  }
  
  // Menu
  printMenu();
}

// ============================================
// LOOP
// ============================================
void loop() {
  // Mode monitoring
  if (monitoringActive) {
    printMonitoring();
    delay(10);
    return;
  }
  
  // Mode test
  if (Serial.available()) {
    handleCommand(Serial.read());
    while (Serial.available()) Serial.read(); // Vider buffer

    if (testMode) {
      delay(500);
      printMenu();
    }
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
      testLectureSimple();
      break;
    case '2':
      testRail12V();
      break;
    case '3':
      testRail5V();
      break;
    case '4':
      testSeuils();
      break;
    case '5':
      testPuissanceEnergie();
      break;
    case '6':
      testComparaison();
      break;
    case '7':
      testMonitoring();
      break;
    case '8':
      testConfiguration();
      break;
    default:
      Serial.println(F("Commande inconnue"));
      break;
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
  Serial.println(F("║      MENU TEST INA226                  ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("1. Lecture simple (tous capteurs)"));
  Serial.println(F("2. Test rail 12V uniquement"));
  Serial.println(F("3. Test rail 5V uniquement"));
  Serial.println(F("4. Test seuils d'alerte"));
  Serial.println(F("5. Mesure puissance/énergie"));
  Serial.println(F("6. Comparaison 12V vs 5V"));
  Serial.println(F("7. Monitoring continu"));
  Serial.println(F("8. Afficher configuration"));
  Serial.println();
  Serial.println(F("Tapez le numéro du test..."));
  Serial.println();
}

/**
 * @brief Affiche les données d'un capteur
 * @param sensor Référence au capteur
 */
void printSensorData(const INA226Sensor& sensor) {
  Serial.print(F("Rail "));
  Serial.print(INA226Sensor::railTypeToString(sensor.getRailType()));
  Serial.print(F(" [0x"));
  Serial.print(sensor.getI2CAddress(), HEX);
  Serial.println(F("]"));
  Serial.println(F("------------------------"));
  
  char buffer[20];
  
  Serial.print(F("  Tension bus   : "));
  INA226Sensor::formatValue(sensor.getBusVoltage(), "V", buffer);
  Serial.println(buffer);
  
  Serial.print(F("  Tension shunt : "));
  INA226Sensor::formatValue(sensor.getShuntVoltageMilliVolts(), "mV", buffer);
  Serial.println(buffer);
  
  Serial.print(F("  Courant       : "));
  INA226Sensor::formatValue(sensor.getCurrent(), "A", buffer, 3);
  Serial.println(buffer);
  
  Serial.print(F("  Puissance     : "));
  INA226Sensor::formatValue(sensor.getPower(), "W", buffer);
  Serial.println(buffer);
  
  Serial.print(F("  Statut        : "));
  Serial.println(INA226Sensor::statusToString(sensor.getStatus()));
  
  // Indicateurs visuels
  if (!sensor.isVoltageOK()) {
    Serial.println(F("  ⚠ TENSION HORS LIMITES"));
  }
  if (!sensor.isCurrentOK()) {
    Serial.println(F("  ⚠ COURANT EXCESSIF"));
  }
  if (sensor.isHealthy()) {
    Serial.println(F("  ✓ Fonctionnement normal"));
  }
}

/**
 * @brief Affiche une barre de progression
 * @param value Valeur actuelle
 * @param max Valeur maximale
 * @param width Largeur de la barre
 */
void printProgressBar(float value, float max, uint8_t width = 20) {
  float percent = (value / max) * 100.0f;
  uint8_t filled = (value / max) * width;
  
  Serial.print(F("["));
  for (uint8_t i = 0; i < width; i++) {
    if (i < filled) {
      Serial.print(F("█"));
    } else {
      Serial.print(F("─"));
    }
  }
  Serial.print(F("] "));
  Serial.print(percent, 1);
  Serial.println(F("%"));
}

// ============================================
// FONCTIONS DE TEST
// ============================================
/**
 * @brief Test 1 : Lecture simple
 */
void testLectureSimple() {
  Serial.println(F("=== TEST 1 : LECTURE SIMPLE"));
  
  Serial.println(F("Lecture des deux capteurs..."));
  Serial.println();
  
  // Lecture 12V
  if (sensor12V.forceUpdate()) {
    printSensorData(sensor12V);
    Serial.println();
  } else {
    Serial.println(F("✗ Erreur lecture capteur 12V"));
    Serial.println();
  }
  
  // Lecture 5V
  if (sensor5V.forceUpdate()) {
    printSensorData(sensor5V);
    Serial.println();
  } else {
    Serial.println(F("✗ Erreur lecture capteur 5V"));
    Serial.println();
  }
  
  Serial.println(F("✓ Test terminé"));
}

/**
 * @brief Test 2 : Rail 12V uniquement
 */
void testRail12V() {
  Serial.println(F("=== TEST 2 : RAIL 12V UNIQUEMENT ==="));
  
  Serial.println(F("Mesures continues pendant 5 secondes..."));
  Serial.println();
  
  unsigned long startTime = millis();
  uint8_t count = 0;
  
  float minVoltage = 999.0f;
  float maxVoltage = 0.0f;
  float avgCurrent = 0.0f;
  
  while (millis() - startTime < 5000) {
    if (sensor12V.forceUpdate()) {
      count++;
      
      float voltage = sensor12V.getBusVoltage();
      float current = sensor12V.getCurrent();
      
      minVoltage = min(minVoltage, voltage);
      maxVoltage = max(maxVoltage, voltage);
      avgCurrent += current;
      
      Serial.print(F("#"));
      Serial.print(count);
      Serial.print(F(" : "));
      Serial.print(voltage, 2);
      Serial.print(F("V, "));
      Serial.print(current, 3);
      Serial.print(F("A, "));
      Serial.print(sensor12V.getPower(), 2);
      Serial.println(F("W"));
      
      delay(1000);
    }
  }
  
  avgCurrent /= count;
  
  Serial.println();
  Serial.println(F("Statistiques :"));
  Serial.print(F("  Tension min : "));
  Serial.print(minVoltage, 2);
  Serial.println(F(" V"));
  Serial.print(F("  Tension max : "));
  Serial.print(maxVoltage, 2);
  Serial.println(F(" V"));
  Serial.print(F("  Courant moy : "));
  Serial.print(avgCurrent, 3);
  Serial.println(F(" A"));
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 3 : Rail 5V uniquement
 */
void testRail5V() {
  Serial.println(F("=== TEST 3 : RAIL 5V UNIQUEMENT ==="));
  
  Serial.println(F("Mesures continues pendant 5 secondes..."));
  Serial.println();
  
  unsigned long startTime = millis();
  uint8_t count = 0;
  
  float minVoltage = 999.0f;
  float maxVoltage = 0.0f;
  float avgCurrent = 0.0f;
  
  while (millis() - startTime < 5000) {
    if (sensor5V.forceUpdate()) {
      count++;
      
      float voltage = sensor5V.getBusVoltage();
      float current = sensor5V.getCurrent();
      
      minVoltage = min(minVoltage, voltage);
      maxVoltage = max(maxVoltage, voltage);
      avgCurrent += current;
      
      Serial.print(F("#"));
      Serial.print(count);
      Serial.print(F(" : "));
      Serial.print(voltage, 2);
      Serial.print(F("V, "));
      Serial.print(current, 3);
      Serial.print(F("A, "));
      Serial.print(sensor5V.getPower(), 2);
      Serial.println(F("W"));
      
      delay(1000);
    }
  }
  
  avgCurrent /= count;
  
  Serial.println();
  Serial.println(F("Statistiques :"));
  Serial.print(F("  Tension min : "));
  Serial.print(minVoltage, 2);
  Serial.println(F(" V"));
  Serial.print(F("  Tension max : "));
  Serial.print(maxVoltage, 2);
  Serial.println(F(" V"));
  Serial.print(F("  Courant moy : "));
  Serial.print(avgCurrent, 3);
  Serial.println(F(" A"));
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 4 : Seuils d'alerte
 */
void testSeuils() {
  Serial.println(F("=== TEST 4 : SEUILS D'ALERTE ==="));
  
  Serial.println(F("Vérification des seuils configurés..."));
  Serial.println();
  
  // Afficher seuils 12V
  float vMin12, vMax12, iMax12;
  sensor12V.getThresholds(vMin12, vMax12, iMax12);
  
  Serial.println(F("Rail 12V :"));
  Serial.print(F("  Tension : ["));
  Serial.print(vMin12, 1);
  Serial.print(F("V - "));
  Serial.print(vMax12, 1);
  Serial.println(F("V]"));
  Serial.print(F("  Courant : max "));
  Serial.print(iMax12, 1);
  Serial.println(F("A"));
  Serial.println();
  
  // Afficher seuils 5V
  float vMin5, vMax5, iMax5;
  sensor5V.getThresholds(vMin5, vMax5, iMax5);
  
  Serial.println(F("Rail 5V :"));
  Serial.print(F("  Tension : ["));
  Serial.print(vMin5, 1);
  Serial.print(F("V - "));
  Serial.print(vMax5, 1);
  Serial.println(F("V]"));
  Serial.print(F("  Courant : max "));
  Serial.print(iMax5, 1);
  Serial.println(F("A"));
  Serial.println();
  
  // Test des mesures actuelles
  Serial.println(F("Mesures actuelles vs seuils :"));
  Serial.println();
  
  sensor12V.forceUpdate();
  sensor5V.forceUpdate();
  
  // Test 12V
  Serial.println(F("Rail 12V :"));
  Serial.print(F("  Tension : "));
  Serial.print(sensor12V.getBusVoltage(), 2);
  Serial.print(F(" V "));
  Serial.println(sensor12V.isVoltageOK() ? F("✓") : F("✗"));
  
  Serial.print(F("  Courant : "));
  Serial.print(sensor12V.getCurrent(), 3);
  Serial.print(F(" A "));
  Serial.println(sensor12V.isCurrentOK() ? F("✓") : F("✗"));
  Serial.println();
  
  // Test 5V
  Serial.println(F("Rail 5V :"));
  Serial.print(F("  Tension : "));
  Serial.print(sensor5V.getBusVoltage(), 2);
  Serial.print(F(" V "));
  Serial.println(sensor5V.isVoltageOK() ? F("✓") : F("✗"));
  
  Serial.print(F("  Courant : "));
  Serial.print(sensor5V.getCurrent(), 3);
  Serial.print(F(" A "));
  Serial.println(sensor5V.isCurrentOK() ? F("✓") : F("✗"));
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 5 : Puissance et énergie
 */
void testPuissanceEnergie() {
  Serial.println(F("=== TEST 5 : PUISSANCE/ÉNERGIE ==="));
  
  Serial.println(F("Mesure puissance et calcul énergie (10 sec)..."));
  Serial.println();
  
  energyStartTime = millis();
  totalEnergy12V = 0.0f;
  totalEnergy5V = 0.0f;
  
  unsigned long lastTime = energyStartTime;
  
  while (millis() - energyStartTime < 10000) {
    unsigned long now = millis();
    unsigned long deltaTime = now - lastTime;
    
    if (deltaTime >= 1000) {
      sensor12V.forceUpdate();
      sensor5V.forceUpdate();
      
      float energy12 = sensor12V.getEnergy(deltaTime);
      float energy5 = sensor5V.getEnergy(deltaTime);
      
      totalEnergy12V += energy12;
      totalEnergy5V += energy5;
      
      float totalPower = sensor12V.getPower() + sensor5V.getPower();
      
      Serial.print(F("["));
      Serial.print((now - energyStartTime) / 1000);
      Serial.print(F("s] "));
      
      Serial.print(F("12V: "));
      Serial.print(sensor12V.getPower(), 2);
      Serial.print(F("W | "));
      
      Serial.print(F("5V: "));
      Serial.print(sensor5V.getPower(), 2);
      Serial.print(F("W | "));
      
      Serial.print(F("Total: "));
      Serial.print(totalPower, 2);
      Serial.println(F("W"));
      
      lastTime = now;
    }
    
    delay(10);
  }
  
  Serial.println();
  Serial.println(F("Énergie consommée :"));
  Serial.print(F("  Rail 12V : "));
  Serial.print(totalEnergy12V * 1000.0f, 3);
  Serial.println(F(" mWh"));
  
  Serial.print(F("  Rail 5V  : "));
  Serial.print(totalEnergy5V * 1000.0f, 3);
  Serial.println(F(" mWh"));
  
  Serial.print(F("  Total    : "));
  Serial.print((totalEnergy12V + totalEnergy5V) * 1000.0f, 3);
  Serial.println(F(" mWh"));
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 6 : Comparaison
 */
void testComparaison() {
  Serial.println(F("=== TEST 6 : COMPARAISON 12V vs 5V ==="));
  
  Serial.println(F("Comparaison des deux rails..."));
  Serial.println();
  
  sensor12V.forceUpdate();
  sensor5V.forceUpdate();
  
  // Barres de progression tension
  Serial.println(F("Tension (% de nominal) :"));
  Serial.print(F("  12V : "));
  printProgressBar(sensor12V.getBusVoltage(), 12.0f);
  Serial.print(F("  5V  : "));
  printProgressBar(sensor5V.getBusVoltage(), 5.0f);
  Serial.println();
  
  // Barres de progression courant
  Serial.println(F("Courant (% de max) :"));
  Serial.print(F("  12V : "));
  printProgressBar(sensor12V.getCurrent(), RAIL_12V_CURRENT);
  Serial.print(F("  5V  : "));
  printProgressBar(sensor5V.getCurrent(), RAIL_5V_CURRENT);
  Serial.println();
  
  // Comparaison puissance
  float power12 = sensor12V.getPower();
  float power5 = sensor5V.getPower();
  float totalPower = power12 + power5;
  
  Serial.println(F("Répartition de puissance :"));
  Serial.print(F("  12V : "));
  Serial.print(power12, 2);
  Serial.print(F(" W ("));
  Serial.print((power12 / totalPower) * 100.0f, 0);
  Serial.println(F("%)"));
  
  Serial.print(F("  5V  : "));
  Serial.print(power5, 2);
  Serial.print(F(" W ("));
  Serial.print((power5 / totalPower) * 100.0f, 0);
  Serial.println(F("%)"));
  
  Serial.print(F("  Total : "));
  Serial.print(totalPower, 2);
  Serial.println(F(" W"));
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 7 : Monitoring continu
 */
void testMonitoring() {
  Serial.println(F("=== TEST 7 : MONITORING CONTINU ===="));
  
  Serial.println(F("Mode monitoring actif"));
  Serial.println(F("Envoyez 'q' pour quitter"));
  Serial.println();
  
  monitoringActive = true;
  testMode = false;
  
  Serial.println(F("Format : [12V] V/A/W | [5V] V/A/W | Statut"));
  Serial.println();
}

/**
 * @brief Test 8 : Configuration
 */
void testConfiguration() {
  Serial.println(F("=== TEST 8 : CONFIGURATION ==="));
  
  Serial.println(F("Plateforme : Arduino Mega R3"));
  Serial.println(F("Configuration I2C :"));
  Serial.println(F("  SDA : Pin 20 (fixe)"));
  Serial.println(F("  SCL : Pin 21 (fixe)"));
  Serial.println();
  
  Serial.println(F("Capteur 12V :"));
  Serial.print(F("  Adresse I2C : 0x"));
  Serial.println(sensor12V.getI2CAddress(), HEX);
  Serial.print(F("  État : "));
  Serial.println(INA226Sensor::statusToString(sensor12V.getStatus()));
  Serial.println();
  
  Serial.println(F("Capteur 5V :"));
  Serial.print(F("  Adresse I2C : 0x"));
  Serial.println(sensor5V.getI2CAddress(), HEX);
  Serial.print(F("  État : "));
  Serial.println(INA226Sensor::statusToString(sensor5V.getStatus()));
  Serial.println();
  
  Serial.println(F("Résistance shunt : 100 mΩ (0.1Ω)"));
  Serial.println(F("Courant max      : 3.2 A"));
  
  Serial.println(F("\n✓ Affichage terminé"));
}

// ============================================
// AFFICHAGE MONITORING
// ============================================
/**
 * @brief Affiche les données en mode monitoring
 */
void printMonitoring() {
  static unsigned long lastDisplay = 0;
  unsigned long now = millis();
  
  if (now - lastDisplay >= 1000) {
    lastDisplay = now;
    
    sensor12V.update();
    sensor5V.update();
    
    // Format compact
    Serial.print(F("[12V] "));
    Serial.print(sensor12V.getBusVoltage(), 2);
    Serial.print(F("V/"));
    Serial.print(sensor12V.getCurrent(), 3);
    Serial.print(F("A/"));
    Serial.print(sensor12V.getPower(), 2);
    Serial.print(F("W | "));
    
    Serial.print(F("[5V] "));
    Serial.print(sensor5V.getBusVoltage(), 2);
    Serial.print(F("V/"));
    Serial.print(sensor5V.getCurrent(), 3);
    Serial.print(F("A/"));
    Serial.print(sensor5V.getPower(), 2);
    Serial.print(F("W | "));
    
    // Statut global
    if (sensor12V.isHealthy() && sensor5V.isHealthy()) {
      Serial.println(F("✓"));
    } else {
      Serial.print(F("⚠ "));
      if (!sensor12V.isHealthy()) {
        Serial.print(F("12V:"));
        Serial.print(INA226Sensor::statusToString(sensor12V.getStatus()));
        Serial.print(F(" "));
      }
      if (!sensor5V.isHealthy()) {
        Serial.print(F("5V:"));
        Serial.print(INA226Sensor::statusToString(sensor5V.getStatus()));
      }
      Serial.println();
    }
  }
  
  // Quitter monitoring
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'q' || c == 'Q') {
      monitoringActive = false;
      testMode = true;
      Serial.println();
      Serial.println(F("Monitoring arrêté"));
      printMenu();
    }
    while (Serial.available()) Serial.read();
  }
}
