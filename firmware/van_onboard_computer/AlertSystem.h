/**
 * @file AlertSystem.h
 * @brief Système de gestion des alertes et alarmes
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-26
 * 
 * @details
 * Classe responsable de :
 * - Détection des conditions d'alerte (CO, GPL, tensions, températures)
 * - Hiérarchisation des alertes (INFO < WARNING < DANGER < CRITICAL)
 * - Gestion du buzzer selon niveau d'alerte
 * - Blocage de navigation en cas de danger
 * - Historique des alertes actives
 */

#ifndef ALERT_SYSTEM_H
#define ALERT_SYSTEM_H

#include <Arduino.h>
#include "config.h"
#include "SystemData.h"
#include "Buzzer.h"

// ============================================
// CLASSE AlertSystem
// ============================================
/**
 * @class AlertSystem
 * @brief Système de gestion des alertes et alarmes
 * 
 * @details
 * Hiérarchie des alertes (du plus grave au moins grave) :
 * 1. CO > 400 ppm → CRITICAL (bloque tout, buzzer continu)
 * 2. GPL > 3000 ppm → CRITICAL (idem)
 * 3. Fumée > 2000 ppm → DANGER (bloque écran, bips rapides)
 * 4. Batterie < 10.5V → DANGER
 * 5. CO > 200 ppm → WARNING (icône + bip lent)
 * 6. GPL > 1000 ppm → WARNING
 * 7. Température > 35° → WARNING
 * 8. Humidité > 80% → INFO (icône uniquement)
 */
class AlertSystem {
private:
  // Référence à l'état système
  SystemState& state;
  
  // Buzzer
  Buzzer* buzzer;
  
  // Timing
  unsigned long lastBuzzerToggle;
  unsigned long lastAlertCheck;
  
  // Configuration buzzer
  uint16_t buzzerInterval;      ///< Intervalle entre bips (ms)
  bool buzzerState;             ///< État actuel buzzer (on/off)
  
  // Flags
  bool initialized;
  
public:
  /**
   * @brief Constructeur
   * @param sysState Référence à l'état système
   */
  AlertSystem(SystemState& sysState) 
    : state(sysState),
      buzzer(nullptr),
      lastBuzzerToggle(0),
      lastAlertCheck(0),
      buzzerInterval(1000),
      buzzerState(false),
      initialized(false)
  {
  }
  
  /**
   * @brief Destructeur
   */
  ~AlertSystem() {
    if (buzzer) delete buzzer;
  }
  
  // ============================================
  // INITIALISATION
  // ============================================
  
  /**
   * @brief Initialise le système d'alerte
   * @return true si succès
   */
  bool begin() {
    DEBUG_PRINTLN(F("=== INITIALISATION SYSTEME ALERTE ==="));
    
    // Initialiser buzzer
    buzzer = new Buzzer(PIN_BUZZER);
    if (buzzer->begin()) {
      state.sensors.buzzer = true;
      DEBUG_PRINTLN(F("[OK] Buzzer initialise"));
    } else {
      DEBUG_PRINTLN(F("[ECHEC] Buzzer non initialise"));
      state.sensors.buzzer = false;
    }
    
    // Initialiser état alertes
    state.alerts.currentLevel = AlertLevel::NONE;
    state.alerts.primaryAlert = AlertType::NONE;
    state.alerts.activeAlertCount = 0;
    state.alerts.buzzerActive = false;
    state.alerts.blockNavigation = false;
    
    // Bip de démarrage
    if (state.sensors.buzzer) {
      buzzer->beep(1000);
    }
    
    initialized = true;
    return true;
  }
  
  // ============================================
  // VÉRIFICATION ALERTES
  // ============================================
  
  /**
   * @brief Vérifie toutes les conditions d'alerte
   * 
   * @details
   * À appeler dans chaque cycle de loop().
   * Met à jour l'état des alertes dans SystemState.
   */
  void checkAlerts() {
    if (!initialized) return;
    
    // Réinitialiser compteur alertes
    state.alerts.activeAlertCount = 0;
    state.alerts.currentLevel = AlertLevel::NONE;
    state.alerts.primaryAlert = AlertType::NONE;
    
    // Réinitialiser toutes les alertes
    for (uint8_t i = 0; i < 10; i++) {
      state.alerts.alerts[i].active = false;
    }
    
    // Ne vérifier les alertes gaz que si pré-chauffage terminé
    if (state.safety.mq7Preheated && state.safety.mq2Preheated) {
      checkGasAlerts();
    }
    
    // Toujours vérifier les autres alertes
    checkPowerAlerts();
    checkEnvironmentAlerts();
    checkLevelAlerts();
    
    // Mettre à jour mode système et buzzer
    updateAlertMode();
  }
  
