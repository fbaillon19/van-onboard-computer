/**
 * @file test_mpu6050.ino
 * @brief Programme de test MPU6050 - Validation mesure d'horizontalité
 * @author Frédéric BAILLON
 * @version 2.0.0
 * @date 2024-12-05
 * 
 * @details
 * Test de validation matérielle pour mesure d'horizontalité du van.
 * 
 * Philosophie : Simple et focalisé sur le besoin du projet
 * - Validation : "Est-ce que ça fonctionne pour mon besoin ?"
 * - Confiance : "Puis-je l'intégrer dans le projet ?"
 * 
 * Tests effectués :
 * 1. Détection I2C
 * 2. Calibration sur plan horizontal
 * 3. Mesure de stabilité (dérive < 2°)
 * 4. Réactivité aux mouvements
 * 5. Simulation alertes horizontalité (±2°, ±5°)
 * 
 * Résultat attendu :
 * ✓ Angles proches de 0° sur surface plane
 * ✓ Dérive < 2° sur 30 secondes
 * ✓ Réaction immédiate aux inclinaisons
 * ✓ Détection fiable des seuils ±2° et ±5°
 * 
 * Connexions :
 * - VCC  → 3.3V ou 5V
 * - GND  → GND
 * - SCL  → Pin 21 (Arduino MEGA)
 * - SDA  → Pin 20 (Arduino MEGA)
 * - AD0  → GND (adresse 0x68)
 */

#include "MPU6050Sensor.h"

// ============================================
// CONFIGURATION
// ============================================
#define SERIAL_BAUD     115200

// ============================================
// VARIABLES GLOBALES
// ============================================
MPU6050Sensor mpu;
bool testMode = true;

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(3000);
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   TEST MPU6050 - Vérification Hardware ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  // Initialiser I2C
  Serial.println(F("Initialisation I2C..."));
  Wire.begin();
  Wire.setClock(400000);
  Serial.println(F("✓ I2C @ 400kHz\n"));
  
  // Scanner I2C
  scanI2C();
  
  // Initialiser capteur
  Serial.println(F("Initialisation MPU6050..."));
  if (!mpu.begin()) {
    Serial.println(F("❌ ÉCHEC : MPU6050 non détecté"));
    Serial.println(F("   Vérifier câblage (VCC, GND, SDA, SCL, AD0)"));
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println(F("✓ MPU6050 initialisé\n"));
  
  // Instructions calibration
  Serial.println(F("═══ CALIBRATION ═══"));
  Serial.println(F("Place le capteur sur un plan PARFAITEMENT horizontal"));
  Serial.println(F("(utilise un niveau à bulle si nécessaire)"));
  Serial.println();
  Serial.println(F("Quand prêt, tape 'C' pour calibrer"));
  
  // Attendre commande calibration
  while (true) {
    if (Serial.available()) {
      char ch = Serial.read();
      if (ch == 'c' || ch == 'C') {
        Serial.println(F("\nCalibration en cours (800 échantillons)..."));
        mpu.calibrateLevel(800);
        
        float offsetX, offsetY;
        mpu.getOffsets(offsetX, offsetY);
        
        Serial.println(F("✓ Calibration terminée"));
        Serial.print(F("  Offset X : "));
        Serial.print(offsetX, 2);
        Serial.println(F("°"));
        Serial.print(F("  Offset Y : "));
        Serial.print(offsetY, 2);
        Serial.println(F("°"));
        Serial.println();
        Serial.println(F("Angles de référence = 0°/0° sur cette position"));
        break;
      }
    }
  }
  
  delay(1000);
  printMenu();
}

// ============================================
// LOOP
// ============================================
void loop() {
  // Mise à jour capteur
  mpu.update();
  
  // Gestion commandes
  if (Serial.available()) {
    handleCommand(Serial.read());
    while (Serial.available()) Serial.read();
  }
  
  delay(10);
}

// ============================================
// GESTION DES COMMANDES
// ============================================
/**
 * @brief Gestion commandes
 */
void handleCommand(char cmd) {
  switch (cmd) {
    case '1':
      testMonitoring();
      printMenu();
      break;
    case '2':
      testStability();
      printMenu();
      break;
    case '3':
      testReactivity();
      printMenu();
      break;
    case '4':
      testAlerts();
      printMenu();
      break;
    case 'c':
    case 'C':
      recalibrate();
      printMenu();
      break;
    case 'h':
    case 'H':
      printMenu();
      break;
    default:
      Serial.print(F("Commande inconnue : "));
      break;
  }
}

// ============================================
// FONCTIONS AFFICHAGE
// ============================================
/**
 * @brief Affiche menu
 */
