/**
 * @file BME280Sensor.h
 * @brief Classe de gestion du capteur BME280 (température + humidité + pression)
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-21
 * 
 * @details
 * Classe découplée pour le capteur environnemental BME280 de Bosch.
 * Mesure simultanément :
 * - Température (-40°C à +85°C, précision ±1°C)
 * - Humidité relative (0-100%, précision ±3%)
 * - Pression atmosphérique (300-1100 hPa, précision ±1 hPa)
 * 
 * Peut aussi calculer l'altitude approximative à partir de la pression.
 * 
 * @note Compatible I2C (adresse 0x76 ou 0x77 selon module)
 * @warning Vérifier l'adresse I2C de votre module avant utilisation
 */

#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>


// ============================================
// CONFIGURATION MATÉRIELLE
// ============================================
#define BME280_I2C_ADDR         0x76      // ou 0x77 selon module

// ============================================
// TYPES ET STRUCTURES
// ============================================
#define BME280_SAMPLE_INTERVAL  5000      // 5 secondes

/**
 * @struct BME280Data
 * @brief Structure contenant toutes les données du capteur
 */
struct BME280Data {
  float temperature;      ///< Température en °C
  float humidity;         ///< Humidité relative en %
  float pressure;         ///< Pression atmosphérique en hPa
  float altitude;         ///< Altitude approximative en m (basée sur pression)
  unsigned long timestamp; ///< Timestamp de la mesure (millis)
};

/**
 * @enum BME280Status
 * @brief État du capteur
 */
enum class BME280Status {
  NOT_INITIALIZED,   ///< Capteur non initialisé
  READY,            ///< Capteur prêt et fonctionnel
  ERROR_NOT_FOUND,  ///< Capteur non détecté sur I2C
  ERROR_COMM        ///< Erreur de communication
};

// ============================================
// DEFINITION CLASSE BME280Sensor
// ============================================
/**
 * @class BME280Sensor
 * @brief Classe de gestion du capteur environnemental BME280
 * 
 * @details
 * Classe découplée sans dépendances d'affichage.
 * Gère l'acquisition et le traitement des données environnementales.
 * 
 * Exemple d'utilisation :
 * @code
 * BME280Sensor bme;
 * 
 * void setup() {
 *   if (bme.begin()) {
 *     Serial.println("BME280 prêt");
 *   }
 * }
 * 
 * void loop() {
 *   if (bme.update()) {
 *     BME280Data data = bme.getData();
 *     Serial.print("Temp: ");
 *     Serial.print(data.temperature);
 *     Serial.println("°C");
 *   }
 * }
 * @endcode
 */
class BME280Sensor {
private:
#define SEA_LEVEL_PRESSURE_HPA  1013.25

  Adafruit_BME280 bme;            ///< Instance librairie Adafruit
  BME280Data currentData;         ///< Données courantes
  BME280Status status;            ///< État du capteur
  unsigned long lastUpdate;       ///< Timestamp dernière lecture
  uint16_t sampleInterval;        ///< Intervalle entre lectures (ms)
  uint8_t i2cAddress;             ///< Adresse I2C du capteur
  float seaLevelPressure;         ///< Pression niveau mer pour calcul altitude
  
public:
  /**
   * @brief Constructeur
   * @param addr Adresse I2C du capteur (0x76 ou 0x77)
   * @param interval Intervalle de lecture en ms (défaut: 5000ms)
   */
  BME280Sensor(uint8_t addr = BME280_I2C_ADDR, 
               uint16_t interval = BME280_SAMPLE_INTERVAL) :
    status(BME280Status::NOT_INITIALIZED),
    lastUpdate(0),
    sampleInterval(interval),
    i2cAddress(addr),
    seaLevelPressure(SEA_LEVEL_PRESSURE_HPA)
  {
    // Initialiser structure données
    memset(&currentData, 0, sizeof(BME280Data));
  }

  // INITIALISATION
  // ------------------------------------------
  /**
   * @brief Initialise le capteur BME280
   * @return true si succès, false si échec
   * 
   * @details
   * - Vérifie la présence du capteur sur le bus I2C
   * - Configure les paramètres de mesure
   * - Active le mode normal
   */
  bool begin() {
    // Vérifier présence I2C
    Wire.beginTransmission(i2cAddress);
    if (Wire.endTransmission() != 0) {
      status = BME280Status::ERROR_NOT_FOUND;
      return false;
    }
    
    // Initialiser avec adresse spécifique
    if (!bme.begin(i2cAddress)) {
      status = BME280Status::ERROR_NOT_FOUND;
      return false;
    }
    
    // Configuration recommandée pour usage général
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,     // Mode continu
                    Adafruit_BME280::SAMPLING_X2,     // Temp oversampling
                    Adafruit_BME280::SAMPLING_X16,    // Pressure oversampling
                    Adafruit_BME280::SAMPLING_X1,     // Humidity oversampling
                    Adafruit_BME280::FILTER_X16,      // Filtre pour stabilité
                    Adafruit_BME280::STANDBY_MS_500); // Standby 500ms
    
    status = BME280Status::READY;
    
    // Première lecture pour remplir les données
    forceUpdate();
    
