/**
 * @file SensorManager.h
 * @brief Gestionnaire centralisé de tous les capteurs
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-26
 * 
 * @details
 * Classe responsable de :
 * - Initialisation de tous les capteurs
 * - Acquisition non-bloquante avec intervalles individuels
 * - Mise à jour des données dans SystemState
 * - Gestion du pré-chauffage MQ7/MQ2
 * - Détection des capteurs présents sur I2C
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "SystemData.h"

// Inclusion des classes capteurs
#include "BME280Sensor.h"
#include "DS18B20Sensor.h"
#include "MPU6050Sensor.h"
#include "MQ7Sensor.h"
#include "MQ2Sensor.h"
#include "INA226Sensor.h"

// ============================================
// CLASSE SensorManager
// ============================================
/**
 * @class SensorManager
 * @brief Gestionnaire centralisé de tous les capteurs
 * 
 * @details
 * Cette classe orchestre l'acquisition de tous les capteurs
 * de manière non-bloquante et met à jour l'état système.
 */
class SensorManager {
private:
  // Instances des capteurs
  BME280Sensor* bme280;
  DS18B20Sensor* ds18b20;
  MPU6050Sensor* mpu6050;
  MQ7Sensor* mq7;
  MQ2Sensor* mq2;
  INA226Sensor* ina226_12v;
  INA226Sensor* ina226_5v;
  
  // Référence à l'état système
  SystemState& state;
  
  // Timing pré-chauffage
  unsigned long preheatStartTime;
  bool preheatComplete;
  
  // Flags d'initialisation
  bool initialized;
  
public:
  /**
   * @brief Constructeur
   * @param sysState Référence à l'état système
   */
  SensorManager(SystemState& sysState) 
    : state(sysState),
      bme280(nullptr),
      ds18b20(nullptr),
      mpu6050(nullptr),
      mq7(nullptr),
      mq2(nullptr),
      ina226_12v(nullptr),
      ina226_5v(nullptr),
      preheatStartTime(0),
      preheatComplete(false),
      initialized(false)
  {
  }
  
  /**
   * @brief Destructeur - libère la mémoire
   */
  ~SensorManager() {
    if (bme280) delete bme280;
    if (ds18b20) delete ds18b20;
    if (mpu6050) delete mpu6050;
    if (mq7) delete mq7;
    if (mq2) delete mq2;
    if (ina226_12v) delete ina226_12v;
    if (ina226_5v) delete ina226_5v;
  }
  
  // ============================================
  // INITIALISATION
  // ============================================
  
