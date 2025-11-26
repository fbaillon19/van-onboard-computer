/**
 * @file van_onboard_computer.ino
 * @brief Programme principal Van Onboard Computer
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-26
 * 
 * @details
 * Système de monitoring pour van aménagé (Renault Trafic)
 * Plateforme : Arduino Mega R3
 * 
 * Fonctionnalités :
 * - Surveillance environnement (températures, humidité, pression)
 * - Surveillance électrique (12V, 5V, courants, puissance)
 * - Détection gaz dangereux (CO, GPL, fumée)
 * - Horizontalité (inclinomètre MPU6050)
 * - Alertes hiérarchisées avec buzzer
 * - Affichage LCD 20x4 + Navigation encodeur
 * - Bandeau LED WS2812B (8 LEDs)
 * 
 * Modules :
 * - SensorManager : Acquisition capteurs
 * - AlertSystem : Gestion alertes
 * - LEDManager : Affichage LEDs
 * - DisplayManager : Affichage LCD + Navigation
 * 
 * @warning Priorité absolue à la sécurité (CO, GPL)
 * @note Pré-chauffage requis : MQ7 (3 min), MQ2 (1 min)
 */

// ============================================
// INCLUDES
// ============================================
#include <Wire.h>
#include "config.h"
#include "SystemData.h"
#include "SensorManager.h"
#include "AlertSystem.h"
#include "LEDManager.h"
#include "DisplayManager.h"

// ============================================
// ÉTAT SYSTÈME GLOBAL
// ============================================
SystemState systemState;

// ============================================
// GESTIONNAIRES
// ============================================
SensorManager* sensorManager = nullptr;
AlertSystem* alertSystem = nullptr;
LEDManager* ledManager = nullptr;
DisplayManager* displayManager = nullptr;

// ============================================
// TIMING
// ============================================
unsigned long loopStartTime = 0;
unsigned long loopCount = 0;
unsigned long lastStatsDisplay = 0;

