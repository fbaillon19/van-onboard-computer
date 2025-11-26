/**
 * @file MQ7Sensor.h
 * @brief Classe de gestion du capteur MQ7 (détection CO - monoxyde de carbone)
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-25
 * 
 * @details
 * Classe découplée pour le capteur de CO MQ7.
 * 
 * Le MQ7 détecte le monoxyde de carbone (CO), gaz inodore et MORTEL.
 * 
 * Caractéristiques :
 * - Plage détection : 20-2000 ppm de CO
 * - Temps de réponse : < 150s
 * - Temps de récupération : < 200s
 * - Cycle de chauffe spécial pour précision
 * 
 * IMPORTANT - Cycle de chauffe du MQ7 :
 * Le MQ7 nécessite un cycle de chauffe alterné pour fonctionner correctement :
 * - Phase HAUTE : 5V pendant 60 secondes (nettoyage du capteur)
 * - Phase BASSE : 1.4V pendant 90 secondes (mesure du CO)
 * 
 * La lecture du CO n'est valide QUE pendant la phase BASSE (1.4V).
 * 
 * Sources de CO dans un van :
 * - Moteur (gaz d'échappement)
 * - Chauffage au gaz mal réglé
 * - Réchaud à gaz
 * - Générateur/groupe électrogène
 * 
 * @warning CO = GAZ MORTEL ! Seuils critiques :
 *   - 50 ppm  : Exposition prolongée dangereuse
 *   - 200 ppm : Maux de tête en 2-3h
 *   - 400 ppm : Maux de tête en 1-2h
 *   - 800 ppm : Convulsions en 45min
 *   - 1600 ppm : MORTEL en 20min
 *   - 3200 ppm : MORTEL en 5-10min
 * 
 * @note Interface analogique, lecture sur pin ADC Arduino
 */

#ifndef MQ7_SENSOR_H
#define MQ7_SENSOR_H

#include <Arduino.h>

// ============================================
// CONFIGURATION MATÉRIELLE
// ============================================
#define MQ7_PREHEAT_TIME        180000    ///< Pré-chauffe initiale (3 min)
#define MQ7_HIGH_PHASE_TIME     60000     ///< Phase haute 5V (60s)
#define MQ7_LOW_PHASE_TIME      90000     ///< Phase basse 1.4V (90s)

// ============================================
// SEUILS DE SÉCURITÉ CO (ppm)
// ============================================
#define MQ7_PPM_SAFE            50        ///< Seuil sécurité (exposition longue OK)
#define MQ7_PPM_WARNING         200       ///< Seuil avertissement (exposition limitée)
#define MQ7_PPM_DANGER          400       ///< Seuil danger (évacuer)
#define MQ7_PPM_CRITICAL        800       ///< Seuil critique (URGENCE)

// ============================================
// TYPES ET STRUCTURES
// ============================================
/**
 * @struct MQ7Data
 * @brief Structure contenant les données du capteur
 */
struct MQ7Data {
  uint16_t rawValue;          ///< Valeur ADC brute (0-1023)
  float voltage;              ///< Tension mesurée (V)
  float rs;                   ///< Résistance capteur (kΩ)
  float ratio;                ///< Ratio Rs/R0
  float ppm;                  ///< Concentration CO (ppm)
  bool valid;                 ///< Mesure valide (phase basse)
  unsigned long timestamp;    ///< Timestamp de la mesure
};

/**
 * @enum MQ7Status
 * @brief État du capteur
 */
enum class MQ7Status {
  NOT_INITIALIZED,   ///< Capteur non initialisé
  PREHEATING,        ///< Pré-chauffe initiale (3 min)
  HEATING_HIGH,      ///< Cycle haute température (60s)
  HEATING_LOW,       ///< Cycle basse température (90s) - LECTURE VALIDE
  READY,             ///< Capteur prêt et stable
  ERROR_READ         ///< Erreur de lecture
};

/**
 * @enum MQ7AlertLevel
 * @brief Niveau d'alerte CO
 */
enum class MQ7AlertLevel {
  SAFE,      ///< Sûr (< 50 ppm)
  WARNING,   ///< Avertissement (50-200 ppm)
  DANGER,    ///< Danger (200-400 ppm)
  CRITICAL   ///< Critique (> 400 ppm) - URGENCE
};