  /**
   * @brief Vérifie les alertes gaz (CO, GPL, fumée)
   */
  void checkGasAlerts() {
    // === CO - CRITICAL (>400 ppm) ===
    if (state.safety.coValid && state.safety.coPPM > CO_THRESHOLD_DANGER) {
      addAlert(AlertType::CO_HIGH, AlertLevel::CRITICAL, 
               state.safety.coPPM, CO_THRESHOLD_DANGER,
               "CO CRITIQUE!");
    }
    // === CO - WARNING (>200 ppm) ===
    else if (state.safety.coValid && state.safety.coPPM > CO_THRESHOLD_WARNING) {
      addAlert(AlertType::CO_HIGH, AlertLevel::WARNING, 
               state.safety.coPPM, CO_THRESHOLD_WARNING,
               "CO eleve");
    }
    // === CO - INFO (>50 ppm) ===
    else if (state.safety.coValid && state.safety.coPPM > CO_THRESHOLD_INFO) {
      addAlert(AlertType::CO_HIGH, AlertLevel::INFO, 
               state.safety.coPPM, CO_THRESHOLD_INFO,
               "CO detecte");
    }
    
    // === GPL - CRITICAL (>3000 ppm) ===
    if (state.safety.gplValid && state.safety.gplPPM > GPL_THRESHOLD_DANGER) {
      addAlert(AlertType::GPL_HIGH, AlertLevel::CRITICAL, 
               state.safety.gplPPM, GPL_THRESHOLD_DANGER,
               "GPL CRITIQUE!");
    }
    // === GPL - WARNING (>1000 ppm) ===
    else if (state.safety.gplValid && state.safety.gplPPM > GPL_THRESHOLD_WARNING) {
      addAlert(AlertType::GPL_HIGH, AlertLevel::WARNING, 
               state.safety.gplPPM, GPL_THRESHOLD_WARNING,
               "GPL eleve");
    }
    // === GPL - INFO (>500 ppm) ===
    else if (state.safety.gplValid && state.safety.gplPPM > GPL_THRESHOLD_INFO) {
      addAlert(AlertType::GPL_HIGH, AlertLevel::INFO, 
               state.safety.gplPPM, GPL_THRESHOLD_INFO,
               "GPL detecte");
    }
    
    // === FUMÉE - DANGER (>2000 ppm) ===
    if (state.safety.smokeValid && state.safety.smokePPM > SMOKE_THRESHOLD_DANGER) {
      addAlert(AlertType::SMOKE_HIGH, AlertLevel::DANGER, 
               state.safety.smokePPM, SMOKE_THRESHOLD_DANGER,
               "FUMEE DANGER!");
    }
    // === FUMÉE - WARNING (>1500 ppm) ===
    else if (state.safety.smokeValid && state.safety.smokePPM > SMOKE_THRESHOLD_WARNING) {
      addAlert(AlertType::SMOKE_HIGH, AlertLevel::WARNING, 
               state.safety.smokePPM, SMOKE_THRESHOLD_WARNING,
               "Fumee detectee");
    }
    // === FUMÉE - INFO (>1000 ppm) ===
    else if (state.safety.smokeValid && state.safety.smokePPM > SMOKE_THRESHOLD_INFO) {
      addAlert(AlertType::SMOKE_HIGH, AlertLevel::INFO, 
               state.safety.smokePPM, SMOKE_THRESHOLD_INFO,
               "Fumee legere");
    }
  }
  
