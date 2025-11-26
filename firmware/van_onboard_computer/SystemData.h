/**
 * @file SystemData.h
 * @brief Structures de données et types du système Van Onboard Computer
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-26
 * 
 * @details
 * Fichier contenant :
 * - Énumérations des modes système et écrans
 * - Structures de données consolidées
 * - Niveaux d'alerte et types d'alertes
 * - État global du système
 */

#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include <Arduino.h>
#include "config.h"

// ============================================
// ÉNUMÉRATIONS - MODES SYSTÈME
// ============================================

/**
 * @enum SystemMode
 * @brief Mode de fonctionnement du système
 */
enum class SystemMode {
  MODE_PREHEAT,     ///< Pré-chauffage capteurs MQ (3 min)
  MODE_NORMAL,      ///< Fonctionnement normal
  MODE_SETTINGS,    ///< Menu paramètres (appui long encodeur)
  MODE_ALERT        ///< Alerte active (bloque navigation)
};

/**
 * @enum Screen
 * @brief Écrans disponibles dans l'interface
 */
enum class Screen {
  SCREEN_HOME,        ///< Écran accueil (températures + tensions + horizontalité)
  SCREEN_ENVIRONMENT, ///< Détails environnement (temp/humidité/pression/point de rosée)
  SCREEN_ENERGY,      ///< Détails énergie (12V/5V détaillés, courants, puissance)
  SCREEN_SAFETY,      ///< Détails sécurité (CO/GPL/fumée en temps réel)
  SCREEN_LEVEL,       ///< Détails horizontalité (Roll/Pitch détaillés)
  SCREEN_SETTINGS     ///< Paramètres et calibration
};

// ============================================
// ÉNUMÉRATIONS - ALERTES
// ============================================

/**
 * @enum AlertLevel
 * @brief Niveau de gravité d'une alerte
 */
enum class AlertLevel {
  NONE = 0,         ///< Aucune alerte
  INFO = 1,         ///< Information (icône uniquement)
  WARNING = 2,      ///< Avertissement (icône + bip lent)
  DANGER = 3,       ///< Danger (bloque écran, bips rapides)
  CRITICAL = 4      ///< Critique (bloque tout, buzzer continu)
};

/**
 * @enum AlertType
 * @brief Type d'alerte détecté
 */
enum class AlertType {
  NONE,             ///< Aucune alerte
  CO_HIGH,          ///< CO élevé
  GPL_HIGH,         ///< GPL élevé
  SMOKE_HIGH,       ///< Fumée détectée
  VOLTAGE_12V_LOW,  ///< Batterie faible
  VOLTAGE_12V_HIGH, ///< Surtension 12V
  VOLTAGE_5V_LOW,   ///< Sous-tension 5V
  VOLTAGE_5V_HIGH,  ///< Surtension 5V
  CURRENT_12V_HIGH, ///< Sur-courant 12V
  CURRENT_5V_HIGH,  ///< Sur-courant 5V
  TEMP_HIGH,        ///< Température élevée
  TEMP_LOW,         ///< Température basse
  HUMIDITY_HIGH,    ///< Humidité élevée
  TILT_HIGH         ///< Inclinaison importante
};

// ============================================
// STRUCTURES - DONNÉES CAPTEURS
// ============================================

/**
 * @struct EnvironmentData
 * @brief Données environnementales consolidées
 */
struct EnvironmentData {
  // Températures
  float tempInterior;       ///< Température intérieure (°C) - BME280
  float tempExterior;       ///< Température extérieure (°C) - DS18B20
  
  // Humidité et pression
  float humidity;           ///< Humidité relative (%) - BME280
  float pressure;           ///< Pression atmosphérique (hPa) - BME280
  float dewPoint;           ///< Point de rosée calculé (°C)
  
  // Timestamps
  unsigned long tempIntTimestamp;
  unsigned long tempExtTimestamp;
  
  // Validité
  bool tempIntValid;
  bool tempExtValid;
  bool humidityValid;
  bool pressureValid;
};

/**
 * @struct PowerData
 * @brief Données électriques consolidées
 */
struct PowerData {
  // Rail 12V
  float voltage12V;         ///< Tension 12V (V)
  float current12V;         ///< Courant 12V (A)
  float power12V;           ///< Puissance 12V (W)
  