// ============================================
// DÉFINITION CLASSE MQ7Sensor
// ============================================
/**
 * @class MQ7Sensor
 * @brief Classe de gestion du capteur MQ7 (CO)
 * 
 * @details
 * Gère le cycle de chauffe alterné et la lecture du monoxyde de carbone.
 * 
 * IMPORTANT : Le cycle de chauffe nécessite un contrôle de la tension :
 * - Soit via transistor + PWM pour passer de 5V à 1.4V
 * - Soit via module MQ7 avec gestion intégrée
 * 
 * Cette classe suppose un module simple alimenté en 5V constant.
 * Le cycle est géré par timing logiciel pour savoir quand lire.
 * 
 * Pour une gestion PWM complète, voir documentation avancée.
 * 
 * Exemple d'utilisation :
 * @code
 * MQ7Sensor mq7(A0);
 * 
 * void setup() {
 *   if (mq7.begin()) {
 *     Serial.println("MQ7 en pré-chauffe (3 min)...");
 *   }
 * }
 * 
 * void loop() {
 *   mq7.update();
 *   
 *   if (mq7.isReadingValid()) {
 *     float co = mq7.getPPM();
 *     
 *     if (mq7.isDangerous()) {
 *       // ALARME !
 *       Serial.println("⚠️ DANGER CO !");
 *     }
 *   }
 * }
 * @endcode
 */
class MQ7Sensor {
private:
  uint8_t pin;                    ///< Pin ADC
  MQ7Status status;               ///< État du capteur
  MQ7Data currentData;            ///< Données courantes
  
  unsigned long initTime;         ///< Temps d'initialisation
  unsigned long phaseStartTime;   ///< Début phase actuelle
  bool isLowPhase;                ///< true = phase basse (lecture valide)
  
  float r0;                       ///< Résistance R0 (calibration air pur)
  float rl;                       ///< Résistance de charge (kΩ)
  
  uint16_t sampleInterval;        ///< Intervalle échantillonnage (ms)
  unsigned long lastSample;       ///< Dernier échantillon
  
public:
  /**
   * @brief Constructeur
   * @param analogPin Pin ADC du capteur (A0-A15)
   * @param loadResistor Résistance de charge en kΩ (défaut 10kΩ)
   * @param interval Intervalle de lecture en ms (défaut 1000ms)
   */
  MQ7Sensor(uint8_t analogPin, float loadResistor = 10.0, uint16_t interval = 1000)
    : pin(analogPin),
      status(MQ7Status::NOT_INITIALIZED),
      initTime(0),
      phaseStartTime(0),
      isLowPhase(false),
      r0(10.0),  // Valeur par défaut, à calibrer
      rl(loadResistor),
      sampleInterval(interval),
      lastSample(0)
  {
    memset(&currentData, 0, sizeof(MQ7Data));
  }

  // INITIALISATION
  // ------------------------------------------
  /**
   * @brief Initialise le capteur
   * @return true si succès
   * 
   * @details
   * - Configure la pin en entrée
   * - Lance le pré-chauffage (3 minutes minimum)
   * - Le capteur ne sera READY qu'après pré-chauffe complète
   */
  bool begin() {
    pinMode(pin, INPUT);
    
    initTime = millis();
    phaseStartTime = initTime;
    status = MQ7Status::PREHEATING;
    isLowPhase = false;
    
    return true;
  }

  /**
   * @brief Met à jour l'état du capteur
   * @return true si nouvelle lecture effectuée
   * 
   * @details
   * À appeler dans loop() aussi souvent que possible.
   * Gère le cycle de chauffe et les lectures.
   */
  bool update() {
    unsigned long now = millis();
    
    // Gérer les phases de chauffe
    updateHeatingPhase(now);
    
    // Échantillonner si intervalle écoulé et phase basse
    if (now - lastSample >= sampleInterval) {
      lastSample = now;
      return readSensor();
    }
    
    return false;
  }

  /**
   * @brief Force une lecture immédiate
   * @return true si succès
   * 
   * @warning La lecture n'est valide QUE en phase basse !
   */
  bool forceUpdate() {
    lastSample = millis();
    return readSensor();
  }

  // GETTERS - ÉTAT
  // ------------------------------------------
  /**
   * @brief Vérifie si le capteur est prêt
   * @return true si pré-chauffe terminée
   */
  bool isReady() const {
    return status == MQ7Status::READY || 
           status == MQ7Status::HEATING_LOW || 
           status == MQ7Status::HEATING_HIGH;
  }

  /**
   * @brief Vérifie si la lecture est valide
   * @return true si en phase basse (mesure CO possible)
   */
  bool isReadingValid() const {
    return isLowPhase && currentData.valid;
  }

