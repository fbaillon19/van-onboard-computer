/**
 * @file INA226Sensor.h
 * @brief Classe de gestion du capteur de courant/tension INA226
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-24
 * 
 * @details
 * Utilisable dans le programme de test et le programme principal.
 * Compatible avec la bibliothèque INA226Lib par Peter Buchegger.
 * Développé pour Arduino Mega R3.
 * 
 * Principes de conception :
 * - Support de plusieurs INA226 sur le même bus I2C (12V et 5V)
 * - Mesure tension, courant et puissance
 * - Configuration flexible (résolution, moyennage)
 * - Responsabilité unique : surveiller alimentation électrique
 */

#ifndef INA226_SENSOR_H
#define INA226_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <INA226.h>  // INA226Lib par Peter Buchegger

// ============================================
// CONFIGURATION MATÉRIELLE
// ============================================
#define INA226_I2C_ADDR_12V     0x40  ///< Adresse I2C rail 12V (A1=GND, A0=GND)
#define INA226_I2C_ADDR_5V      0x41  ///< Adresse I2C rail 5V (A1=GND, A0=VCC)

// Paramètres des Shunts
#define INA226_SHUNT_12V        0.002f  ///< Résistance shunt 12V en Ohms (2 mΩ)
#define INA226_SHUNT_5V         0.010f  ///< Résistance shunt 5V en Ohms (10 mΩ)

// Courants max calculés selon les shunts
// INA226 : V_shunt_max = ±81.92 mV
// I_max = V_shunt_max / R_shunt
#define INA226_MAX_CURRENT_12V  40.0f   ///< Courant max 12V : 81.92mV / 2mΩ = 40.96A
#define INA226_MAX_CURRENT_5V   8.0f    ///< Courant max 5V : 81.92mV / 10mΩ = 8.19A

#define INA226_UPDATE_INTERVAL  500     ///< Intervalle de mise à jour (ms)

// ============================================
// TYPES ET STRUCTURES
// ============================================
/**
 * @enum PowerRailType
 * @brief Type de rail d'alimentation
 */
enum class PowerRailType {
  RAIL_12V,     ///< Rail 12 volts
  RAIL_5V       ///< Rail 5 volts
};

/**
 * @struct INA226Data
 * @brief Structure contenant les mesures du capteur
 */
struct INA226Data {
  float busVoltage;         ///< Tension bus en V
  float shuntVoltage;       ///< Tension shunt en V
  float current;            ///< Courant en A
  float power;              ///< Puissance en W
  bool valid;               ///< Validité des mesures
  unsigned long timestamp;  ///< Timestamp de la mesure
};

/**
 * @enum INA226Status
 * @brief État du capteur
 */
enum class INA226Status {
  OK,                   ///< Fonctionnement normal
  NOT_INITIALIZED,      ///< Pas encore initialisé
  NOT_FOUND,            ///< Capteur non détecté sur I2C
  OVERFLOW,             ///< Dépassement capacité mesure
  UNDERVOLTAGE,         ///< Sous-tension détectée
  OVERCURRENT           ///< Sur-courant détecté
};

// ============================================
// DEFINITION CLASSE INA226Sensor
// ============================================
/**
 * @class INA226Sensor
 * @brief Classe de gestion du capteur INA226
 * 
 * Cette classe gère un capteur INA226 pour surveiller tension,
 * courant et puissance sur un rail d'alimentation.
 */
class INA226Sensor {
private:
  INA226 ina;                       ///< Instance librairie INA226
  PowerRailType railType;           ///< Type de rail surveillé
  uint8_t i2cAddress;               ///< Adresse I2C
  
  // Mesures actuelles
  float busVoltage;                 ///< Tension bus (V)
  float shuntVoltage;               ///< Tension shunt (V)
  float current;                    ///< Courant (A)
  float power;                      ///< Puissance (W)
  
  // Configuration
  float shuntResistance;            ///< Résistance shunt (Ω)
  float maxCurrent;                 ///< Courant max (A)
  
  // Seuils d'alerte
  float voltageMin;                 ///< Tension minimale (V)
  float voltageMax;                 ///< Tension maximale (V)
  float currentMax;                 ///< Courant maximal (A)
  
  // État
  bool initialized;                 ///< État initialisation
  INA226Status status;              ///< État actuel
  unsigned long lastUpdate;         ///< Timestamp dernière MAJ
  uint16_t updateInterval;          ///< Intervalle de mise à jour (ms)
  