  // Rail 5V
  float voltage5V;          ///< Tension 5V (V)
  float current5V;          ///< Courant 5V (A)
  float power5V;            ///< Puissance 5V (W)
  
  // Puissance totale
  float powerTotal;         ///< Puissance totale (W)
  
  // Timestamps
  unsigned long voltage12VTimestamp;
  unsigned long voltage5VTimestamp;
  
  // Validité
  bool voltage12VValid;
  bool voltage5VValid;
};

/**
 * @struct SafetyData
 * @brief Données de sécurité (gaz)
 */
struct SafetyData {
  // Concentrations en ppm
  float coPPM;              ///< Monoxyde de carbone (ppm) - MQ7
  float gplPPM;             ///< GPL/Méthane (ppm) - MQ2
  float smokePPM;           ///< Fumée (ppm) - MQ2
  
  // Timestamps
  unsigned long coTimestamp;
  unsigned long gplTimestamp;
  unsigned long smokeTimestamp;
  
  // Validité
  bool coValid;
  bool gplValid;
  bool smokeValid;
  
  // État pré-chauffage
  bool mq7Preheated;
  bool mq2Preheated;
};

/**
 * @struct LevelData
 * @brief Données d'horizontalité (MPU6050)
 */
struct LevelData {
  // Angles calibrés
  float roll;               ///< Angle Roll (°) - inclinaison latérale
  float pitch;              ///< Angle Pitch (°) - inclinaison avant/arrière
  float yaw;                ///< Angle Yaw (°) - rotation
  
  // Angles bruts (avant calibration)
  float rawRoll;
  float rawPitch;
  
  // Inclinaison totale
  float totalTilt;          ///< Magnitude inclinaison (°)
  
  // Température du capteur
  float temperature;        ///< Température MPU6050 (°C)
  
  // Timestamp
  unsigned long timestamp;
  
  // Validité
  bool valid;
  bool calibrated;
};

// ============================================
// STRUCTURES - ALERTES
// ============================================

/**
 * @struct Alert
 * @brief Structure d'une alerte active
 */
struct Alert {
  AlertType type;           ///< Type d'alerte
  AlertLevel level;         ///< Niveau de gravité
  float value;              ///< Valeur ayant déclenché l'alerte
  float threshold;          ///< Seuil déclenché
  unsigned long timestamp;  ///< Moment déclenchement (millis)
  bool active;              ///< Alerte active ou non
  const char* message;      ///< Message descriptif
};

/**
 * @struct AlertState
 * @brief État global du système d'alerte
 */
struct AlertState {
  AlertLevel currentLevel;      ///< Niveau le plus élevé actif
  AlertType primaryAlert;       ///< Alerte prioritaire
  uint8_t activeAlertCount;     ///< Nombre d'alertes actives
  Alert alerts[10];             ///< Tableau des alertes (max 10)
  bool buzzerActive;            ///< Buzzer activé ou non
  bool blockNavigation;         ///< Navigation bloquée (DANGER/CRITICAL)
  unsigned long lastBuzzerToggle; ///< Pour gestion bips
};

// ============================================
// STRUCTURES - ÉTAT SYSTÈME
// ============================================

/**
 * @struct SensorStatus
 * @brief État des capteurs
 */
struct SensorStatus {
  bool bme280;              ///< BME280 disponible
  bool ds18b20;             ///< DS18B20 disponible
  bool mpu6050;             ///< MPU6050 disponible
  bool mq7;                 ///< MQ7 disponible
  bool mq2;                 ///< MQ2 disponible
  bool ina226_12v;          ///< INA226 12V disponible
  bool ina226_5v;           ///< INA226 5V disponible
  bool lcd;                 ///< LCD disponible
  bool encoder;             ///< Encodeur disponible
  bool leds;                ///< LEDs WS2812B disponibles
  bool buzzer;              ///< Buzzer disponible
};

/**
 * @struct SystemState
 * @brief État global du système
 */
struct SystemState {
  // Mode et écran
  SystemMode mode;              ///< Mode de fonctionnement actuel
  Screen currentScreen;         ///< Écran affiché actuellement
  Screen previousScreen;        ///< Écran précédent (pour retour)
  
