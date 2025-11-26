/**
 * @file MPU6050Sensor.h
 * @brief Classe de gestion du capteur MPU6050 (gyroscope + accéléromètre)
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-21
 * 
 * @details
 * Utilisable dans le programme de test et le programme principal.
 * 
 * Principes de conception :
 * - Calibration = simple calcul, pas d'interaction utilisateur
 * - Responsabilité unique : lire et traiter données capteur
 */

#ifndef MPU6050_SENSOR_H
#define MPU6050_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <MPU6050_tockn.h>


// ============================================
// CONFIGURATION MATÉRIELLE
// ============================================
#define MPU6050_I2C_ADDR          0x68

// ============================================
// TYPES ET STRUCTURES
// ============================================
/**
 * @struct MPU6050Data
 * @brief Structure contenant toutes les données du capteur
 */
struct MPU6050Data {
  float roll;          ///< Angle Roll calibré (degrés)
  float pitch;         ///< Angle Pitch calibré (degrés)
  float yaw;           ///< Angle Yaw (degrés) - optionnel
  float rollRaw;       ///< Angle Roll brut (avant calibration)
  float pitchRaw;      ///< Angle Pitch brut (avant calibration)
  float temperature;   ///< Température capteur (°C)
  float totalTilt;     ///< Inclinaison totale (magnitude)
  unsigned long timestamp; ///< Timestamp de la mesure (millis)
};

/**
 * @enum CalibrationStatus
 * @brief État de la calibration
 */
enum class CalibrationStatus {
  NOT_CALIBRATED,      ///< Pas encore calibré
  IN_PROGRESS,         ///< Calibration en cours
  SUCCESS,             ///< Calibration réussie
  FAILED_ABERRANT,     ///< Échec : valeurs aberrantes
  FAILED_UNSTABLE      ///< Échec : capteur instable
};

/**
 * @struct CalibrationResult
 * @brief Résultat d'une calibration
 */
struct CalibrationResult {
  CalibrationStatus status;   ///< État de la calibration
  float offsetRoll;           ///< Offset Roll calculé
  float offsetPitch;          ///< Offset Pitch calculé
  uint16_t validSamples;      ///< Nombre d'échantillons valides
  uint16_t totalSamples;      ///< Nombre d'échantillons total
  float standardDeviation;    ///< Écart-type (stabilité)
};

// ============================================
// DEFINITION CLASSE MPU6050Sensor
// ============================================
/**
 * @class MPU6050Sensor
 * @brief Classe de gestion du capteur MPU6050
 * 
 * Cette classe gère uniquement la logique métier du capteur.
 */
class MPU6050Sensor {
private:
#define MPU6050_SAMPLE_INTERVAL   100

  MPU6050 mpu;                    ///< Instance librairie MPU6050
  float offsetRoll;               ///< Offset calibration Roll
  float offsetPitch;              ///< Offset calibration Pitch
  unsigned long lastUpdate;       ///< Timestamp dernière MAJ
  uint16_t updateInterval;        ///< Intervalle de mise à jour (ms)
  bool initialized;               ///< État initialisation
  
  float currentRoll;              ///< Angle Roll actuel (degrés)
  float currentPitch;             ///< Angle Pitch actuel (degrés)
  float currentYaw;               ///< Angle Yaw actuel (degrés)
  float currentTemp;              ///< Température (°C)
  
  float rawRoll;                  ///< Roll brut (avant compensation)
  float rawPitch;                 ///< Pitch brut (avant compensation)

public:
  /**
   * @brief Constructeur
   * @param wire Instance I2C (défaut Wire)
   * @param interval Intervalle mise à jour en ms (défaut 100ms)
   */
  MPU6050Sensor(TwoWire &wire = Wire, uint16_t interval = MPU6050_SAMPLE_INTERVAL) : 
    mpu(wire),
    offsetRoll(0.0),
    offsetPitch(0.0),
    lastUpdate(0),
    updateInterval(interval),
    initialized(false),
    currentRoll(0.0),
    currentPitch(0.0),
    currentYaw(0.0),
    currentTemp(0.0),
    rawRoll(0.0),
    rawPitch(0.0)
  {}

  // INITIALISATION
  // ------------------------------------------
  /**
   * @brief Initialise le capteur
   * @return true si succès
   */
  bool begin() {
    // Vérifier présence I2C
    Wire.beginTransmission(MPU6050_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
      return false;
    }
    
    mpu.begin();
    initialized = true;
    return true;
  }