  /**
   * @brief Retourne l'état du capteur
   * @return État actuel
   */
  MQ7Status getStatus() const {
    return status;
  }

  /**
   * @brief Retourne le temps restant de pré-chauffe
   * @return Temps en secondes (0 si terminé)
   */
  uint32_t getPreheatTimeRemaining() const {
    if (status != MQ7Status::PREHEATING) return 0;
    
    unsigned long elapsed = millis() - initTime;
    if (elapsed >= MQ7_PREHEAT_TIME) return 0;
    
    return (MQ7_PREHEAT_TIME - elapsed) / 1000;
  }

  // GETTERS - DONNÉES
  // ------------------------------------------
  /**
   * @brief Retourne les données courantes
   * @return Structure MQ7Data
   */
  MQ7Data getData() const {
    return currentData;
  }

  /**
   * @brief Retourne la concentration CO
   * @return Concentration en ppm
   */
  float getPPM() const {
    return currentData.ppm;
  }

  /**
   * @brief Retourne la valeur ADC brute
   * @return Valeur 0-1023
   */
  uint16_t getRawValue() const {
    return currentData.rawValue;
  }

  /**
   * @brief Retourne la tension mesurée
   * @return Tension en V
   */
  float getVoltage() const {
    return currentData.voltage;
  }

  /**
   * @brief Retourne le ratio Rs/R0
   * @return Ratio
   */
  float getRatio() const {
    return currentData.ratio;
  }

  // DÉTECTION SEUILS
  // ------------------------------------------
  /**
   * @brief Détermine le niveau d'alerte
   * @return Niveau d'alerte (SAFE, WARNING, DANGER, CRITICAL)
   */
  MQ7AlertLevel getAlertLevel() const {
    float ppm = currentData.ppm;
    
    if (ppm >= MQ7_PPM_CRITICAL) return MQ7AlertLevel::CRITICAL;
    if (ppm >= MQ7_PPM_DANGER) return MQ7AlertLevel::DANGER;
    if (ppm >= MQ7_PPM_WARNING) return MQ7AlertLevel::WARNING;
    return MQ7AlertLevel::SAFE;
  }

  /**
   * @brief Vérifie si CO détecté
   * @return true si CO > 50 ppm
   */
  bool isCODetected() const {
    return currentData.ppm > MQ7_PPM_SAFE;
  }

  /**
   * @brief Vérifie si niveau dangereux
   * @return true si CO > 200 ppm
   */
  bool isDangerous() const {
    return currentData.ppm >= MQ7_PPM_WARNING;
  }

  /**
   * @brief Vérifie si niveau critique
   * @return true si CO > 400 ppm (URGENCE)
   */
  bool isCritical() const {
    return currentData.ppm >= MQ7_PPM_DANGER;
  }

  // CALIBRATION
  // ------------------------------------------
  /**
   * @brief Calibre le capteur en air pur
   * @param samples Nombre d'échantillons (défaut 50)
   * @return Valeur R0 calculée
   * 
   * @details
   * À effectuer en extérieur, air pur, après pré-chauffe complète.
   * Le capteur doit être en phase basse pendant la calibration.
   */
  float calibrate(uint16_t samples = 50) {
    if (!isReady()) return r0;
    
    // Attendre phase basse
    while (!isLowPhase) {
      update();
      delay(100);
    }
    
    float sum = 0;
    uint16_t validSamples = 0;
    
    for (uint16_t i = 0; i < samples; i++) {
      uint16_t raw = analogRead(pin);
      float voltage = (raw / 1023.0) * 5.0;
      
      if (voltage > 0.1) {  // Éviter division par zéro
        float rs = ((5.0 - voltage) / voltage) * rl;
        sum += rs;
        validSamples++;
      }
      
      delay(50);
    }
    
    if (validSamples > 0) {
      r0 = sum / validSamples;
    }
    
    return r0;
  }

  /**
   * @brief Définit manuellement R0
   * @param r0Value Valeur R0 en kΩ
   */
  void setR0(float r0Value) {
    r0 = r0Value;
  }

  /**
   * @brief Retourne la valeur R0 actuelle
   * @return R0 en kΩ
   */
  float getR0() const {
    return r0;
  }

  // CONFIGURATION
  // ------------------------------------------
  /**
   * @brief Change l'intervalle d'échantillonnage
   * @param interval Intervalle en ms
   */
  void setSampleInterval(uint16_t interval) {
    sampleInterval = interval;
  }