  /**
   * @brief Vérifie les alertes électriques (tensions, courants)
   */
  void checkPowerAlerts() {
    // === BATTERIE 12V BASSE - DANGER (<10.5V) ===
    if (state.power.voltage12VValid && state.power.voltage12V < VOLTAGE_12V_MIN) {
      addAlert(AlertType::VOLTAGE_12V_LOW, AlertLevel::DANGER, 
               state.power.voltage12V, VOLTAGE_12V_MIN,
               "BATTERIE CRITIQUE!");
    }
    // === BATTERIE 12V BASSE - WARNING (<11.5V) ===
    else if (state.power.voltage12VValid && state.power.voltage12V < VOLTAGE_12V_WARNING) {
      addAlert(AlertType::VOLTAGE_12V_LOW, AlertLevel::WARNING, 
               state.power.voltage12V, VOLTAGE_12V_WARNING,
               "Batterie faible");
    }
    
    // === SURTENSION 12V - WARNING (>14.5V) ===
    if (state.power.voltage12VValid && state.power.voltage12V > VOLTAGE_12V_MAX) {
      addAlert(AlertType::VOLTAGE_12V_HIGH, AlertLevel::WARNING, 
               state.power.voltage12V, VOLTAGE_12V_MAX,
               "Surtension 12V");
    }
    
    // === SOUS-TENSION 5V - DANGER (<4.5V) ===
    if (state.power.voltage5VValid && state.power.voltage5V < VOLTAGE_5V_MIN) {
      addAlert(AlertType::VOLTAGE_5V_LOW, AlertLevel::DANGER, 
               state.power.voltage5V, VOLTAGE_5V_MIN,
               "5V CRITIQUE!");
    }
    
    // === SURTENSION 5V - WARNING (>5.5V) ===
    if (state.power.voltage5VValid && state.power.voltage5V > VOLTAGE_5V_MAX) {
      addAlert(AlertType::VOLTAGE_5V_HIGH, AlertLevel::WARNING, 
               state.power.voltage5V, VOLTAGE_5V_MAX,
               "Surtension 5V");
    }
    
    // === SUR-COURANT 12V - WARNING ===
    if (state.power.voltage12VValid && state.power.current12V > CURRENT_12V_MAX) {
      addAlert(AlertType::CURRENT_12V_HIGH, AlertLevel::WARNING, 
               state.power.current12V, CURRENT_12V_MAX,
               "Surintensity 12V");
    }
    
    // === SUR-COURANT 5V - WARNING ===
    if (state.power.voltage5VValid && state.power.current5V > CURRENT_5V_MAX) {
      addAlert(AlertType::CURRENT_5V_HIGH, AlertLevel::WARNING, 
               state.power.current5V, CURRENT_5V_MAX,
               "Surintensity 5V");
    }
  }
  
  /**
   * @brief Vérifie les alertes environnementales (température, humidité)
   */
  void checkEnvironmentAlerts() {
    // === TEMPÉRATURE HAUTE - WARNING (>35°C) ===
    if (state.environment.tempIntValid && state.environment.tempInterior > TEMP_WARNING) {
      addAlert(AlertType::TEMP_HIGH, AlertLevel::WARNING, 
               state.environment.tempInterior, TEMP_WARNING,
               "Temp elevee");
    }
    
    // === TEMPÉRATURE BASSE - WARNING (<0°C) ===
    if (state.environment.tempIntValid && state.environment.tempInterior < 0) {
      addAlert(AlertType::TEMP_LOW, AlertLevel::WARNING, 
               state.environment.tempInterior, 0,
               "Temp basse");
    }
    
    // === HUMIDITÉ HAUTE - INFO (>80%) ===
    if (state.environment.humidityValid && state.environment.humidity > HUMIDITY_WARNING) {
      addAlert(AlertType::HUMIDITY_HIGH, AlertLevel::INFO, 
               state.environment.humidity, HUMIDITY_WARNING,
               "Humidite haute");
    }
  }
  
  /**
   * @brief Vérifie les alertes d'horizontalité
   */
  void checkLevelAlerts() {
    // === INCLINAISON - WARNING (>5°) ===
    if (state.level.valid && state.level.totalTilt > TILT_WARNING) {
      addAlert(AlertType::TILT_HIGH, AlertLevel::WARNING, 
               state.level.totalTilt, TILT_WARNING,
               "Inclinaison");
    }
  }
  
  // ============================================
  // GESTION ALERTES
  // ============================================
  
  /**
   * @brief Ajoute une alerte à la liste
   * @param type Type d'alerte
   * @param level Niveau de gravité
   * @param value Valeur ayant déclenché l'alerte
   * @param threshold Seuil franchi
   * @param message Message descriptif
   */
  void addAlert(AlertType type, AlertLevel level, float value, float threshold, const char* message) {
    if (state.alerts.activeAlertCount >= 10) return; // Limite atteinte
    
    uint8_t index = state.alerts.activeAlertCount;
    
    state.alerts.alerts[index].type = type;
    state.alerts.alerts[index].level = level;
    state.alerts.alerts[index].value = value;
    state.alerts.alerts[index].threshold = threshold;
    state.alerts.alerts[index].timestamp = millis();
    state.alerts.alerts[index].active = true;
    state.alerts.alerts[index].message = message;
    
    state.alerts.activeAlertCount++;
    
    // Mettre à jour niveau maximum
    if (level > state.alerts.currentLevel) {
      state.alerts.currentLevel = level;
      state.alerts.primaryAlert = type;
    }
  }
  