  /**
   * @brief Vérifie si les mesures sont dans les limites
   * @return true si OK, false si dépassement
   */
  bool checkLimits() {
    bool ok = true;
    
    // Vérifier tension
    if (busVoltage < voltageMin) {
      status = INA226Status::UNDERVOLTAGE;
      ok = false;
    } else if (busVoltage > voltageMax) {
      status = INA226Status::OVERFLOW;
      ok = false;
    }
    
    // Vérifier courant
    if (current > currentMax) {
      status = INA226Status::OVERCURRENT;
      ok = false;
    }
    
    if (ok) {
      status = INA226Status::OK;
    }
    
    return ok;
  }

public:
  /**
   * @brief Constructeur
   * @param rail Type de rail (12V ou 5V)
   * @param address Adresse I2C (défaut selon le rail)
   * @param interval Intervalle mise à jour en ms (défaut 500ms)
   */
  INA226Sensor(PowerRailType rail, 
               uint8_t address = 0, 
               uint16_t interval = INA226_UPDATE_INTERVAL)
    : railType(rail),
      i2cAddress(address),
      busVoltage(0.0f),
      shuntVoltage(0.0f),
      current(0.0f),
      power(0.0f),
      initialized(false),
      status(INA226Status::NOT_INITIALIZED),
      lastUpdate(0),
      updateInterval(interval)
  {
    // Définir adresse I2C par défaut selon le rail
    if (address == 0) {
      i2cAddress = (rail == PowerRailType::RAIL_12V) ? 
        INA226_I2C_ADDR_12V : INA226_I2C_ADDR_5V;
    }
    
    // Définir shunt et courant max selon le rail
    if (rail == PowerRailType::RAIL_12V) {
      shuntResistance = INA226_SHUNT_12V;      // 2 mΩ
      maxCurrent = INA226_MAX_CURRENT_12V;     // 40A
      voltageMin = 10.5f;   // Minimum 12V
      voltageMax = 14.5f;   // Maximum 12V
      currentMax = 20.0f;   // Seuil alerte courant (adapté à vos besoins)
    } else {
      shuntResistance = INA226_SHUNT_5V;       // 10 mΩ
      maxCurrent = INA226_MAX_CURRENT_5V;      // 8A
      voltageMin = 4.5f;    // Minimum 5V
      voltageMax = 5.5f;    // Maximum 5V
      currentMax = 3.0f;    // Seuil alerte courant (adapté à vos besoins)
    }
  }