  // Données capteurs
  EnvironmentData environment;  ///< Données environnement
  PowerData power;              ///< Données électriques
  SafetyData safety;            ///< Données sécurité gaz
  LevelData level;              ///< Données horizontalité
  
  // Alertes
  AlertState alerts;            ///< État des alertes
  
  // État capteurs
  SensorStatus sensors;         ///< Disponibilité capteurs
  
  // Timing
  unsigned long preheatStartTime;     ///< Début pré-chauffage
  unsigned long lastEncoderActivity;  ///< Dernière activité encodeur
  unsigned long uptime;               ///< Temps depuis démarrage (s)
  
  // Flags
  bool initialized;             ///< Système initialisé
  bool backlightOn;             ///< Rétro-éclairage LCD actif
  bool calibrationMode;         ///< Mode calibration MPU6050
};

// ============================================
// FONCTIONS UTILITAIRES - CONVERSIONS
// ============================================

/**
 * @brief Convertit un niveau d'alerte en texte
 * @param level Niveau d'alerte
 * @return Chaîne de caractères
 */
inline const char* alertLevelToString(AlertLevel level) {
  switch (level) {
    case AlertLevel::NONE:      return "OK";
    case AlertLevel::INFO:      return "INFO";
    case AlertLevel::WARNING:   return "ATTENTION";
    case AlertLevel::DANGER:    return "DANGER";
    case AlertLevel::CRITICAL:  return "CRITIQUE";
    default:                    return "INCONNU";
  }
}

/**
 * @brief Convertit un type d'alerte en message court
 * @param type Type d'alerte
 * @return Message court
 */
inline const char* alertTypeToShortString(AlertType type) {
  switch (type) {
    case AlertType::NONE:             return "OK";
    case AlertType::CO_HIGH:          return "CO ELEVE";
    case AlertType::GPL_HIGH:         return "GPL ELEVE";
    case AlertType::SMOKE_HIGH:       return "FUMEE";
    case AlertType::VOLTAGE_12V_LOW:  return "BATT FAIBLE";
    case AlertType::VOLTAGE_12V_HIGH: return "12V HAUT";
    case AlertType::VOLTAGE_5V_LOW:   return "5V BAS";
    case AlertType::VOLTAGE_5V_HIGH:  return "5V HAUT";
    case AlertType::CURRENT_12V_HIGH: return "SURINT 12V";
    case AlertType::CURRENT_5V_HIGH:  return "SURINT 5V";
    case AlertType::TEMP_HIGH:        return "TEMP HAUTE";
    case AlertType::TEMP_LOW:         return "TEMP BASSE";
    case AlertType::HUMIDITY_HIGH:    return "HUMID HAUTE";
    case AlertType::TILT_HIGH:        return "INCLINAISON";
    default:                          return "INCONNU";
  }
}

/**
 * @brief Convertit un mode système en texte
 * @param mode Mode système
 * @return Chaîne de caractères
 */
inline const char* systemModeToString(SystemMode mode) {
  switch (mode) {
    case SystemMode::MODE_PREHEAT:  return "PRE-CHAUFFE";
    case SystemMode::MODE_NORMAL:   return "NORMAL";
    case SystemMode::MODE_SETTINGS: return "PARAMETRES";
    case SystemMode::MODE_ALERT:    return "ALERTE";
    default:                        return "INCONNU";
  }
}

/**
 * @brief Convertit un écran en texte
 * @param screen Écran
 * @return Nom de l'écran
 */
inline const char* screenToString(Screen screen) {
  switch (screen) {
    case Screen::SCREEN_HOME:         return "ACCUEIL";
    case Screen::SCREEN_ENVIRONMENT:  return "ENVIRONNEMENT";
    case Screen::SCREEN_ENERGY:       return "ENERGIE";
    case Screen::SCREEN_SAFETY:       return "SECURITE";
    case Screen::SCREEN_LEVEL:        return "HORIZONTALITE";
    case Screen::SCREEN_SETTINGS:     return "PARAMETRES";
    default:                          return "INCONNU";
  }
}

// ============================================
// FONCTIONS UTILITAIRES - VALIDATION
// ============================================

/**
 * @brief Vérifie si une valeur de température est valide
 * @param temp Température en °C
 * @return true si valide
 */
