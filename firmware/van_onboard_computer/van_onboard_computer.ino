/**
 * @file van_onboard_computer.ino
 * @brief Programme principal VOBC - Van Onboard Computer
 * @version 0.1.0
 * @date $(date +%Y-%m-%d)
 * 
 * Système de monitoring et gestion pour fourgon aménagé
 * Contrôleur: Arduino MEGA 2560
 */

#include "config.h"

// ============================================
// INCLUDES DES LIBRAIRIES
// ============================================
#include <Wire.h>

// TODO: Décommenter au fur et à mesure de l'implémentation
// #include <Adafruit_BME280.h>
// #include <INA226.h>
// #include <MPU6050_tockn.h>
// #include <OneWire.h>
// #include <DallasTemperature.h>
// #include <LiquidCrystal_I2C.h>

// ============================================
// VARIABLES GLOBALES
// ============================================
unsigned long lastUpdate = 0;
bool systemInitialized = false;

// ============================================
// SETUP - INITIALISATION
// ============================================
void setup() {
  // Initialiser communication série
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && millis() < 3000); // Attendre 3s max
  
  Serial.println(F("========================================"));
  Serial.println(F("   VOBC - Van Onboard Computer"));
  Serial.print(F("   Version: ")); Serial.println(VOBC_VERSION);
  Serial.print(F("   Build: ")); Serial.print(VOBC_BUILD_DATE);
  Serial.print(F(" ")); Serial.println(VOBC_BUILD_TIME);
  Serial.println(F("========================================"));
  Serial.println();
  
  // Initialiser bus I2C
  Serial.println(F("Initialisation I2C..."));
  Wire.begin();
  Wire.setClock(100000); // 100kHz (standard)
  
  // Scanner I2C (debug)
  if (DEBUG_MODE) {
    scanI2C();
  }
  
  // TODO: Initialiser capteurs
  // initBME280();
  // initINA226();
  // initMPU6050();
  // initLCD();
  // etc.
  
  // TODO: Initialiser actionneurs
  // initBuzzer();
  // initLED();
  
  // TODO: Initialiser encodeur
  // initEncoder();
  
  Serial.println(F("\n✓ Système initialisé avec succès!"));
  Serial.println(F("========================================\n"));
  
  systemInitialized = true;
}

// ============================================
// LOOP - BOUCLE PRINCIPALE
// ============================================
void loop() {
  if (!systemInitialized) return;
  
  unsigned long currentMillis = millis();
  
  // TODO: Lecture capteurs selon intervals
  // TODO: Vérification seuils d'alerte
  // TODO: Mise à jour affichage
  // TODO: Gestion encodeur/menu
  // TODO: Logging SD si activé
  
  // Placeholder - à remplacer
  if (currentMillis - lastUpdate >= 5000) {
    lastUpdate = currentMillis;
    Serial.println(F("System running... (placeholder)"));
  }
  
  // Petit délai pour ne pas surcharger le CPU
  delay(10);
}

// ============================================
// FONCTIONS UTILITAIRES
// ============================================

/**
 * @brief Scanner les périphériques I2C présents
 */
void scanI2C() {
  Serial.println(F("\nScan du bus I2C..."));
  byte count = 0;
  
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  Périphérique trouvé à l'adresse 0x"));
      if (addr < 16) Serial.print(F("0"));
      Serial.print(addr, HEX);
      
      // Identifier le périphérique si possible
      switch(addr) {
        case 0x27:
        case 0x3F:
          Serial.print(F(" - LCD I2C"));
          break;
        case 0x40:
          Serial.print(F(" - INA226 Batterie"));
          break;
        case 0x41:
          Serial.print(F(" - INA226 Solaire"));
          break;
        case 0x68:
          Serial.print(F(" - MPU6050 ou RTC DS3231"));
          break;
        case 0x76:
        case 0x77:
          Serial.print(F(" - BME280"));
          break;
        case 0x1E:
          Serial.print(F(" - HMC5883L"));
          break;
      }
      Serial.println();
      count++;
    }
  }
  
  Serial.print(F("\nTotal: "));
  Serial.print(count);
  Serial.println(F(" périphérique(s) I2C trouvé(s)\n"));
}