// ============================================
// SETUP - INITIALISATION
// ============================================
void setup() {
  // Initialiser Serial pour debug
  #if USE_SERIAL_DEBUG
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial && millis() < 3000); // Attendre max 3s

  Serial.println();
  Serial.println(F("===================================="));
  Serial.println(F("   VAN ONBOARD COMPUTER v" FIRMWARE_VERSION));
  Serial.println(F("   (c) 2024 Frederic BAILLON"));
  Serial.println(F("===================================="));
  Serial.println();
  #endif
  
  // Initialiser I2C
  Wire.begin();
  Wire.setClock(400000); // I2C Fast Mode (400 kHz)
  
  DEBUG_PRINTLN(F("I2C initialise (400 kHz)"));
  delay(100);
  
  // Initialiser état système
  initSystemState(systemState);
  systemState.uptime = 0;
  
  DEBUG_PRINTLN(F("Etat systeme initialise"));
  
  // ====================================
  // INITIALISATION GESTIONNAIRES
  // ====================================
  // 1. DisplayManager (premier pour afficher progression)
  DEBUG_PRINTLN(F("\n--- Initialisation Display ---"));
  displayManager = new DisplayManager(systemState);
  if (!displayManager->begin()) {
    DEBUG_PRINTLN(F("[ERREUR] Echec initialisation Display"));
  }
  delay(1000); // Laisser temps de lire écran boot
  
  // 2. SensorManager
  DEBUG_PRINTLN(F("\n--- Initialisation Capteurs ---"));
  if (systemState.sensors.lcd) {
    displayManager->showMessage("Init capteurs...", 0);
  }
  
  sensorManager = new SensorManager(systemState);
  if (!sensorManager->begin()) {
    DEBUG_PRINTLN(F("[ERREUR] Echec initialisation Capteurs"));
    if (systemState.sensors.lcd) {
      displayManager->showMessage("ERREUR CAPTEURS!", 3000);
    }
  }
  
  // 3. AlertSystem
  DEBUG_PRINTLN(F("\n--- Initialisation Alertes ---"));
  alertSystem = new AlertSystem(systemState);
  if (!alertSystem->begin()) {
    DEBUG_PRINTLN(F("[ERREUR] Echec initialisation Alertes"));
  }
  
  // 4. LEDManager
  DEBUG_PRINTLN(F("\n--- Initialisation LEDs ---"));
  ledManager = new LEDManager(systemState);
  if (!ledManager->begin()) {
    DEBUG_PRINTLN(F("[ERREUR] Echec initialisation LEDs"));
  }
  
  // Animation démarrage
  if (systemState.sensors.leds) {
    ledManager->bootAnimation();
  }
  
  // ====================================
  // RÉCAPITULATIF INITIALISATION
  // ====================================
  DEBUG_PRINTLN(F("\n===================================="));
  DEBUG_PRINTLN(F("RECAPITULATIF INITIALISATION"));
  DEBUG_PRINTLN(F("===================================="));
  
  uint8_t sensorCount = 0;
  
  DEBUG_PRINT(F("BME280 (temp/hum int): "));
  DEBUG_PRINTLN(systemState.sensors.bme280 ? F("OK") : F("ABSENT"));
  if (systemState.sensors.bme280) sensorCount++;
  
  DEBUG_PRINT(F("DS18B20 (temp ext):    "));
  DEBUG_PRINTLN(systemState.sensors.ds18b20 ? F("OK") : F("ABSENT"));
  if (systemState.sensors.ds18b20) sensorCount++;
  
  DEBUG_PRINT(F("MPU6050 (niveau):      "));
  DEBUG_PRINTLN(systemState.sensors.mpu6050 ? F("OK") : F("ABSENT"));
  if (systemState.sensors.mpu6050) sensorCount++;
  
  DEBUG_PRINT(F("MQ7 (CO):              "));
  DEBUG_PRINTLN(systemState.sensors.mq7 ? F("OK") : F("ABSENT"));
  if (systemState.sensors.mq7) sensorCount++;
  
  DEBUG_PRINT(F("MQ2 (GPL/fumee):       "));
  DEBUG_PRINTLN(systemState.sensors.mq2 ? F("OK") : F("ABSENT"));
  if (systemState.sensors.mq2) sensorCount++;
  
  DEBUG_PRINT(F("INA226 12V:            "));
  DEBUG_PRINTLN(systemState.sensors.ina226_12v ? F("OK") : F("ABSENT"));
  if (systemState.sensors.ina226_12v) sensorCount++;
  
  DEBUG_PRINT(F("INA226 5V:             "));
  DEBUG_PRINTLN(systemState.sensors.ina226_5v ? F("OK") : F("ABSENT"));
  if (systemState.sensors.ina226_5v) sensorCount++;
  
  DEBUG_PRINT(F("LCD 20x4:              "));
  DEBUG_PRINTLN(systemState.sensors.lcd ? F("OK") : F("ABSENT"));
  
  DEBUG_PRINT(F("Encodeur KY040:        "));
  DEBUG_PRINTLN(systemState.sensors.encoder ? F("OK") : F("ABSENT"));
  
  DEBUG_PRINT(F("LEDs WS2812B:          "));
  DEBUG_PRINTLN(systemState.sensors.leds ? F("OK") : F("ABSENT"));
  
  DEBUG_PRINT(F("Buzzer:                "));
  DEBUG_PRINTLN(systemState.sensors.buzzer ? F("OK") : F("ABSENT"));
  
  DEBUG_PRINTLN(F("===================================="));
  DEBUG_PRINTF("Total capteurs actifs: %d/7\n", sensorCount);
  DEBUG_PRINTLN(F("====================================\n"));
  
  // Vérifier qu'au moins les capteurs critiques sont présents
  if (!systemState.sensors.mq7 || !systemState.sensors.mq2) {
    DEBUG_PRINTLN(F("[ALERTE] Capteurs gaz manquants - Securite compromise!"));
    if (systemState.sensors.lcd) {
      displayManager->showMessage("ALERTE: GAZ!", 3000);
    }
  }
  
  // Mode pré-chauffage
  if (systemState.sensors.mq7 || systemState.sensors.mq2) {
    DEBUG_PRINTLN(F("\n=== DEMARRAGE PRE-CHAUFFE ==="));
    DEBUG_PRINTLN(F("MQ7: 180 secondes"));
    DEBUG_PRINTLN(F("MQ2: 60 secondes"));
    DEBUG_PRINTLN(F("Merci de patienter..."));
    DEBUG_PRINTLN(F("================================\n"));
  }
  
  // Système prêt
  systemState.initialized = true;
  
  DEBUG_PRINTLN(F("=== SYSTEME PRET ===\n"));
  
  // Marquer temps de démarrage
  loopStartTime = millis();
}

