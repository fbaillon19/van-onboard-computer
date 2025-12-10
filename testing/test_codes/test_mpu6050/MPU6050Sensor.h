/**
 * @file MPU6050Sensor.h
 * @brief Classe de gestion du capteur MPU6050 pour mesure d'horizontalité
 * @author Frédéric BAILLON
 * @version 2.0.0 - Simplification : accéléromètre uniquement
 * @date 2024-12-05
 * 
 * @details
 * Version simplifiée et pragmatique pour mesure d'horizontalité statique.
 * 
 * Philosophie de conception :
 * - Utilise UNIQUEMENT l'accéléromètre (pas le gyroscope)
 * - Pas de dérive thermique
 * - Calibration simple sur plan horizontal
 * - Filtre passe-bas exponentiel pour stabilité
 * 
 * Pourquoi accéléromètre seul ?
 * - Mesure la gravité directement
 * - Aucune dérive dans le temps
 * - Parfait pour usage statique (van immobile)
 * - Plus simple = plus fiable
 * 
 * Usage :
 * @code
 * MPU6050Sensor mpu;
 * 
 * void setup() {
 *   mpu.begin();
 *   mpu.calibrateLevel(500);  // Capteur sur plan horizontal
 * }
 * 
 * void loop() {
 *   mpu.update();
 *   float pitch = mpu.getAngleX();  // Tangage
 *   float roll = mpu.getAngleY();   // Roulis
 * }
 * @endcode
 */

#ifndef MPU6050_SENSOR_H
#define MPU6050_SENSOR_H

#include <Arduino.h>
#include <Wire.h>

// ============================================
// CONFIGURATION
// ============================================
#define MPU6050_I2C_ADDR        0x68      ///< Adresse I2C (AD0 à GND)
#define MPU6050_REG_PWR_MGMT_1  0x6B      ///< Registre Power Management
#define MPU6050_REG_ACCEL_CONFIG 0x1C     ///< Configuration accéléromètre
#define MPU6050_REG_CONFIG      0x1A      ///< Configuration générale
#define MPU6050_REG_ACCEL_XOUT_H 0x3B     ///< Début données accéléromètre

#define MPU6050_ACCEL_SENSITIVITY 16384.0 ///< Sensibilité ±2g (LSB/g)
#define MPU6050_DEFAULT_FILTER    0.3     ///< Alpha du filtre passe-bas (0.1-0.5)

// ============================================
// TYPES ET STRUCTURES
// ============================================
/**
 * @struct MPU6050Data
 * @brief Données brutes et traitées du capteur
 */
struct MPU6050Data {
  float angleX;         ///< Angle X (pitch/tangage) en degrés
  float angleY;         ///< Angle Y (roll/roulis) en degrés
  float offsetX;        ///< Offset calibration X
  float offsetY;        ///< Offset calibration Y
  unsigned long timestamp; ///< Timestamp de la mesure
};

// ============================================
// DEFINITION CLASSE MPU6050Sensor
// ============================================
/**
 * @class MPU6050Sensor
 * @brief Gestion simplifiée du MPU6050 pour horizontalité
 * 
 * @details
 * Classe minimaliste focalisée sur le besoin réel :
 * - Mesurer l'inclinaison du van (pitch et roll)
 * - Stable à long terme (pas de dérive)
 * - Facile à calibrer
 */
class MPU6050Sensor {
private:
  uint8_t i2cAddress;       ///< Adresse I2C du capteur
  float angleX;             ///< Angle X filtré (pitch)
  float angleY;             ///< Angle Y filtré (roll)
  float lastAngleX;         ///< Dernière valeur X (pour filtre)
  float lastAngleY;         ///< Dernière valeur Y (pour filtre)
  float offsetX;            ///< Offset calibration X
  float offsetY;            ///< Offset calibration Y
  float filterAlpha;        ///< Coefficient filtre passe-bas
  bool initialized;         ///< État initialisation
  