  /**
   * @brief Met à jour le mode système selon les alertes
   */
  void updateAlertMode() {
    // Déterminer action selon niveau d'alerte
    switch (state.alerts.currentLevel) {
      case AlertLevel::CRITICAL:
        // Buzzer continu
        state.alerts.buzzerActive = true;
        state.alerts.blockNavigation = true;
        state.mode = SystemMode::MODE_ALERT;
        buzzerInterval = 0; // Continu
        break;
        
      case AlertLevel::DANGER:
        // Bips rapides (200ms)
        state.alerts.buzzerActive = true;
        state.alerts.blockNavigation = true;
        state.mode = SystemMode::MODE_ALERT;
        buzzerInterval = 200;
        break;
        
      case AlertLevel::WARNING:
        // Bips lents (1000ms)
        state.alerts.buzzerActive = true;
        state.alerts.blockNavigation = false;
        buzzerInterval = 1000;
        break;
        
      case AlertLevel::INFO:
        // Pas de son, icône uniquement
        state.alerts.buzzerActive = false;
        state.alerts.blockNavigation = false;
        if (state.mode == SystemMode::MODE_ALERT) {
          state.mode = SystemMode::MODE_NORMAL;
        }
        break;
        
      case AlertLevel::NONE:
        // Tout OK
        state.alerts.buzzerActive = false;
        state.alerts.blockNavigation = false;
        if (state.mode == SystemMode::MODE_ALERT) {
          state.mode = SystemMode::MODE_NORMAL;
        }
        break;
    }
  }
  
  // ============================================
  // GESTION BUZZER
  // ============================================
  
  /**
   * @brief Met à jour le buzzer (à appeler dans loop)
   * 
   * @details
   * Gère les patterns de bips selon le niveau d'alerte :
   * - CRITICAL : Buzzer continu
   * - DANGER : Bips rapides (200ms on/off)
   * - WARNING : Bips lents (1000ms on/off)
   * - INFO/NONE : Silence
   */
  void updateBuzzer() {
    if (!state.sensors.buzzer || !buzzer) return;
    
    unsigned long now = millis();
    
    if (!state.alerts.buzzerActive) {
      // Arrêter le buzzer si actif
      if (buzzerState) {
        buzzer->stop();
        buzzerState = false;
      }
      return;
    }
    
    // CRITICAL : Buzzer continu
    if (buzzerInterval == 0) {
      if (!buzzerState) {
        buzzer->tone(1000, 0); // Son continu
        buzzerState = true;
      }
      return;
    }
    
    // DANGER / WARNING : Bips intermittents
    if (now - lastBuzzerToggle >= buzzerInterval) {
      lastBuzzerToggle = now;
      
      if (buzzerState) {
        buzzer->stop();
        buzzerState = false;
      } else {
        buzzer->tone(1000, buzzerInterval - 50); // Bip avec petite pause
        buzzerState = true;
      }
    }
  }
  
  /**
   * @brief Force l'arrêt du buzzer (pour acquittement temporaire)
   */
  void silenceBuzzer() {
    if (buzzer) {
      buzzer->stop();
      buzzerState = false;
    }
  }
  
  // ============================================
  // GETTERS
  // ============================================
  
  /**
   * @brief Obtient le niveau d'alerte actuel
   * @return Niveau le plus élevé actif
   */
  AlertLevel getCurrentLevel() const {
    return state.alerts.currentLevel;
  }
  
  /**
   * @brief Obtient le type d'alerte prioritaire
   * @return Type de l'alerte la plus grave
   */
  AlertType getPrimaryAlert() const {
    return state.alerts.primaryAlert;
  }
  
  /**
   * @brief Obtient le nombre d'alertes actives
   * @return Nombre d'alertes
   */
  uint8_t getActiveAlertCount() const {
    return state.alerts.activeAlertCount;
  }
  
  /**
   * @brief Vérifie si la navigation est bloquée
   * @return true si bloquée (DANGER ou CRITICAL)
   */
  bool isNavigationBlocked() const {
    return state.alerts.blockNavigation;
  }
  
  /**
   * @brief Vérifie si le buzzer est actif
   * @return true si actif
   */
  bool isBuzzerActive() const {
    return state.alerts.buzzerActive;
  }
  
  /**
   * @brief Obtient une alerte par index
   * @param index Index de l'alerte (0-9)
   * @return Structure Alert
   */
  Alert getAlert(uint8_t index) const {
    if (index < 10) {
      return state.alerts.alerts[index];
    }
    Alert empty;
    empty.active = false;
    return empty;
  }
  
  /**
   * @brief Vérifie si le système est initialisé
   * @return true si initialisé
   */
  bool isInitialized() const {
    return initialized;
  }
};

#endif // ALERT_SYSTEM_H