// ============================================
// LOOP - BOUCLE PRINCIPALE
// ============================================
void loop() {
  unsigned long loopStart = millis();
  
  // ====================================
  // 1. ACQUISITION CAPTEURS
  // ====================================
  // Chaque capteur gère son propre intervalle
  if (sensorManager) {
    sensorManager->update();
  }
  
  // ====================================
  // 2. VÉRIFICATION ALERTES
  // ====================================
  // PRIORITÉ ABSOLUE - Vérifié à chaque cycle
  if (alertSystem) {
    alertSystem->checkAlerts();
    alertSystem->updateBuzzer();
  }
  
  // ====================================
  // 3. MISE À JOUR LEDS
  // ====================================
  if (ledManager) {
    // Animation spéciale pendant pré-chauffage
    if (systemState.mode == SystemMode::MODE_PREHEAT && sensorManager) {
      uint8_t percent = sensorManager->getPreheatPercent();
      ledManager->preheatAnimation(percent);
    } else {
      ledManager->update();
    }
  }
  
  // ====================================
  // 4. MISE À JOUR AFFICHAGE
  // ====================================
  if (displayManager) {
    displayManager->update();
    displayManager->handleScreenTimeout();
  }
  
  // ====================================
  // 5. GESTION CALIBRATION MPU6050
  // ====================================
  if (systemState.calibrationMode) {
    performMPU6050Calibration();
  }
  
  // ====================================
  // 6. MISE À JOUR UPTIME
  // ====================================
  systemState.uptime = millis() / 1000; // En secondes
  
  // ====================================
  // 7. STATISTIQUES DEBUG (toutes les 10s)
  // ====================================
  #if USE_SERIAL_DEBUG
  if (millis() - lastStatsDisplay >= 10000) {
    lastStatsDisplay = millis();
    displayStats();
  }
  #endif
  
  // ====================================
  // 8. WATCHDOG / MONITORING LOOP
  // ====================================
  loopCount++;
  unsigned long loopDuration = millis() - loopStart;
  
  // Avertir si loop trop lent (>100ms)
  if (loopDuration > 100) {
    DEBUG_PRINTF("[WARNING] Loop lent: %lu ms\n", loopDuration);
  }
  
  // Petit délai pour stabilité (optionnel)
  // delay(1); // Décommenter si nécessaire
}

// ============================================
// FONCTIONS AUXILIAIRES
// ============================================
/**
 * @brief Effectue la calibration du MPU6050
 * 
 * @details
 * - Affiche instructions sur LCD
 * - Effectue 100 mesures
 * - Calcule et applique offsets
 * - Affiche résultat
 */
