/**
 * @file DS18B20Sensor.h
 * @brief Classe de gestion du capteur de température DS18B20
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-21
 * 
 * @details
 * Utilisable dans le programme de test et le programme principal.
 * 
 * Principes de conception :
 * - Gestion simple d'un ou plusieurs capteurs sur le même bus OneWire
 * - Responsabilité unique : lire et traiter température
 * - Support multi-capteurs avec adresses
 */

#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ============================================
// CONFIGURATION
// ============================================
#define DS18B20_RESOLUTION_9BIT     9   ///< Résolution 9 bits (0.5°C, 93.75ms)
#define DS18B20_RESOLUTION_10BIT    10  ///< Résolution 10 bits (0.25°C, 187.5ms)
#define DS18B20_RESOLUTION_11BIT    11  ///< Résolution 11 bits (0.125°C, 375ms)
#define DS18B20_RESOLUTION_12BIT    12  ///< Résolution 12 bits (0.0625°C, 750ms)

#define DS18B20_DEFAULT_RESOLUTION  DS18B20_RESOLUTION_12BIT
#define DS18B20_INVALID_TEMP        -127.0f                   ///< Valeur d'erreur
#define DS18B20_MAX_SENSORS         4                         ///< Nombre max de capteurs

// ============================================
// TYPES ET STRUCTURES
// ============================================
/**
 * @struct DS18B20Data
 * @brief Structure contenant les données d'un capteur
 */
struct DS18B20Data {
  float temperature;           ///< Température en °C
  bool valid;                  ///< Validité de la mesure
  unsigned long timestamp;     ///< Timestamp de la mesure (millis)
  uint8_t address[8];          ///< Adresse OneWire du capteur
};

/**
 * @enum DS18B20Status
 * @brief État du capteur
 */
enum class DS18B20Status {
  OK,                    ///< Fonctionnement normal
  NOT_INITIALIZED,       ///< Pas encore initialisé
  NO_SENSOR_FOUND,       ///< Aucun capteur détecté
  READ_ERROR,            ///< Erreur de lecture
  INVALID_TEMPERATURE    ///< Température invalide
};

// ============================================
// DEFINITION CLASSE DS18B20Sensor
// ============================================
/**
 * @class DS18B20Sensor
 * @brief Classe de gestion du capteur DS18B20
 * 
 * Cette classe gère un ou plusieurs capteurs DS18B20 sur un bus OneWire.
 * Elle permet la lecture de température avec gestion d'erreurs.
 */
class DS18B20Sensor {
private:
  OneWire oneWire;                      ///< Instance OneWire
  DallasTemperature sensors;            ///< Instance DallasTemperature
  
  uint8_t sensorCount;                  ///< Nombre de capteurs détectés
  DeviceAddress sensorAddresses[DS18B20_MAX_SENSORS]; ///< Adresses des capteurs
  
  float currentTemperatures[DS18B20_MAX_SENSORS];  ///< Températures actuelles
  unsigned long lastUpdate;             ///< Timestamp dernière MAJ
  uint16_t updateInterval;              ///< Intervalle de mise à jour (ms)
  uint8_t resolution;                   ///< Résolution (9-12 bits)
  
  bool initialized;                     ///< État initialisation
  DS18B20Status status;                 ///< État actuel du capteur

  /**
   * @brief Vérifie si une température est valide
   * @param temp Température à vérifier
   * @return true si valide
   */
  bool isValidTemperature(float temp) const {
    return (temp > -55.0 && temp < 125.0 && temp != DS18B20_INVALID_TEMP);
  }

public:
  /**
   * @brief Constructeur
   * @param pin Broche GPIO du bus OneWire
   * @param interval Intervalle mise à jour en ms (défaut 1000ms)
   * @param res Résolution en bits (9-12, défaut 12)
   */
  DS18B20Sensor(uint8_t pin, uint16_t interval = 1000, uint8_t res = DS18B20_DEFAULT_RESOLUTION) 
    : oneWire(pin),
      sensors(&oneWire),
      sensorCount(0),
      lastUpdate(0),
      updateInterval(interval),
      resolution(res),
      initialized(false),
      status(DS18B20Status::NOT_INITIALIZED)
  {
    // Initialiser températures à valeur invalide
    for (uint8_t i = 0; i < DS18B20_MAX_SENSORS; i++) {
      currentTemperatures[i] = DS18B20_INVALID_TEMP;
    }
  }

  // INITIALISATION
  // ------------------------------------------
  /**
   * @brief Initialise le(s) capteur(s)
   * @return true si au moins un capteur détecté
   */
  bool begin() {
    sensors.begin();
    
    // Détecter capteurs
    sensorCount = sensors.getDeviceCount();
    
    if (sensorCount == 0) {
      status = DS18B20Status::NO_SENSOR_FOUND;
      return false;
    }
    
    // Limiter au nombre max
    if (sensorCount > DS18B20_MAX_SENSORS) {
      sensorCount = DS18B20_MAX_SENSORS;
    }
    
    // Récupérer adresses et configurer résolution
    for (uint8_t i = 0; i < sensorCount; i++) {
      if (sensors.getAddress(sensorAddresses[i], i)) {
        sensors.setResolution(sensorAddresses[i], resolution);
      }
    }
    
    // Mode asynchrone pour non-blocage
    sensors.setWaitForConversion(false);
    
    initialized = true;
    status = DS18B20Status::OK;
    return true;
  }