inline bool isValidTemperature(float temp) {
  return (temp > -50.0 && temp < 100.0 && !isnan(temp));
}

/**
 * @brief Vérifie si une valeur d'humidité est valide
 * @param humidity Humidité en %
 * @return true si valide
 */
inline bool isValidHumidity(float humidity) {
  return (humidity >= 0.0 && humidity <= 100.0 && !isnan(humidity));
}

/**
 * @brief Vérifie si une valeur de tension est valide
 * @param voltage Tension en V
 * @return true si valide
 */
inline bool isValidVoltage(float voltage) {
  return (voltage > 0.0 && voltage < 20.0 && !isnan(voltage));
}

/**
 * @brief Vérifie si une valeur PPM est valide
 * @param ppm Concentration en ppm
 * @return true si valide
 */
inline bool isValidPPM(float ppm) {
  return (ppm >= 0.0 && ppm < 10000.0 && !isnan(ppm));
}

// ============================================
// INITIALISATION STRUCTURES
// ============================================

/**
 * @brief Initialise une structure SystemState avec valeurs par défaut
 * @param state Structure à initialiser
 */
inline void initSystemState(SystemState& state) {
  // Mode et écrans
  state.mode = SystemMode::MODE_PREHEAT;
  state.currentScreen = Screen::SCREEN_HOME;
  state.previousScreen = Screen::SCREEN_HOME;
  
  // Environnement
  state.environment.tempInterior = 0.0;
  state.environment.tempExterior = 0.0;
  state.environment.humidity = 0.0;
  state.environment.pressure = 0.0;
  state.environment.dewPoint = 0.0;
  state.environment.tempIntValid = false;
  state.environment.tempExtValid = false;
  state.environment.humidityValid = false;
  state.environment.pressureValid = false;
  
  // Puissance
  state.power.voltage12V = 0.0;
  state.power.current12V = 0.0;
  state.power.power12V = 0.0;
  state.power.voltage5V = 0.0;
  state.power.current5V = 0.0;
  state.power.power5V = 0.0;
  state.power.powerTotal = 0.0;
  state.power.voltage12VValid = false;
  state.power.voltage5VValid = false;
  
  // Sécurité
  state.safety.coPPM = 0.0;
  state.safety.gplPPM = 0.0;
  state.safety.smokePPM = 0.0;
  state.safety.coValid = false;
  state.safety.gplValid = false;
  state.safety.smokeValid = false;
  state.safety.mq7Preheated = false;
  state.safety.mq2Preheated = false;
  
  // Horizontalité
  state.level.roll = 0.0;
  state.level.pitch = 0.0;
  state.level.yaw = 0.0;
  state.level.rawRoll = 0.0;
  state.level.rawPitch = 0.0;
  state.level.totalTilt = 0.0;
  state.level.temperature = 0.0;
  state.level.valid = false;
  state.level.calibrated = false;
  
  // Alertes
  state.alerts.currentLevel = AlertLevel::NONE;
  state.alerts.primaryAlert = AlertType::NONE;
  state.alerts.activeAlertCount = 0;
  state.alerts.buzzerActive = false;
  state.alerts.blockNavigation = false;
  state.alerts.lastBuzzerToggle = 0;
  
  // Initialiser tableau alertes
  for (uint8_t i = 0; i < 10; i++) {
    state.alerts.alerts[i].active = false;
    state.alerts.alerts[i].type = AlertType::NONE;
    state.alerts.alerts[i].level = AlertLevel::NONE;
  }
  
  // Capteurs
  state.sensors.bme280 = false;
  state.sensors.ds18b20 = false;
  state.sensors.mpu6050 = false;
  state.sensors.mq7 = false;
  state.sensors.mq2 = false;
  state.sensors.ina226_12v = false;
  state.sensors.ina226_5v = false;
  state.sensors.lcd = false;
  state.sensors.encoder = false;
  state.sensors.leds = false;
  state.sensors.buzzer = false;
  
  // Timing
  state.preheatStartTime = 0;
  state.lastEncoderActivity = 0;
  state.uptime = 0;
  
  // Flags
  state.initialized = false;
  state.backlightOn = true;
  state.calibrationMode = false;
}

#endif // SYSTEM_DATA_H