void performMPU6050Calibration() {
  systemState.calibrationMode = false; // Reset flag
  
  if (!systemState.sensors.mpu6050 || !sensorManager) {
    if (displayManager) {
      displayManager->showMessage("MPU6050 absent!", 2000);
    }
    return;
  }
  
  DEBUG_PRINTLN(F("\n=== CALIBRATION MPU6050 ==="));
  DEBUG_PRINTLN(F("Placez le van a plat et immobile"));
  DEBUG_PRINTLN(F("Calibration en cours..."));
  
  // Afficher message LCD
  if (displayManager && systemState.sensors.lcd) {
    displayManager->showMessage("Calibration MPU...", 0);
    delay(1000);
    displayManager->showMessage("Van a plat!", 2000);
  }
  
  // Callback progression
  auto progressCallback = [](uint16_t current, uint16_t total) {
    if (current % 10 == 0) { // Afficher tous les 10 échantillons
      DEBUG_PRINTF("  %d/%d\n", current, total);
    }
  };
  
  // Lancer calibration
  bool success = sensorManager->calibrateMPU6050(progressCallback);
  
  if (success) {
    DEBUG_PRINTLN(F("Calibration reussie!"));
    
    float roll, pitch;
    sensorManager->getMPU6050Offsets(roll, pitch);
    DEBUG_PRINTF("Offsets: Roll=%.2f, Pitch=%.2f\n", roll, pitch);
    
    if (displayManager) {
      displayManager->showMessage("Calibration OK!", 2000);
    }
  } else {
    DEBUG_PRINTLN(F("Echec calibration!"));
    
    if (displayManager) {
      displayManager->showMessage("Echec calib!", 2000);
    }
  }
  
  DEBUG_PRINTLN(F("=========================\n"));
  
  // Retour écran précédent
  if (displayManager) {
    displayManager->forceRefresh();
  }
}

/**
 * @brief Affiche les statistiques système sur Serial
 */