  // LECTURE DONNÉES
  // ------------------------------------------
  /**
   * @brief Met à jour les lectures du capteur (non-bloquant)
   * @return true si mise à jour effectuée
   * 
   * @details À appeler dans loop(). Met à jour uniquement si intervalle écoulé.
   * Utilise le mode asynchrone pour éviter les blocages.
   */
  bool update() {
    if (!initialized) return false;
    
    unsigned long now = millis();
    if (now - lastUpdate < updateInterval) {
      return false;
    }
    
    lastUpdate = now;
    
    // Lancer conversion asynchrone
    requestTemperatures();
    
    // Attendre fin conversion (non-bloquant via délai court)
    delay(750 / (1 << (12 - resolution))); // Délai selon résolution
    
    // Lire températures
    return readTemperatures();
  }

  /**
   * @brief Définit l'intervalle de mise à jour
   * @param interval Intervalle en millisecondes
   */
  void setUpdateInterval(uint16_t interval) {
    updateInterval = interval;
  }

  /**
   * @brief Définit la résolution du capteur
   * @param res Résolution en bits (9-12)
   */
  void setResolution(uint8_t res) {
    if (res < 9 || res > 12) return;
    resolution = res;
    
    if (initialized) {
      for (uint8_t i = 0; i < sensorCount; i++) {
        sensors.setResolution(sensorAddresses[i], resolution);
      }
    }
  }

  /**
   * @brief Démarre une conversion de température
   * 
   * @details En mode asynchrone, lance la conversion sans attendre.
   * Appeler isConversionComplete() puis readTemperatures() ensuite.
   */
  void requestTemperatures() {
    if (!initialized) return;
    sensors.requestTemperatures();
  }

  /**
   * @brief Vérifie si la conversion est terminée
   * @return true si conversion terminée
   */
  bool isConversionComplete() const {
    if (!initialized) return false;
    return sensors.isConversionComplete();
  }

  /**
   * @brief Lit les températures de tous les capteurs
   * @return true si lecture réussie
   */
  bool readTemperatures() {
    if (!initialized) return false;
    
    bool allValid = true;
    
    for (uint8_t i = 0; i < sensorCount; i++) {
      float temp = sensors.getTempC(sensorAddresses[i]);
      
      if (isValidTemperature(temp)) {
        currentTemperatures[i] = temp;
      } else {
        currentTemperatures[i] = DS18B20_INVALID_TEMP;
        allValid = false;
      }
    }
    
    if (!allValid) {
      status = DS18B20Status::INVALID_TEMPERATURE;
    } else {
      status = DS18B20Status::OK;
    }
    
    return allValid;
  }

  /**
   * @brief Force une mise à jour immédiate bloquante
   * @return true si lecture réussie
   */
  bool forceUpdate() {
    if (!initialized) return false;
    
    sensors.setWaitForConversion(true);
    sensors.requestTemperatures();
    bool result = readTemperatures();
    sensors.setWaitForConversion(false);
    
    lastUpdate = millis();
    return result;
  }

  // Getters
  
  /**
   * @brief Obtient la température d'un capteur
   * @param index Index du capteur (0 par défaut)
   * @return Température en °C ou DS18B20_INVALID_TEMP si erreur
   */
  float getTemperature(uint8_t index = 0) const {
    if (index >= sensorCount) return DS18B20_INVALID_TEMP;
    return currentTemperatures[index];
  }

  /**
   * @brief Obtient le nombre de capteurs détectés
   * @return Nombre de capteurs
   */
  uint8_t getSensorCount() const {
    return sensorCount;
  }

  /**
   * @brief Obtient l'adresse d'un capteur
   * @param index Index du capteur
   * @param address [out] Tableau de 8 bytes pour l'adresse
   * @return true si succès
   */
  bool getSensorAddress(uint8_t index, uint8_t* address) const {
    if (index >= sensorCount) return false;
    memcpy(address, sensorAddresses[index], 8);
    return true;
  }

  /**
   * @brief Obtient le statut du capteur
   * @return Statut actuel
   */
  DS18B20Status getStatus() const {
    return status;
  }

  /**
   * @brief Vérifie si le capteur est initialisé
   * @return true si initialisé
   */
  bool isInitialized() const {
    return initialized;
  }

  /**
   * @brief Vérifie si une température est dans une plage
   * @param index Index du capteur
   * @param minTemp Température minimale
   * @param maxTemp Température maximale
   * @return true si dans la plage
   */
  bool isInRange(uint8_t index, float minTemp, float maxTemp) const {
    float temp = getTemperature(index);
    if (temp == DS18B20_INVALID_TEMP) return false;
    return (temp >= minTemp && temp <= maxTemp);
  }

  /**
   * @brief Vérifie si température > seuil
   * @param index Index du capteur
   * @param threshold Seuil en °C
   * @return true si dépassement
   */
  bool isAboveThreshold(uint8_t index, float threshold) const {
    float temp = getTemperature(index);
    if (temp == DS18B20_INVALID_TEMP) return false;
    return temp > threshold;
  }

  /**
   * @brief Obtient toutes les données d'un capteur
   * @param index Index du capteur
   * @return Structure DS18B20Data
   */
  DS18B20Data getData(uint8_t index = 0) const {
    DS18B20Data data;
    data.temperature = getTemperature(index);
    data.valid = (data.temperature != DS18B20_INVALID_TEMP);
    data.timestamp = millis();
    
    if (index < sensorCount) {
      memcpy(data.address, sensorAddresses[index], 8);
    }
    
    return data;
  }

  /**
   * @brief Convertit une adresse en chaîne hexadécimale
   * @param address Adresse du capteur (8 bytes)
   * @param buffer Buffer de sortie (min 24 caractères)
   */
  static void addressToString(const uint8_t* address, char* buffer) {
    for (uint8_t i = 0; i < 8; i++) {
      sprintf(buffer + i * 3, "%02X ", address[i]);
    }
    buffer[23] = '\0'; // Terminer la chaîne
  }
};

#endif // DS18B20_SENSOR_H