  /**
   * @brief Écrit dans un registre
   * @param reg Adresse du registre
   * @param value Valeur à écrire
   * @return true si succès
   */
  bool writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(i2cAddress);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
  }
  
  /**
   * @brief Lit plusieurs octets
   * @param startReg Registre de départ
   * @param buffer Buffer de réception
   * @param length Nombre d'octets à lire
   * @return true si succès
   */
  bool readBytes(uint8_t startReg, uint8_t* buffer, uint8_t length) {
    Wire.beginTransmission(i2cAddress);
    Wire.write(startReg);
    if (Wire.endTransmission(false) != 0) {
      return false;
    }
    
    uint8_t read = Wire.requestFrom((int)i2cAddress, (int)length, (int)true);
    if (read != length) {
      return false;
    }
    
    for (uint8_t i = 0; i < length; i++) {
      buffer[i] = Wire.read();
    }
    return true;
  }
  
  /**
   * @brief Lit les données brutes de l'accéléromètre
   * @param ax [out] Accélération X (LSB)
   * @param ay [out] Accélération Y (LSB)
   * @param az [out] Accélération Z (LSB)
   * @return true si succès
   */
  bool readAccelRaw(int16_t &ax, int16_t &ay, int16_t &az) {
    uint8_t buffer[6];
    if (!readBytes(MPU6050_REG_ACCEL_XOUT_H, buffer, 6)) {
      return false;
    }
    
    ax = (int16_t)((buffer[0] << 8) | buffer[1]);
    ay = (int16_t)((buffer[2] << 8) | buffer[3]);
    az = (int16_t)((buffer[4] << 8) | buffer[5]);
    
    return true;
  }