  /**
   * @brief Retourne l'intervalle d'échantillonnage
   * @return Intervalle en ms
   */
  uint16_t getSampleInterval() const {
    return sampleInterval;
  }

  /**
   * @brief Convertit le statut en texte
   * @param s Statut
   * @return Chaîne de caractères
   */
  static const char* statusToString(MQ7Status s) {
    switch (s) {
      case MQ7Status::NOT_INITIALIZED: return "NON INITIALISE";
      case MQ7Status::PREHEATING:      return "PRE-CHAUFFE";
      case MQ7Status::HEATING_HIGH:    return "CHAUFFE HAUTE";
      case MQ7Status::HEATING_LOW:     return "CHAUFFE BASSE";
      case MQ7Status::READY:           return "PRET";
      case MQ7Status::ERROR_READ:      return "ERREUR LECTURE";
      default:                         return "INCONNU";
    }
  }

  /**
   * @brief Convertit le niveau d'alerte en texte
   * @param level Niveau d'alerte
   * @return Chaîne de caractères
   */
  static const char* alertLevelToString(MQ7AlertLevel level) {
    switch (level) {
      case MQ7AlertLevel::SAFE:     return "SUR";
      case MQ7AlertLevel::WARNING:  return "ATTENTION";
      case MQ7AlertLevel::DANGER:   return "DANGER";
      case MQ7AlertLevel::CRITICAL: return "CRITIQUE";
      default:                      return "INCONNU";
    }
  }

private:
  /**
   * @brief Gère les phases de chauffe
   * @param now Timestamp actuel
   */
  void updateHeatingPhase(unsigned long now) {
    // Si en pré-chauffe
    if (status == MQ7Status::PREHEATING) {
      if (now - initTime >= MQ7_PREHEAT_TIME) {
        // Pré-chauffe terminée, démarrer cycle
        status = MQ7Status::HEATING_HIGH;
        phaseStartTime = now;
        isLowPhase = false;
      }
      return;
    }
    
    // Cycle haute/basse température
    unsigned long phaseElapsed = now - phaseStartTime;
    
    if (isLowPhase) {
      // Phase basse (1.4V) - 90 secondes
      if (phaseElapsed >= MQ7_LOW_PHASE_TIME) {
        // Passer en phase haute
        status = MQ7Status::HEATING_HIGH;
        phaseStartTime = now;
        isLowPhase = false;
      } else {
        status = MQ7Status::HEATING_LOW;
      }
    } else {
      // Phase haute (5V) - 60 secondes
      if (phaseElapsed >= MQ7_HIGH_PHASE_TIME) {
        // Passer en phase basse
        status = MQ7Status::HEATING_LOW;
        phaseStartTime = now;
        isLowPhase = true;
      } else {
        status = MQ7Status::HEATING_HIGH;
      }
    }
    
    // Marquer comme prêt après premier cycle complet
    if (status == MQ7Status::HEATING_LOW || status == MQ7Status::HEATING_HIGH) {
      status = MQ7Status::READY;
    }
  }

  /**
   * @brief Lit les données du capteur
   * @return true si succès
   */
  bool readSensor() {
    // Lire ADC
    currentData.rawValue = analogRead(pin);
    currentData.voltage = (currentData.rawValue / 1023.0) * 5.0;
    
    // Calculer Rs (résistance capteur)
    if (currentData.voltage < 0.1) {
      currentData.valid = false;
      return false;
    }
    
    currentData.rs = ((5.0 - currentData.voltage) / currentData.voltage) * rl;
    
    // Calculer ratio Rs/R0
    currentData.ratio = currentData.rs / r0;
    
    // Calculer PPM CO (formule approximative basée sur datasheet)
    // Courbe : Rs/R0 = 4 * (ppm)^(-0.67)
    // Donc : ppm = (Rs/R0 / 4)^(-1.49)
    
    if (currentData.ratio > 0) {
      currentData.ppm = pow(currentData.ratio / 4.0, -1.49);
    } else {
      currentData.ppm = 0;
    }
    
    // Limiter à la plage du capteur
    if (currentData.ppm < 20) currentData.ppm = 0;
    if (currentData.ppm > 2000) currentData.ppm = 2000;
    
    // Mesure valide uniquement en phase basse
    currentData.valid = isLowPhase;
    currentData.timestamp = millis();
    
    return true;
  }
};

#endif // MQ7_SENSOR_H