void printMenu() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      MENU TEST MPU6050                 ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("  1 - Mesure continue (monitoring)"));
  Serial.println(F("  2 - Test stabilité (dérive 30s)"));
  Serial.println(F("  3 - Test réactivité (mouvements)"));
  Serial.println(F("  4 - Simulation alertes (±2°/±5°)"));
  Serial.println(F("  C - Recalibration"));
  Serial.println(F("  H - Afficher cette aide"));
  Serial.println();
}

// ============================================
// FONCTIONS DE TEST
// ============================================
/**
 * @brief Test 1 : Mesure continue
 */
void testMonitoring() {
  Serial.println(F("\n═══ TEST 1 : MESURE CONTINUE ═══"));
  Serial.println(F("Affichage en temps réel des angles"));
  Serial.println(F("Tapez 'q' pour quitter"));
  
  while (true) {
    mpu.update();
    
    float angleX = round(mpu.getAngleX() * 10) / 10.0;
    float angleY = round(mpu.getAngleY() * 10) / 10.0;
    
    Serial.print(F("Pitch (X) : "));
    Serial.print(angleX, 1);
    Serial.print(F("°\t"));
    
    Serial.print(F("Roll (Y) : "));
    Serial.print(angleY, 1);
    Serial.println(F("°"));
    
    // Quitter monitoring
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'q' || c == 'Q') {
        Serial.println();
        Serial.println(F("✓ Test terminé"));
        while (Serial.available()) Serial.read();
        break;
      }
    }
    
    delay(250);
  }
}

/**
 * @brief Test 2 : Stabilité (dérive)
 */
void testStability() {
  Serial.println(F("\n═══ TEST 2 : STABILITÉ ═══"));
  Serial.println(F("Mesure de la dérive sur 30 secondes"));
  Serial.println(F("Ne PAS bouger le capteur !\n"));
  
  float minX = 999, maxX = -999, sumX = 0;
  float minY = 999, maxY = -999, sumY = 0;
  uint16_t samples = 0;
  
  unsigned long startTime = millis();
  
  while (millis() - startTime < 30000) {
    mpu.update();
    
    float angleX = mpu.getAngleX();
    float angleY = mpu.getAngleY();
    
    if (angleX < minX) minX = angleX;
    if (angleX > maxX) maxX = angleX;
    sumX += angleX;
    
    if (angleY < minY) minY = angleY;
    if (angleY > maxY) maxY = angleY;
    sumY += angleY;
    
    samples++;
    
    // Indicateur progression
    if (samples % 30 == 0) {
      Serial.print(F("."));
    }
    
    delay(100);
  }
  
  Serial.println();
  Serial.println(F("\n--- Résultats ---"));
  Serial.print(F("Échantillons : "));
  Serial.println(samples);
  Serial.println();
  
  Serial.println(F("Pitch (X) :"));
  Serial.print(F("  Min    : "));
  Serial.print(minX, 2);
  Serial.println(F("°"));
  Serial.print(F("  Max    : "));
  Serial.print(maxX, 2);
  Serial.println(F("°"));
  Serial.print(F("  Moyen  : "));
  Serial.print(sumX / samples, 2);
  Serial.println(F("°"));
  Serial.print(F("  Dérive : "));
  Serial.print(maxX - minX, 2);
  Serial.println(F("°"));
  Serial.println();
  
  Serial.println(F("Roll (Y) :"));
  Serial.print(F("  Min    : "));
  Serial.print(minY, 2);
  Serial.println(F("°"));
  Serial.print(F("  Max    : "));
  Serial.print(maxY, 2);
  Serial.println(F("°"));
  Serial.print(F("  Moyen  : "));
  Serial.print(sumY / samples, 2);
  Serial.println(F("°"));
  Serial.print(F("  Dérive : "));
  Serial.print(maxY - minY, 2);
  Serial.println(F("°"));
  Serial.println();
  
  // Évaluation
  float deriveX = maxX - minX;
  float deriveY = maxY - minY;
  
  if (deriveX < 1.0 && deriveY < 1.0) {
    Serial.println(F("✓ EXCELLENT : Dérive < 1°"));
  } else if (deriveX < 2.0 && deriveY < 2.0) {
    Serial.println(F("✓ BON : Dérive < 2°"));
  } else {
    Serial.println(F("⚠ ATTENTION : Dérive > 2°"));
    Serial.println(F("  Vérifier fixation ou qualité capteur"));
  }
  
  Serial.println();
}

/**
 * @brief Test 3 : Réactivité
 */