public:
  /**
   * @brief Constructeur
   * @param address Adresse I2C (défaut 0x68, AD0 à GND)
   */
  MPU6050Sensor(uint8_t address = MPU6050_I2C_ADDR)
    : i2cAddress(address),
      angleX(0.0f),
      angleY(0.0f),
      lastAngleX(0.0f),
      lastAngleY(0.0f),
      offsetX(0.0f),
      offsetY(0.0f),
      filterAlpha(MPU6050_DEFAULT_FILTER),
      initialized(false)
  {}

  // INITIALISATION
  // ------------------------------------------
  /**
   * @brief Initialise le capteur MPU6050
   * @return true si succès
   * 
   * @details
   * - Réveille le capteur (sortie du mode sleep)
   * - Configure l'accéléromètre à ±2g
   * - Active le filtre passe-bas (~44Hz)
   */
  bool begin() {
    Wire.begin();
    delay(10);
    
    // Réveiller le MPU6050 (sortie du mode sleep)
    if (!writeRegister(MPU6050_REG_PWR_MGMT_1, 0x00)) {
      return false;
    }
    delay(10);
    
    // Configurer accéléromètre à ±2g
    if (!writeRegister(MPU6050_REG_ACCEL_CONFIG, 0x00)) {
      return false;
    }
    
    // Activer filtre passe-bas ~44Hz
    if (!writeRegister(MPU6050_REG_CONFIG, 0x03)) {
      return false;
    }
    
    initialized = true;
    return true;
  }

  // LECTURE DONNÉES
  // ------------------------------------------
  /**
   * @brief Met à jour les angles (à appeler dans loop())
   * 
   * @details
   * Calcule les angles pitch et roll à partir de l'accéléromètre,
   * applique les offsets de calibration et filtre le résultat.
   */
  void update() {
    if (!initialized) return;
    
    int16_t rawAx, rawAy, rawAz;
    if (!readAccelRaw(rawAx, rawAy, rawAz)) {
      return;
    }
    
    // Conversion LSB → g (sensibilité ±2g = 16384 LSB/g)
    float ax = (float)rawAx / MPU6050_ACCEL_SENSITIVITY;
    float ay = (float)rawAy / MPU6050_ACCEL_SENSITIVITY;
    float az = (float)rawAz / MPU6050_ACCEL_SENSITIVITY;
    
    // Calcul des angles à partir de la gravité
    // pitch = atan2(-ax, sqrt(ay² + az²))
    // roll  = atan2(ay, az)
    float pitchRad = atan2(-ax, sqrt(ay * ay + az * az));
    float rollRad  = atan2(ay, az);
    
    float angleXRaw = pitchRad * 180.0f / PI;
    float angleYRaw = rollRad  * 180.0f / PI;
    
    // Application des offsets
    angleXRaw -= offsetX;
    angleYRaw -= offsetY;
    
    // Filtre passe-bas exponentiel (lissage)
    // newValue = alpha * raw + (1-alpha) * oldValue
    angleX = filterAlpha * angleXRaw + (1.0f - filterAlpha) * lastAngleX;
    angleY = filterAlpha * angleYRaw + (1.0f - filterAlpha) * lastAngleY;
    
    lastAngleX = angleX;
    lastAngleY = angleY;
  }

  // CALIBRATION
  // ------------------------------------------
  /**
   * @brief Calibre le capteur sur un plan horizontal
   * @param samples Nombre d'échantillons à moyenner (défaut 500)
   * 
   * @details
   * IMPORTANT : Le capteur DOIT être parfaitement horizontal.
   * Cette méthode calcule les offsets pour que les angles
   * mesurent 0°/0° sur cette position de référence.
   * 
   * Utiliser un niveau à bulle pour positionner le capteur.
   */
  void calibrateLevel(uint16_t samples = 500) {
    if (!initialized) return;
    
    float sumX = 0.0f;
    float sumY = 0.0f;
    uint16_t validSamples = 0;
    
    for (uint16_t i = 0; i < samples; i++) {
      int16_t rawAx, rawAy, rawAz;
      if (!readAccelRaw(rawAx, rawAy, rawAz)) {
        continue;
      }
      
      float ax = (float)rawAx / MPU6050_ACCEL_SENSITIVITY;
      float ay = (float)rawAy / MPU6050_ACCEL_SENSITIVITY;
      float az = (float)rawAz / MPU6050_ACCEL_SENSITIVITY;
      
      float pitchRad = atan2(-ax, sqrt(ay * ay + az * az));
      float rollRad  = atan2(ay, az);
      
      float angleXRaw = pitchRad * 180.0f / PI;
      float angleYRaw = rollRad  * 180.0f / PI;
      
      sumX += angleXRaw;
      sumY += angleYRaw;
      validSamples++;
      
      delay(2);
    }
    
    if (validSamples == 0) return;
    
    // Les offsets sont les moyennes mesurées
    offsetX = sumX / (float)validSamples;
    offsetY = sumY / (float)validSamples;
    
    // Réinitialiser les valeurs filtrées
    lastAngleX = 0.0f;
    lastAngleY = 0.0f;
    angleX = 0.0f;
    angleY = 0.0f;
  }

  // GETTERS
  // ------------------------------------------
  /**
   * @brief Retourne l'angle X (pitch/tangage)
   * @return Angle en degrés
   */
  float getAngleX() const { return angleX; }
  
  /**
   * @brief Retourne l'angle Y (roll/roulis)
   * @return Angle en degrés
   */
  float getAngleY() const { return angleY; }
  
  /**
   * @brief Retourne les offsets de calibration
   * @param outOffsetX [out] Offset X
   * @param outOffsetY [out] Offset Y
   */
  void getOffsets(float &outOffsetX, float &outOffsetY) const {
    outOffsetX = offsetX;
    outOffsetY = offsetY;
  }
  
  /**
   * @brief Vérifie si le capteur est initialisé
   * @return true si initialisé
   */
  bool isInitialized() const { return initialized; }
  
  /**
   * @brief Retourne toutes les données
   * @return Structure MPU6050Data
   */
  MPU6050Data getData() const {
    MPU6050Data data;
    data.angleX = angleX;
    data.angleY = angleY;
    data.offsetX = offsetX;
    data.offsetY = offsetY;
    data.timestamp = millis();
    return data;
  }

  // CONFIGURATION
  // ------------------------------------------
  /**
   * @brief Ajuste le coefficient du filtre passe-bas
   * @param alpha Coefficient (0 < alpha <= 1)
   * 
   * @details
   * - alpha petit (0.1) : très stable, lent à réagir
   * - alpha grand (0.9) : très réactif, plus bruité
   * - défaut (0.3) : bon compromis
   */
  void setFilterAlpha(float alpha) {
    if (alpha > 0.0f && alpha <= 1.0f) {
      filterAlpha = alpha;
    }
  }
  
  /**
   * @brief Vérifie si inclinaison totale dépasse un seuil
   * @param threshold Seuil en degrés
   * @return true si incliné au-delà du seuil
   */
  bool isTilted(float threshold) const {
    float totalTilt = sqrt(angleX * angleX + angleY * angleY);
    return totalTilt > threshold;
  }
};

#endif // MPU6050_SENSOR_H