  /**
   * @brief Initialise tous les capteurs
   * @return true si au moins un capteur initialisé
   * 
   * @details
   * - Scanne le bus I2C
   * - Initialise chaque capteur
   * - Met à jour SensorStatus
   * - Démarre le pré-chauffage MQ7/MQ2
   */
  bool begin() {
    DEBUG_PRINTLN(F("=== INITIALISATION CAPTEURS ==="));
    
    // Scanner I2C pour détecter périphériques
    scanI2C();
    
    // Initialiser BME280 (température/humidité intérieur)
    bme280 = new BME280Sensor(I2C_BME280, INTERVAL_BME280);
    if (bme280->begin()) {
      state.sensors.bme280 = true;
      DEBUG_PRINTLN(F("[OK] BME280 initialise"));
    } else {
      DEBUG_PRINTLN(F("[ECHEC] BME280 non detecte"));
      state.sensors.bme280 = false;
    }
    
    // Initialiser DS18B20 (température extérieur)
    ds18b20 = new DS18B20Sensor(PIN_DS18B20, INTERVAL_DS18B20);
    if (ds18b20->begin()) {
      state.sensors.ds18b20 = true;
      DEBUG_PRINTF("[OK] DS18B20 initialise (%d capteur(s))\n", ds18b20->getSensorCount());
    } else {
      DEBUG_PRINTLN(F("[ECHEC] DS18B20 non detecte"));
      state.sensors.ds18b20 = false;
    }
    
    // Initialiser MPU6050 (horizontalité)
    mpu6050 = new MPU6050Sensor(Wire, INTERVAL_MPU6050);
    if (mpu6050->begin()) {
      state.sensors.mpu6050 = true;
      DEBUG_PRINTLN(F("[OK] MPU6050 initialise"));
    } else {
      DEBUG_PRINTLN(F("[ECHEC] MPU6050 non detecte"));
      state.sensors.mpu6050 = false;
    }
    
    // Initialiser MQ7 (CO)
    mq7 = new MQ7Sensor(PIN_MQ7, INTERVAL_MQ7);
    mq7->begin();
    state.sensors.mq7 = true;
    DEBUG_PRINTLN(F("[OK] MQ7 initialise (pre-chauffe requise)"));
    
    // Initialiser MQ2 (GPL/fumée)
    mq2 = new MQ2Sensor(PIN_MQ2, INTERVAL_MQ2);
    mq2->begin();
    state.sensors.mq2 = true;
    DEBUG_PRINTLN(F("[OK] MQ2 initialise (pre-chauffe requise)"));
    
    // Initialiser INA226 12V
    ina226_12v = new INA226Sensor(PowerRailType::RAIL_12V, I2C_INA226_12V, INTERVAL_INA226);
    if (ina226_12v->begin()) {
      ina226_12v->setVoltageThresholds(VOLTAGE_12V_MIN, VOLTAGE_12V_MAX);
      ina226_12v->setCurrentThreshold(CURRENT_12V_MAX);
      state.sensors.ina226_12v = true;
      DEBUG_PRINTLN(F("[OK] INA226 12V initialise"));
    } else {
      DEBUG_PRINTLN(F("[ECHEC] INA226 12V non detecte"));
      state.sensors.ina226_12v = false;
    }
    
    // Initialiser INA226 5V
    ina226_5v = new INA226Sensor(PowerRailType::RAIL_5V, I2C_INA226_5V, INTERVAL_INA226);
    if (ina226_5v->begin()) {
      ina226_5v->setVoltageThresholds(VOLTAGE_5V_MIN, VOLTAGE_5V_MAX);
      ina226_5v->setCurrentThreshold(CURRENT_5V_MAX);
      state.sensors.ina226_5v = true;
      DEBUG_PRINTLN(F("[OK] INA226 5V initialise"));
    } else {
      DEBUG_PRINTLN(F("[ECHEC] INA226 5V non detecte"));
      state.sensors.ina226_5v = false;
    }
    
    // Démarrer pré-chauffage capteurs gaz
    preheatStartTime = millis();
    state.preheatStartTime = preheatStartTime;
    state.mode = SystemMode::MODE_PREHEAT;
    
    DEBUG_PRINTLN(F("=== PRE-CHAUFFE EN COURS ==="));
    DEBUG_PRINTF("MQ7: %d secondes\n", PREHEAT_MQ7_TIME / 1000);
    DEBUG_PRINTF("MQ2: %d secondes\n", PREHEAT_MQ2_TIME / 1000);
    
    initialized = true;
    return (state.sensors.bme280 || state.sensors.ds18b20 || 
            state.sensors.mpu6050 || state.sensors.ina226_12v || 
            state.sensors.ina226_5v);
  }
  