void testReactivity() {
  Serial.println(F("\n═══ TEST 3 : RÉACTIVITÉ ═══"));
  Serial.println(F("Incline le capteur dans différentes directions"));
  Serial.println(F("Les angles doivent changer immédiatement"));
  Serial.println(F("Tapez 'q' pour quitter"));
  
  while (true) {
    mpu.update();
    
    float angleX = round(mpu.getAngleX() * 10) / 10.0;
    float angleY = round(mpu.getAngleY() * 10) / 10.0;
    
    // Affichage avec indicateurs visuels
    Serial.print(F("Pitch : "));
    printAngleBar(angleX);
    Serial.print(F("  "));
    Serial.print(angleX, 1);
    Serial.print(F("°\t"));
    
    Serial.print(F("Roll : "));
    printAngleBar(angleY);
    Serial.print(F("  "));
    Serial.print(angleY, 1);
    Serial.println(F("°"));
    
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'q' || c == 'Q') {
        Serial.println();
        Serial.println(F("✓ Test terminé"));
        while (Serial.available()) Serial.read();
        break;
      }
    }
    
    delay(200);
  }
}

/**
 * @brief Test 4 : Simulation alertes
 */
void testAlerts() {
  Serial.println(F("\n═══ TEST 4 : SIMULATION ALERTES ═══"));
  Serial.println(F("Teste les seuils d'alerte du van"));
  Serial.println(F("  ±2° = Alerte ORANGE (attention)"));
  Serial.println(F("  ±5° = Alerte ROUGE (danger)"));
  Serial.println(F("Tapez 'q' pour quitter"));
  
  while (true) {
    mpu.update();
    
    float angleX = mpu.getAngleX();
    float angleY = mpu.getAngleY();
    float totalTilt = sqrt(angleX * angleX + angleY * angleY);
    
    Serial.print(F("Inclinaison totale : "));
    Serial.print(totalTilt, 1);
    Serial.print(F("° "));
    
    // Déterminer niveau d'alerte
    if (totalTilt < 2.0) {
      Serial.println(F("✓ VERT (OK)"));
    } else if (totalTilt < 5.0) {
      Serial.println(F("⚠ ORANGE (Attention)"));
    } else {
      Serial.println(F("❌ ROUGE (Danger)"));
    }
    
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'q' || c == 'Q') {
        Serial.println();
        Serial.println(F("✓ Test terminé"));
        while (Serial.available()) Serial.read();
        break;
      }
    }
    
    delay(500);
  }
}

// ============================================
// FONCTIONS UTILITAIRES
// ============================================
/**
 * @brief Recalibration
 */
void recalibrate() {
  Serial.println(F("\n═══ RECALIBRATION ═══"));
  Serial.println(F("Replace le capteur à l'horizontal"));
  Serial.println(F("Appuie sur ENTRÉE quand prêt..."));
  
  while (!Serial.available()) delay(10);
  while (Serial.available()) Serial.read();
  
  Serial.println(F("Calibration en cours..."));
  mpu.calibrateLevel(800);
  
  float offsetX, offsetY;
  mpu.getOffsets(offsetX, offsetY);
  
  Serial.println(F("✓ Calibration terminée"));
  Serial.print(F("  Nouveaux offsets X : "));
  Serial.print(offsetX, 2);
  Serial.println(F("°"));
  Serial.print(F("  Nouveaux offsets Y : "));
  Serial.print(offsetY, 2);
  Serial.println(F("°\n"));
}

/**
 * @brief Scan I2C
 */
void scanI2C() {
  Serial.println(F("=== Scan bus I2C ==="));
  
  uint8_t count = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  0x"));
      if (addr < 16) Serial.print(F("0"));
      Serial.print(addr, HEX);
      
      if (addr == 0x68) {
        Serial.println(F(" - MPU6050 ✓"));
      } else {
        Serial.println();
      }
      count++;
    }
  }
  
  Serial.println();
  Serial.print(F("Total : "));
  Serial.print(count);
  Serial.println(F(" périphérique(s)\n"));
}

/**
 * @brief Affiche barre visuelle d'angle
 */
void printAngleBar(float angle) {
  // Barre de -10° à +10°
  int pos = (int)((angle + 10) * 2);  // 0 à 40
  if (pos < 0) pos = 0;
  if (pos > 40) pos = 40;
  
  Serial.print(F("["));
  for (int i = 0; i < 40; i++) {
    if (i == 20) {
      Serial.print(F("|"));  // Centre (0°)
    } else if (i == pos) {
      Serial.print(F("█"));  // Position actuelle
    } else {
      Serial.print(F(" "));
    }
  }
  Serial.print(F("]"));
}