void displayStats() {
  DEBUG_PRINTLN(F("\n===================================="));
  DEBUG_PRINTLN(F("STATISTIQUES SYSTEME"));
  DEBUG_PRINTLN(F("===================================="));
  
  // Uptime
  unsigned long uptimeSeconds = systemState.uptime;
  unsigned long hours = uptimeSeconds / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;
  DEBUG_PRINTF("Uptime: %02lu:%02lu:%02lu\n", hours, minutes, seconds);
  
  // Mode
  DEBUG_PRINT(F("Mode: "));
  DEBUG_PRINTLN(systemModeToString(systemState.mode));
  
  // Ecran
  DEBUG_PRINT(F("Ecran: "));
  DEBUG_PRINTLN(screenToString(systemState.currentScreen));
  
  // Environnement
  DEBUG_PRINTLN(F("\n--- ENVIRONNEMENT ---"));
  if (systemState.environment.tempIntValid) {
    DEBUG_PRINTF("Temp int: %.1f C\n", systemState.environment.tempInterior);
  }
  if (systemState.environment.tempExtValid) {
    DEBUG_PRINTF("Temp ext: %.1f C\n", systemState.environment.tempExterior);
  }
  if (systemState.environment.humidityValid) {
    DEBUG_PRINTF("Humidite: %.0f %%\n", systemState.environment.humidity);
  }
  if (systemState.environment.pressureValid) {
    DEBUG_PRINTF("Pression: %.0f hPa\n", systemState.environment.pressure);
  }
  
  // Puissance
  DEBUG_PRINTLN(F("\n--- ENERGIE ---"));
  if (systemState.power.voltage12VValid) {
    DEBUG_PRINTF("12V: %.2f V - %.2f A - %.1f W\n",
                 systemState.power.voltage12V,
                 systemState.power.current12V,
                 systemState.power.power12V);
  }
  if (systemState.power.voltage5VValid) {
    DEBUG_PRINTF("5V: %.2f V - %.2f A - %.1f W\n",
                 systemState.power.voltage5V,
                 systemState.power.current5V,
                 systemState.power.power5V);
  }
  DEBUG_PRINTF("Total: %.1f W\n", systemState.power.powerTotal);
  
  // Sécurité
  DEBUG_PRINTLN(F("\n--- SECURITE ---"));
  if (systemState.safety.mq7Preheated) {
    DEBUG_PRINTF("CO:    %.0f ppm", systemState.safety.coPPM);
    if (systemState.safety.coPPM > CO_THRESHOLD_DANGER) {
      DEBUG_PRINTLN(F(" [DANGER]"));
    } else if (systemState.safety.coPPM > CO_THRESHOLD_WARNING) {
      DEBUG_PRINTLN(F(" [WARNING]"));
    } else {
      DEBUG_PRINTLN(F(" [OK]"));
    }
  } else {
    DEBUG_PRINTLN(F("CO:    [PRE-CHAUFFE]"));
  }
  
  if (systemState.safety.mq2Preheated) {
    DEBUG_PRINTF("GPL:   %.0f ppm", systemState.safety.gplPPM);
    if (systemState.safety.gplPPM > GPL_THRESHOLD_DANGER) {
      DEBUG_PRINTLN(F(" [DANGER]"));
    } else if (systemState.safety.gplPPM > GPL_THRESHOLD_WARNING) {
      DEBUG_PRINTLN(F(" [WARNING]"));
    } else {
      DEBUG_PRINTLN(F(" [OK]"));
    }
    
    DEBUG_PRINTF("Fumee: %.0f ppm", systemState.safety.smokePPM);
    if (systemState.safety.smokePPM > SMOKE_THRESHOLD_DANGER) {
      DEBUG_PRINTLN(F(" [DANGER]"));
    } else if (systemState.safety.smokePPM > SMOKE_THRESHOLD_WARNING) {
      DEBUG_PRINTLN(F(" [WARNING]"));
    } else {
      DEBUG_PRINTLN(F(" [OK]"));
    }
  } else {
    DEBUG_PRINTLN(F("GPL:   [PRE-CHAUFFE]"));
    DEBUG_PRINTLN(F("Fumee: [PRE-CHAUFFE]"));
  }
  
  // Horizontalité
  DEBUG_PRINTLN(F("\n--- HORIZONTALITE ---"));
  if (systemState.level.valid) {
    DEBUG_PRINTF("Roll:  %+.1f deg\n", systemState.level.roll);
    DEBUG_PRINTF("Pitch: %+.1f deg\n", systemState.level.pitch);
    DEBUG_PRINTF("Total: %.1f deg", systemState.level.totalTilt);
    if (systemState.level.totalTilt > TILT_DANGER) {
      DEBUG_PRINTLN(F(" [DANGER]"));
    } else if (systemState.level.totalTilt > TILT_WARNING) {
      DEBUG_PRINTLN(F(" [WARNING]"));
    } else {
      DEBUG_PRINTLN(F(" [OK]"));
    }
  }
  
  // Alertes
  DEBUG_PRINTLN(F("\n--- ALERTES ---"));
  DEBUG_PRINTF("Niveau: %s\n", alertLevelToString(systemState.alerts.currentLevel));
  DEBUG_PRINTF("Actives: %d\n", systemState.alerts.activeAlertCount);
  
  if (systemState.alerts.activeAlertCount > 0) {
    DEBUG_PRINTLN(F("Liste:"));
    for (uint8_t i = 0; i < systemState.alerts.activeAlertCount; i++) {
      Alert alert = systemState.alerts.alerts[i];
      DEBUG_PRINTF("  %d. %s - %s (%.1f)\n",
                   i + 1,
                   alertTypeToShortString(alert.type),
                   alertLevelToString(alert.level),
                   alert.value);
    }
  }
  
  // Performance
  DEBUG_PRINTLN(F("\n--- PERFORMANCE ---"));
  DEBUG_PRINTF("Loops: %lu\n", loopCount);
  unsigned long avgLoopTime = (millis() - loopStartTime) / loopCount;
  DEBUG_PRINTF("Temps loop moyen: %lu ms\n", avgLoopTime);
  DEBUG_PRINTF("RAM libre: %d bytes\n", freeRam());
  
  DEBUG_PRINTLN(F("====================================\n"));
}

/**
 * @brief Calcule la RAM libre disponible
 * @return Bytes de RAM libre
 */
int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