  /**
   * @brief Scanne le bus I2C et affiche les périphériques détectés
   */
  void scanI2C() {
    DEBUG_PRINTLN(F("Scan bus I2C..."));
    byte count = 0;
    
    for (byte addr = 1; addr < 127; addr++) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0) {
        DEBUG_PRINTF("  -> 0x%02X detecte\n", addr);
        count++;
      }
    }
    
    if (count == 0) {
      DEBUG_PRINTLN(F("Aucun peripherique I2C detecte!"));
    } else {
      DEBUG_PRINTF("Total: %d peripherique(s) I2C\n", count);
    }
  }
  
  // ============================================
  // MISE À JOUR
  // ============================================
  
  /**
   * @brief Met à jour tous les capteurs (non-bloquant)
   * 
   * @details
   * Appelle update() sur chaque capteur.
   * Chaque capteur gère son propre intervalle.
   * Met à jour SystemState avec les nouvelles données.
   */
  void update() {
    if (!initialized) return;
    
    // Gérer pré-chauffage
    updatePreheat();
    
    // Mettre à jour chaque capteur (non-bloquant)
    updateBME280();
    updateDS18B20();
    updateMPU6050();
    updateMQ7();
    updateMQ2();
    updateINA226();
  }
  
  /**
   * @brief Gère le pré-chauffage des capteurs MQ
   */
  void updatePreheat() {
    if (preheatComplete) return;
    
    unsigned long elapsed = millis() - preheatStartTime;
    
    // Vérifier MQ2 (1 minute)
    if (!state.safety.mq2Preheated && elapsed >= PREHEAT_MQ2_TIME) {
      state.safety.mq2Preheated = true;
      DEBUG_PRINTLN(F("[OK] MQ2 pre-chauffe terminee"));
    }
    
    // Vérifier MQ7 (3 minutes)
    if (!state.safety.mq7Preheated && elapsed >= PREHEAT_MQ7_TIME) {
      state.safety.mq7Preheated = true;
      DEBUG_PRINTLN(F("[OK] MQ7 pre-chauffe terminee"));
    }
    
    // Pré-chauffage complet ?
    if (state.safety.mq7Preheated && state.safety.mq2Preheated) {
      preheatComplete = true;
      state.mode = SystemMode::MODE_NORMAL;
      DEBUG_PRINTLN(F("=== PRE-CHAUFFE COMPLETE ==="));
      DEBUG_PRINTLN(F("Mode NORMAL active"));
    }
  }
  
  /**
   * @brief Met à jour BME280 (température/humidité intérieur)
   */
  void updateBME280() {
    if (!state.sensors.bme280 || !bme280) return;
    
    if (bme280->update()) {
      BME280Data data = bme280->getData();
      
      state.environment.tempInterior = data.temperature;
      state.environment.humidity = data.humidity;
      state.environment.pressure = data.pressure;
      state.environment.dewPoint = bme280->getDewPoint();
      state.environment.tempIntTimestamp = data.timestamp;
      
      state.environment.tempIntValid = isValidTemperature(data.temperature);
      state.environment.humidityValid = isValidHumidity(data.humidity);
      state.environment.pressureValid = (data.pressure > 900 && data.pressure < 1100);
    }
  }
  
  /**
   * @brief Met à jour DS18B20 (température extérieur)
   */
  void updateDS18B20() {
    if (!state.sensors.ds18b20 || !ds18b20) return;
    
    if (ds18b20->update()) {
      float temp = ds18b20->getTemperature(0);
      state.environment.tempExterior = temp;
      state.environment.tempExtTimestamp = millis();
      state.environment.tempExtValid = isValidTemperature(temp);
    }
  }
  
  /**
   * @brief Met à jour MPU6050 (horizontalité)
   */
  void updateMPU6050() {
    if (!state.sensors.mpu6050 || !mpu6050) return;
    
    if (mpu6050->update()) {
      state.level.roll = mpu6050->getRoll();
      state.level.pitch = mpu6050->getPitch();
      state.level.yaw = mpu6050->getYaw();
      state.level.rawRoll = mpu6050->getRawRoll();
      state.level.rawPitch = mpu6050->getRawPitch();
      state.level.totalTilt = mpu6050->getTotalTilt();
      state.level.temperature = mpu6050->getTemperature();
      state.level.timestamp = millis();
      state.level.valid = true;
    }
  }
  
  /**
   * @brief Met à jour MQ7 (CO)
   */
  void updateMQ7() {
    if (!state.sensors.mq7 || !mq7) return;
    
    if (mq7->update()) {
      state.safety.coPPM = mq7->getPPM();
      state.safety.coTimestamp = millis();
      state.safety.coValid = state.safety.mq7Preheated && isValidPPM(state.safety.coPPM);
    }
  }
  
  /**
   * @brief Met à jour MQ2 (GPL/fumée)
   */
  void updateMQ2() {
    if (!state.sensors.mq2 || !mq2) return;
    
    if (mq2->update()) {
      state.safety.gplPPM = mq2->getLPG();
      state.safety.smokePPM = mq2->getSmoke();
      state.safety.gplTimestamp = millis();
      state.safety.smokeTimestamp = millis();
      state.safety.gplValid = state.safety.mq2Preheated && isValidPPM(state.safety.gplPPM);
      state.safety.smokeValid = state.safety.mq2Preheated && isValidPPM(state.safety.smokePPM);
    }
  }
  
  /**
   * @brief Met à jour INA226 (12V et 5V)
   */
  void updateINA226() {
    // INA226 12V
    if (state.sensors.ina226_12v && ina226_12v) {
      if (ina226_12v->update()) {
        INA226Data data = ina226_12v->getData();
        state.power.voltage12V = data.busVoltage;
        state.power.current12V = data.current;
        state.power.power12V = data.power;
        state.power.voltage12VTimestamp = data.timestamp;
        state.power.voltage12VValid = data.valid && isValidVoltage(data.busVoltage);
      }
    }
    
    // INA226 5V
    if (state.sensors.ina226_5v && ina226_5v) {
      if (ina226_5v->update()) {
        INA226Data data = ina226_5v->getData();
        state.power.voltage5V = data.busVoltage;
        state.power.current5V = data.current;
        state.power.power5V = data.power;
        state.power.voltage5VTimestamp = data.timestamp;
        state.power.voltage5VValid = data.valid && isValidVoltage(data.busVoltage);
      }
    }
    
    // Calculer puissance totale
    state.power.powerTotal = state.power.power12V + state.power.power5V;
  }
  
  // ============================================
  // CALIBRATION MPU6050
  // ============================================
  
  /**
   * @brief Lance la calibration du MPU6050
   * @param progressCallback Fonction callback pour progression
   * @return true si succès
   */
  bool calibrateMPU6050(void (*progressCallback)(uint16_t current, uint16_t total) = nullptr) {
    if (!state.sensors.mpu6050 || !mpu6050) return false;
    
    DEBUG_PRINTLN(F("Calibration MPU6050..."));
    
    float rollOffset, pitchOffset;
    
    if (mpu6050->calculateOffsets(MPU6050_CALIBRATION_SAMPLES, rollOffset, pitchOffset, progressCallback)) {
      mpu6050->setOffsets(rollOffset, pitchOffset);
      state.level.calibrated = true;
      
      DEBUG_PRINTF("Offsets calcules: Roll=%.2f, Pitch=%.2f\n", rollOffset, pitchOffset);
      return true;
    }
    
    DEBUG_PRINTLN(F("Echec calibration MPU6050"));
    return false;
  }
  
  /**
   * @brief Récupère les offsets MPU6050
   * @param roll [out] Offset Roll
   * @param pitch [out] Offset Pitch
   */
  void getMPU6050Offsets(float &roll, float &pitch) {
    if (mpu6050) {
      mpu6050->getOffsets(roll, pitch);
    }
  }
  
  /**
   * @brief Définit les offsets MPU6050
   * @param roll Offset Roll
   * @param pitch Offset Pitch
   */
  void setMPU6050Offsets(float roll, float pitch) {
    if (mpu6050) {
      mpu6050->setOffsets(roll, pitch);
      state.level.calibrated = true;
    }
  }
  
  // ============================================
  // GETTERS
  // ============================================
  
  /**
   * @brief Vérifie si le pré-chauffage est terminé
   * @return true si terminé
   */
  bool isPreheatComplete() const {
    return preheatComplete;
  }
  
  /**
   * @brief Obtient le temps restant de pré-chauffage (ms)
   * @return Temps restant en ms (0 si terminé)
   */
  unsigned long getPreheatTimeRemaining() const {
    if (preheatComplete) return 0;
    
    unsigned long elapsed = millis() - preheatStartTime;
    unsigned long maxTime = max(PREHEAT_MQ7_TIME, PREHEAT_MQ2_TIME);
    
    if (elapsed >= maxTime) return 0;
    return maxTime - elapsed;
  }
  
  /**
   * @brief Obtient le pourcentage de pré-chauffage
   * @return Pourcentage (0-100)
   */
  uint8_t getPreheatPercent() const {
    if (preheatComplete) return 100;
    
    unsigned long elapsed = millis() - preheatStartTime;
    unsigned long maxTime = max(PREHEAT_MQ7_TIME, PREHEAT_MQ2_TIME);
    
    return (elapsed * 100) / maxTime;
  }
  
  /**
   * @brief Vérifie si les capteurs sont initialisés
   * @return true si initialisés
   */
  bool isInitialized() const {
    return initialized;
  }
  
  /**
   * @brief Compte le nombre de capteurs actifs
   * @return Nombre de capteurs fonctionnels
   */
  uint8_t getActiveSensorCount() const {
    uint8_t count = 0;
    if (state.sensors.bme280) count++;
    if (state.sensors.ds18b20) count++;
    if (state.sensors.mpu6050) count++;
    if (state.sensors.mq7) count++;
    if (state.sensors.mq2) count++;
    if (state.sensors.ina226_12v) count++;
    if (state.sensors.ina226_5v) count++;
    return count;
  }
};

#endif // SENSOR_MANAGER_H