  // INITIALISATION
  // ------------------------------------------
  /**
   * @brief Initialise le capteur
   * @return true si succès
   */
  bool begin() {
    // Vérifier présence I2C
    Wire.beginTransmission(i2cAddress);
    if (Wire.endTransmission() != 0) {
      status = INA226Status::NOT_FOUND;
      return false;
    }
    
    // Initialiser INA226 avec l'API de INA226Lib
    // begin(address) uniquement
    if (!ina.begin(i2cAddress)) {
      status = INA226Status::NOT_FOUND;
      return false;
    }
    
    // Configurer INA226
    // configure(averages, busConvTime, shuntConvTime, mode)
    ina.configure(INA226_AVERAGES_16,         // Moyennage sur 16 échantillons
                  INA226_BUS_CONV_TIME_1100US, // Temps conversion bus
                  INA226_SHUNT_CONV_TIME_1100US, // Temps conversion shunt
                  INA226_MODE_SHUNT_BUS_CONT);  // Mode continu
    
    // Calibrer avec résistance shunt et courant max
    // calibrate(rShunt, maxExpectedCurrent)
    ina.calibrate(shuntResistance, maxCurrent);
    
    initialized = true;
    status = INA226Status::OK;
    
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
   * @brief Définit les seuils de tension
   * @param min Tension minimale (V)
   * @param max Tension maximale (V)
   */
  void setVoltageThresholds(float min, float max) {
    voltageMin = min;
    voltageMax = max;
  }

  /**
   * @brief Définit le seuil de courant
   * @param max Courant maximal (A)
   */
  void setCurrentThreshold(float max) {
    currentMax = max;
  }

  /**
   * @brief Obtient les seuils actuels
   * @param vMin [out] Tension min
   * @param vMax [out] Tension max
   * @param iMax [out] Courant max
   */
  void getThresholds(float &vMin, float &vMax, float &iMax) const {
    vMin = voltageMin;
    vMax = voltageMax;
    iMax = currentMax;
  }

  /**
   * @brief Lit les valeurs du capteur
   * @return true si lecture réussie
   */
  bool readSensor() {
    if (!initialized) return false;
    
    // Lire toutes les valeurs avec l'API INA226Lib
    busVoltage = ina.readBusVoltage();      // Retourne en V
    shuntVoltage = ina.readShuntVoltage();  // Retourne en V
    current = ina.readShuntCurrent();       // Retourne en A
    power = ina.readBusPower();             // Retourne en W
    
    // Vérifier limites
    checkLimits();
    
    return true;
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
    return readSensor();
  }

  /**
   * @brief Force une mise à jour immédiate
   * @return true si lecture réussie
   */
  bool forceUpdate() {
    if (!initialized) return false;
    
    lastUpdate = millis();
    return readSensor();
  }

  // Getters
  
  /**
   * @brief Obtient la tension du bus
   * @return Tension en V
   */
  float getBusVoltage() const {
    return busVoltage;
  }

  /**
   * @brief Obtient la tension du shunt
   * @return Tension en V (INA226Lib retourne déjà en V)
   */
  float getShuntVoltage() const {
    return shuntVoltage;
  }

  /**
   * @brief Obtient la tension du shunt en mV
   * @return Tension en mV
   */
  float getShuntVoltageMilliVolts() const {
    return shuntVoltage * 1000.0f;
  }

  /**
   * @brief Obtient le courant
   * @return Courant en A
   */
  float getCurrent() const {
    return current;
  }

  /**
   * @brief Obtient la puissance
   * @return Puissance en W
   */
  float getPower() const {
    return power;
  }

  /**
   * @brief Obtient le type de rail
   * @return Type de rail
   */
  PowerRailType getRailType() const {
    return railType;
  }

  /**
   * @brief Obtient l'adresse I2C
   * @return Adresse I2C
   */
  uint8_t getI2CAddress() const {
    return i2cAddress;
  }

  /**
   * @brief Obtient le statut du capteur
   * @return Statut actuel
   */
  INA226Status getStatus() const {
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
   * @brief Vérifie si la tension est dans les limites
   * @return true si OK
   */
  bool isVoltageOK() const {
    return (busVoltage >= voltageMin && busVoltage <= voltageMax);
  }

  /**
   * @brief Vérifie si le courant est dans les limites
   * @return true si OK
   */
  bool isCurrentOK() const {
    return (current <= currentMax);
  }

  /**
   * @brief Vérifie si toutes les mesures sont OK
   * @return true si OK
   */
  bool isHealthy() const {
    return (status == INA226Status::OK);
  }

  /**
   * @brief Obtient toutes les données du capteur
   * @return Structure INA226Data
   */
  INA226Data getData() const {
    INA226Data data;
    data.busVoltage = busVoltage;
    data.shuntVoltage = shuntVoltage;
    data.current = current;
    data.power = power;
    data.valid = (status == INA226Status::OK);
    data.timestamp = millis();
    return data;
  }

  /**
   * @brief Calcule l'énergie consommée depuis le dernier reset
   * @param deltaTimeMs Temps écoulé en ms
   * @return Énergie en Wh
   */
  float getEnergy(unsigned long deltaTimeMs) const {
    return (power * deltaTimeMs) / 3600000.0f; // W * ms -> Wh
  }

  /**
   * @brief Convertit le statut en texte
   * @param status Statut
   * @return Chaîne de caractères
   */
  static const char* statusToString(INA226Status status) {
    switch (status) {
      case INA226Status::OK:              return "OK";
      case INA226Status::NOT_INITIALIZED: return "NON INITIALISE";
      case INA226Status::NOT_FOUND:       return "NON DETECTE";
      case INA226Status::OVERFLOW:        return "SURTENSION";
      case INA226Status::UNDERVOLTAGE:    return "SOUS-TENSION";
      case INA226Status::OVERCURRENT:     return "SURINTENSITY";
      default:                            return "INCONNU";
    }
  }

  /**
   * @brief Convertit le type de rail en texte
   * @param rail Type de rail
   * @return Chaîne de caractères
   */
  static const char* railTypeToString(PowerRailType rail) {
    switch (rail) {
      case PowerRailType::RAIL_12V: return "12V";
      case PowerRailType::RAIL_5V:  return "5V";
      default:                      return "UNKNOWN";
    }
  }

  /**
   * @brief Formate une valeur avec unité
   * @param value Valeur
   * @param unit Unité
   * @param buffer Buffer de sortie (min 16 caractères)
   * @param decimals Nombre de décimales
   */
  static void formatValue(float value, const char* unit, char* buffer, uint8_t decimals = 2) {
    dtostrf(value, 0, decimals, buffer);
    strcat(buffer, " ");
    strcat(buffer, unit);
  }
};

#endif // INA226_SENSOR_H