    return true;
  }

  // LECTURE DONNÉES
  // ------------------------------------------
  /**
   * @brief Met à jour les données du capteur
   * @return true si mise à jour effectuée, false si intervalle non écoulé
   * 
   * @details
   * Méthode non-bloquante. Retourne false si appelée trop tôt.
   * À appeler régulièrement dans loop().
   */
  bool update() {
    if (status != BME280Status::READY) return false;
    
    unsigned long now = millis();
    if (now - lastUpdate < sampleInterval) {
      return false; // Intervalle non écoulé
    }
    
    return readSensor();
  }

  /**
   * @brief Force une lecture immédiate
   * @return true si succès
   * 
   * @details
   * Ignore l'intervalle de lecture. Utile pour lecture ponctuelle.
   */
  bool forceUpdate() {
    if (status != BME280Status::READY) return false;
    lastUpdate = 0; // Reset timer
    return readSensor();
  }

  /**
   * @brief Vérifie si le capteur est initialisé
   * @return true si initialisé et prêt
   */
  bool isReady() const { 
    return status == BME280Status::READY; 
  }

  /**
   * @brief Retourne l'état du capteur
   * @return État actuel du capteur
   */
  BME280Status getStatus() const { 
    return status; 
  }

  /**
   * @brief Retourne les données courantes
   * @return Structure BME280Data complète
   */
  BME280Data getData() const {
    return currentData;
  }

  /**
   * @brief Retourne la température
   * @return Température en °C
   */
  float getTemperature() const { 
    return currentData.temperature; 
  }

  /**
   * @brief Retourne l'humidité relative
   * @return Humidité en % (0-100)
   */
  float getHumidity() const { 
    return currentData.humidity; 
  }

  /**
   * @brief Retourne la pression atmosphérique
   * @return Pression en hPa
   */
  float getPressure() const { 
    return currentData.pressure; 
  }

  /**
   * @brief Retourne l'altitude approximative
   * @return Altitude en mètres
   * 
   * @note Basée sur la pression et la pression au niveau de la mer
   * @warning Précision ±10m, varie selon météo
   */
  float getAltitude() const { 
    return currentData.altitude; 
  }

  // DÉTECTION SEUILS
  // ------------------------------------------
  /**
   * @brief Vérifie si la température dépasse un seuil haut
   * @param threshold Seuil en °C
   * @return true si température > seuil
   */
  bool isTemperatureHigh(float threshold) const {
    return currentData.temperature > threshold;
  }

  /**
   * @brief Vérifie si la température est en dessous d'un seuil bas
   * @param threshold Seuil en °C
   * @return true si température < seuil
   */
  bool isTemperatureLow(float threshold) const {
    return currentData.temperature < threshold;
  }

  /**
   * @brief Vérifie si l'humidité dépasse un seuil
   * @param threshold Seuil en %
   * @return true si humidité > seuil
   */
  bool isHumidityHigh(float threshold) const {
    return currentData.humidity > threshold;
  }

  /**
   * @brief Calcule le point de rosée
   * @return Point de rosée en °C
   * 
   * @details
   * Température à laquelle l'air doit être refroidi pour atteindre 
   * la saturation (condensation).
   * Formule Magnus-Tetens simplifiée.
   */
  float getDewPoint() const {
    float t = currentData.temperature;
    float h = currentData.humidity;
    
    // Formule Magnus-Tetens
    float a = 17.27;
    float b = 237.7;
    float alpha = ((a * t) / (b + t)) + log(h / 100.0);
    float dewPoint = (b * alpha) / (a - alpha);
    
    return dewPoint;
  }

  /**
   * @brief Calcule le risque de condensation
   * @param surfaceTemp Température de surface en °C
   * @return true si risque de condensation
   * 
   * @details
   * Compare le point de rosée à la température de surface.
   * Risque si point de rosée > température surface.
   */
  bool hasCondensationRisk(float surfaceTemp) const {
    return getDewPoint() > surfaceTemp;
  }

  // CONFIGURATION
  // ------------------------------------------
  /**
   * @brief Change l'intervalle de lecture
   * @param interval Nouvel intervalle en ms
   */
  void setSampleInterval(uint16_t interval) {
    sampleInterval = interval;
  }

  /**
   * @brief Récupère l'intervalle de lecture
   * @return Intervalle en ms
   */
  uint16_t getSampleInterval() const {
    return sampleInterval;
  }

  /**
   * @brief Définit la pression au niveau de la mer
   * @param pressure Pression en hPa
   * 
   * @details
   * Utilisé pour le calcul d'altitude. À ajuster selon conditions météo.
   * Valeur standard : 1013.25 hPa
   */
  void setSeaLevelPressure(float pressure) {
    seaLevelPressure = pressure;
  }

  /**
   * @brief Récupère la pression niveau mer configurée
   * @return Pression en hPa
   */
  float getSeaLevelPressure() const {
    return seaLevelPressure;
  }

  /**
   * @brief Récupère l'adresse I2C du capteur
   * @return Adresse I2C (0x76 ou 0x77)
   */
  uint8_t getI2CAddress() const {
    return i2cAddress;
  }

private:
  /**
   * @brief Lit les données du capteur
   * @return true si succès
   */
  bool readSensor() {
    lastUpdate = millis();
    
    // Lire toutes les valeurs
    currentData.temperature = bme.readTemperature();
    currentData.humidity = bme.readHumidity();
    currentData.pressure = bme.readPressure() / 100.0F; // Pa → hPa
    
    // Calculer altitude basée sur pression
    currentData.altitude = bme.readAltitude(seaLevelPressure);
    
    currentData.timestamp = lastUpdate;
    
    // Vérifier valeurs valides (NaN = erreur lecture)
    if (isnan(currentData.temperature) || 
        isnan(currentData.humidity) || 
        isnan(currentData.pressure)) {
      status = BME280Status::ERROR_COMM;
      return false;
    }
    
    return true;
  }
};

#endif // BME280_SENSOR_H