  /**
   * @brief Définit l'intervalle de mise à jour
   * @param interval Intervalle en millisecondes
   */
  void setUpdateInterval(uint16_t interval) {
    updateInterval = interval;
  }

  /**
   * @brief Applique des offsets de calibration
   * @param roll Offset Roll en degrés
   * @param pitch Offset Pitch en degrés
   */
  void setOffsets(float roll, float pitch) {
    offsetRoll = roll;
    offsetPitch = pitch;
  }

  /**
   * @brief Obtient les offsets actuels
   * @param roll [out] Offset Roll
   * @param pitch [out] Offset Pitch
   */
  void getOffsets(float &roll, float &pitch) const {
    roll = offsetRoll;
    pitch = offsetPitch;
  }

  // LECTURE DONNÉES
  // ------------------------------------------
  /**
   * @brief Met à jour les lectures du capteur
   * @return true si mise à jour effectuée
   * 
   * @details À appeler dans loop(). Met à jour uniquement si intervalle écoulé.
   */
  bool update() {
    if (!initialized) return false;
    
    unsigned long now = millis();
    if (now - lastUpdate < updateInterval) {
      return false;
    }
    
    lastUpdate = now;
    mpu.update();
    
    // Lecture valeurs brutes
    rawRoll = mpu.getAngleX();
    rawPitch = mpu.getAngleY();
    currentYaw = mpu.getAngleZ();
    currentTemp = mpu.getTemp();
    
    // Appliquer offsets
    currentRoll = rawRoll - offsetRoll;
    currentPitch = rawPitch - offsetPitch;
    
    return true;
  }

  /**
   * @brief Force une mise à jour immédiate (ignore intervalle)
   */
  void forceUpdate() {
    if (!initialized) return;
    mpu.update();
    rawRoll = mpu.getAngleX();
    rawPitch = mpu.getAngleY();
    currentYaw = mpu.getAngleZ();
    currentTemp = mpu.getTemp();
    currentRoll = rawRoll - offsetRoll;
    currentPitch = rawPitch - offsetPitch;
    lastUpdate = millis();
  }

  // Getters
  float getRoll() const { return currentRoll; }
  float getPitch() const { return currentPitch; }
  float getYaw() const { return currentYaw; }
  float getTemperature() const { return currentTemp; }
  float getRawRoll() const { return rawRoll; }
  float getRawPitch() const { return rawPitch; }
  bool isInitialized() const { return initialized; }

  /**
   * @brief Calcule l'inclinaison totale (magnitude)
   * @return sqrt(roll² + pitch²) en degrés
   */
  float getTotalTilt() const {
    return sqrt(currentRoll * currentRoll + currentPitch * currentPitch);
  }

  /**
   * @brief Vérifie si inclinaison > seuil
   * @param threshold Seuil en degrés
   * @return true si dépassement
   */
  bool isTilted(float threshold) const {
    return getTotalTilt() > threshold;
  }

  /**
   * @brief Calcule offsets moyens sur N échantillons
   * @param samples Nombre d'échantillons
   * @param rollOffset [out] Offset Roll calculé
   * @param pitchOffset [out] Offset Pitch calculé
   * @param progressCallback Fonction callback pour progression (optionnel)
   * @return true si succès, false si échec
   * 
   * @details
   * Méthode PURE : ne fait que calculer, n'affiche rien.
   * L'appelant décide quoi faire des résultats.
   */
  bool calculateOffsets(
    uint16_t samples,
    float &rollOffset,
    float &pitchOffset,
    void (*progressCallback)(uint16_t current, uint16_t total) = nullptr
  ) {
    if (!initialized) return false;
    
    float sumRoll = 0.0;
    float sumPitch = 0.0;
    uint16_t validSamples = 0;
    
    for (uint16_t i = 0; i < samples; i++) {
      mpu.update();
      
      float roll = mpu.getAngleX();
      float pitch = mpu.getAngleY();
      
      // Filtrer valeurs aberrantes (>45° = pas plat)
      if (abs(roll) < 45.0 && abs(pitch) < 45.0) {
        sumRoll += roll;
        sumPitch += pitch;
        validSamples++;
      }
      
      // Callback progression
      if (progressCallback) {
        progressCallback(i + 1, samples);
      }
      
      delay(10);
    }
    
    // Vérifier qualité échantillons
    if (validSamples < samples * 0.8) {
      return false; // Trop de valeurs aberrantes
    }
    
    // Calculer moyennes
    rollOffset = sumRoll / validSamples;
    pitchOffset = sumPitch / validSamples;
    
    return true;
  }
};

#endif // MPU6050_SENSOR_H
