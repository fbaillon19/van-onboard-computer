/**
 * @file test_mpu6050.ino
 * @brief Programme de TEST du MPU6050 (vérification fonctionnement)
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-21
 * 
 * @details
 * Ce programme teste uniquement le BON FONCTIONNEMENT du capteur :
 * - Connexion I2C
 * - Lecture des valeurs
 * - Stabilité des mesures
 * - Réactivité aux mouvements
 * 
 * La calibration finale se fera dans le système complet avec LCD.
 * 
 * Connexions :
 * - VCC  → 3.3V ou 5V
 * - GND  → GND
 * - SCL  → Pin 21 (Arduino MEGA)
 * - SDA  → Pin 20 (Arduino MEGA)
 */

#include "MPU6050Sensor.h"

// ============================================
// CONFIGURATION
// ============================================
#define SERIAL_BAUD     115200

// ============================================
// VARIABLES GLOBALES
// ============================================
MPU6050Sensor mpuSensor;
unsigned long lastDisplay = 0;


// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);  // Initialisation série
  delay(3000);
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   TEST MPU6050 - Vérification HW       ║"));
  Serial.println(F("║   Version 0.1.0                        ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  // Initialiser I2C
  Serial.println(F("Initialisation I2C..."));
  Wire.begin();
  Wire.setClock(400000);
  Serial.println(F("✓ I2C initialisé\n"));
  
  // Scanner I2C
  scanI2C();
  
  // Init capteur
  Serial.println(F("Initialisation MPU6050..."));
  if (!mpuSensor.begin()) {
    Serial.println(F("❌ ÉCHEC: MPU6050 non détecté"));
    Serial.println(F("   Vérifier câblage et alimentation"));
    while (1) delay(1000);
  }
  
  Serial.println(F("✓ MPU6050 initialisé\n"));
  
  // Menu test
  printMenu();
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  // Lecture capteur
  if (mpuSensor.update()) {
    // Affichage toutes les secondes
    if (millis() - lastDisplay >= 1000) {
      lastDisplay = millis();
      displayValues();
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
// GESTION DES COMMANDES
// ============================================
/**
 * @brief Gère les commandes
 */
void handleCommand(char cmd) {
  Serial.println();
  
  switch (cmd) {
    case '1':
      Serial.println(F("→ Mode affichage continu"));
      break;
      
    case '2':
      testStability();
      break;
      
    case '3':
      testResponsiveness();
      break;
      
    case '4':
      calculateTestOffsets();
      break;
      
    case 's':
    case 'S':
      scanI2C();
      break;
      
    case 'h':
    case 'H':
      printMenu();
      break;
      
    case 'r':
    case 'R':
      Serial.println(F("→ Redémarrage..."));
      delay(500);
      asm volatile ("jmp 0");
      break;
      
    default:
      Serial.println(F("Commande inconnue (H pour aide)"));
  }
  
  Serial.println();
}

// ============================================
// FONCTIONS AFFICHAGE
// ============================================
/**
 * @brief Affiche le menu des tests
 */
void printMenu() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      MENU TEST MPU6050                 ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("1 - Affichage continu (défaut)"));
  Serial.println(F("2 - Test stabilité (30s)"));
  Serial.println(F("3 - Test réactivité (mouvement)"));
  Serial.println(F("4 - Calcul offsets (référence)"));
  Serial.println(F("S - Scanner I2C"));
  Serial.println(F("H - Afficher cette aide"));
  Serial.println(F("R - Redémarrer\n"));
  Serial.println();
  Serial.println(F("Tapez une commande..."));
  Serial.println();
}

/**
 * @brief Affiche les valeurs du capteur
 */
void displayValues() {
  Serial.println(F("┌─────────────────────────────────────┐"));
  Serial.println(F("│      Mesures MPU6050 (brutes)      │"));
  Serial.println(F("├─────────────────────────────────────┤"));
  
  Serial.print(F("│ Roll (X):     "));
  printFloat(mpuSensor.getRawRoll(), 2);
  Serial.println(F("°              │"));
  
  Serial.print(F("│ Pitch (Y):    "));
  printFloat(mpuSensor.getRawPitch(), 2);
  Serial.println(F("°              │"));
  
  Serial.print(F("│ Yaw (Z):      "));
  printFloat(mpuSensor.getYaw(), 2);
  Serial.println(F("°              │"));
  
  Serial.print(F("│ Inclin. tot:  "));
  printFloat(mpuSensor.getTotalTilt(), 2);
  Serial.println(F("°              │"));
  
  Serial.print(F("│ Température:  "));
  printFloat(mpuSensor.getTemperature(), 1);
  Serial.println(F("°C             │"));
  
  Serial.println(F("└─────────────────────────────────────┘\n"));
}

/**
 * @brief Affiche un float avec padding
 */
void printFloat(float value, uint8_t decimals) {
  if (value >= 0) Serial.print(F(" "));
  if (abs(value) < 10) Serial.print(F(" "));
  if (abs(value) < 100) Serial.print(F(" "));
  Serial.print(value, decimals);
}

// ============================================
// FONCTIONS DE TEST
// ============================================
/**
 * @brief Test de stabilité : mesure dérive sur 30s
 */
void testStability() {
  Serial.println(F("\n=== TEST STABILITÉ ==="));
  Serial.println(F("Mesure de la dérive sur 30 secondes..."));
  Serial.println(F("Ne pas bouger le capteur !\n"));
  
  delay(2000);
  
  float minRoll = 999, maxRoll = -999;
  float minPitch = 999, maxPitch = -999;
  float sumRoll = 0, sumPitch = 0;
  uint16_t samples = 0;
  
  unsigned long startTime = millis();
  
  while (millis() - startTime < 30000) {
    mpuSensor.forceUpdate();
    
    float roll = mpuSensor.getRawRoll();
    float pitch = mpuSensor.getRawPitch();
    
    minRoll = min(minRoll, roll);
    maxRoll = max(maxRoll, roll);
    minPitch = min(minPitch, pitch);
    maxPitch = max(maxPitch, pitch);
    
    sumRoll += roll;
    sumPitch += pitch;
    samples++;
    
    // Afficher progression
    if (samples % 50 == 0) {
      Serial.print(F("."));
    }
    
    delay(100);
  }
  
  Serial.println(F("\n\n--- Résultats ---"));
  Serial.print(F("Échantillons: ")); Serial.println(samples);
  
  Serial.println(F("\nRoll (X):"));
  Serial.print(F("  Min:    ")); Serial.print(minRoll, 2); Serial.println(F("°"));
  Serial.print(F("  Max:    ")); Serial.print(maxRoll, 2); Serial.println(F("°"));
  Serial.print(F("  Moyen:  ")); Serial.print(sumRoll/samples, 2); Serial.println(F("°"));
  Serial.print(F("  Dérive: ")); Serial.print(maxRoll - minRoll, 2); Serial.println(F("°"));
  
  Serial.println(F("\nPitch (Y):"));
  Serial.print(F("  Min:    ")); Serial.print(minPitch, 2); Serial.println(F("°"));
  Serial.print(F("  Max:    ")); Serial.print(maxPitch, 2); Serial.println(F("°"));
  Serial.print(F("  Moyen:  ")); Serial.print(sumPitch/samples, 2); Serial.println(F("°"));
  Serial.print(F("  Dérive: ")); Serial.print(maxPitch - minPitch, 2); Serial.println(F("°"));
  
  Serial.println(F("\n✓ Dérive < 1° = BON"));
  Serial.println(F("⚠ Dérive > 2° = Vérifier fixation ou capteur défectueux"));
  Serial.println();
}

/**
 * @brief Test réactivité : demande de bouger le capteur
 */
void testResponsiveness() {
  Serial.println(F("\n=== TEST RÉACTIVITÉ ==="));
  Serial.println(F("Incliner le capteur dans différentes directions"));
  Serial.println(F("Observer que les valeurs changent rapidement\n"));
  Serial.println(F("Appuyer sur une touche pour arrêter...\n"));
  
  delay(2000);
  
  while (!Serial.available()) {
    mpuSensor.forceUpdate();
    
    Serial.print(F("Roll: "));
    printFloat(mpuSensor.getRawRoll(), 1);
    Serial.print(F("°  |  Pitch: "));
    printFloat(mpuSensor.getRawPitch(), 1);
    Serial.print(F("°  |  Total: "));
    printFloat(mpuSensor.getTotalTilt(), 1);
    Serial.println(F("°"));
    
    delay(100);
  }
  
  while (Serial.available()) Serial.read();
  Serial.println(F("\n✓ Test terminé\n"));
}

// ============================================
// FONCTIONS UTILITAIRES
// ============================================
/**
 * @brief Calcul d'offsets (pour référence uniquement)
 */
void calculateTestOffsets() {
  Serial.println(F("\n=== CALCUL OFFSETS (référence) ==="));
  Serial.println(F("⚠️  Ce n'est PAS la calibration finale !"));
  Serial.println(F("⚠️  Juste pour tester la méthode de calcul\n"));
  Serial.println(F("Placer le capteur à plat"));
  Serial.println(F("Démarrage dans 3 secondes...\n"));
  
  delay(3000);
  
  float rollOffset, pitchOffset;
  
  Serial.println(F("Calcul en cours..."));
  
  // Utiliser callback pour afficher progression
  bool success = mpuSensor.calculateOffsets(
    200, 
    rollOffset, 
    pitchOffset,
    [](uint16_t current, uint16_t total) {
      if (current % 20 == 0) {
        Serial.print(F("."));
      }
    }
  );
  
  Serial.println();
  
  if (success) {
    Serial.println(F("\n✓ Calcul réussi\n"));
    Serial.println(F("Offsets calculés (pour info):"));
    Serial.print(F("  Roll:  ")); Serial.print(rollOffset, 2); Serial.println(F("°"));
    Serial.print(F("  Pitch: ")); Serial.print(pitchOffset, 2); Serial.println(F("°"));
    Serial.println(F("\nℹ Ces valeurs sont justes pour TEST."));
    Serial.println(F("  La calibration finale se fera dans le véhicule"));
    Serial.println(F("  avec le système complet (LCD).\n"));
  } else {
    Serial.println(F("\n❌ ÉCHEC: Valeurs aberrantes détectées"));
    Serial.println(F("   Le capteur n'est probablement pas à plat\n"));
  }
}

/**
 * @brief Scanner I2C
 */
void scanI2C() {
  Serial.println(F("=== Scan bus I2C ==="));
  byte count = 0;
  
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  0x"));
      if (addr < 16) Serial.print(F("0"));
      Serial.print(addr, HEX);
      if (addr == 0x68) Serial.print(F(" - MPU6050 ✓"));
      Serial.println();
      count++;
    }
  }
  
  Serial.print(F("\nTotal: ")); Serial.print(count);
  Serial.println(F(" périphérique(s)\n"));
}
