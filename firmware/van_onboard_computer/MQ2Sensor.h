/**
 * @file MQ2Sensor.h
 * @brief Classe de gestion du capteur MQ2 (détection GPL, méthane, fumée)
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-25
 * 
 * @details
 * Classe découplée pour le capteur de gaz MQ2.
 * 
 * Le MQ2 détecte plusieurs types de gaz :
 * - GPL (gaz de pétrole liquéfié : butane, propane)
 * - Méthane (CH4)
 * - Fumée
 * - Hydrogène (H2)
 * - Alcool
 * 
 * Caractéristiques :
 * - Plage détection GPL : 200-10000 ppm
 * - Plage détection CH4 : 200-10000 ppm
 * - Temps de réponse : < 10s
 * - Temps de récupération : < 30s
 * - Chauffage continu 5V (pas de cycle comme MQ7)
 * 
 * Sources de gaz dans un van :
 * - Bouteille de gaz cuisine (butane/propane)
 * - Fuite canalisation gaz
 * - Réchaud/chauffage au gaz
 * - Fumée de cuisson
 * 
 * @warning GPL et méthane sont EXPLOSIFS !
 *   - GPL : 1.8-8.5% dans l'air = EXPLOSIF
 *   - CH4 : 5-15% dans l'air = EXPLOSIF
 *   - Toujours ventiler en cas de détection
 * 
 * @note Interface analogique, lecture sur pin ADC Arduino
 * @note Nécessite 24-48h de première chauffe pour stabilisation
 */

#ifndef MQ2_SENSOR_H
#define MQ2_SENSOR_H

#include <Arduino.h>

// ============================================
// CONFIGURATION MATÉRIELLE
// ============================================
#define MQ2_PREHEAT_TIME        180000    ///< Pré-chauffe initiale (3 min)
#define MQ2_SAMPLE_INTERVAL     1000      ///< Intervalle lecture (1s)

// ============================================
// SEUILS DE DÉTECTION (ppm)
// ============================================
#define MQ2_LPG_THRESHOLD       1000      ///< Seuil détection GPL (ppm)
#define MQ2_CH4_THRESHOLD       1000      ///< Seuil détection méthane (ppm)
#define MQ2_SMOKE_THRESHOLD     500       ///< Seuil détection fumée (ppm)

#define MQ2_LPG_DANGER          3000      ///< Seuil danger GPL (ppm)
#define MQ2_CH4_DANGER          5000      ///< Seuil danger CH4 (ppm)
#define MQ2_SMOKE_DANGER        2000      ///< Seuil danger fumée (ppm)

// ============================================
// TYPES ET STRUCTURES
// ============================================
/**
 * @struct MQ2Data
 * @brief Structure contenant les données du capteur
 */
struct MQ2Data {
  uint16_t rawValue;          ///< Valeur ADC brute (0-1023)
  float voltage;              ///< Tension mesurée (V)
  float rs;                   ///< Résistance capteur (kΩ)
  float ratio;                ///< Ratio Rs/R0
  float lpg;                  ///< Concentration GPL (ppm)
  float methane;              ///< Concentration méthane (ppm)
  float smoke;                ///< Concentration fumée (ppm)
  unsigned long timestamp;    ///< Timestamp de la mesure
};

/**
 * @enum MQ2Status
 * @brief État du capteur
 */
enum class MQ2Status {
  NOT_INITIALIZED,   ///< Capteur non initialisé
  PREHEATING,        ///< Pré-chauffe initiale (3 min)
  READY,             ///< Capteur prêt et stable
  ERROR_READ         ///< Erreur de lecture
};

/**
 * @enum MQ2GasType
 * @brief Type de gaz détecté
 */
enum class MQ2GasType {
  NONE,      ///< Aucun gaz détecté
  LPG,       ///< GPL détecté
  METHANE,   ///< Méthane détecté
  SMOKE,     ///< Fumée détectée
  MULTIPLE   ///< Plusieurs gaz détectés
};

/**
 * @enum MQ2AlertLevel
 * @brief Niveau d'alerte
 */
enum class MQ2AlertLevel {
  SAFE,      ///< Sûr (sous seuils)
  WARNING,   ///< Avertissement (détection faible)
  DANGER,    ///< Danger (concentration élevée)
  CRITICAL   ///< Critique (risque explosion/intoxication)
};

// ============================================
// DÉFINITION CLASSE MQ2Sensor
// ============================================
/**
 * @class MQ2Sensor
 * @brief Classe de gestion du capteur MQ2
 * 
 * @details
 * Gère la lecture et l'interprétation des concentrations de gaz.
 * Contrairement au MQ7, le MQ2 fonctionne en chauffage continu.
 * 
 * Le capteur peut détecter plusieurs gaz simultanément.
 * Les concentrations sont calculées à partir de courbes du datasheet.
 * 
 * Exemple d'utilisation :
 * @code
 * MQ2Sensor mq2(A1);
 * 
 * void setup() {
 *   if (mq2.begin()) {
 *     Serial.println("MQ2 en pré-chauffe (3 min)...");
 *   }
 * }
 * 
 * void loop() {
 *   if (mq2.update()) {
 *     if (mq2.isLPGDetected()) {
 *       Serial.println("⚠️ FUITE GAZ DÉTECTÉE !");
 *     }
 *     
 *     if (mq2.isSmokeDetected()) {
 *       Serial.println("⚠️ FUMÉE DÉTECTÉE !");
 *     }
 *   }
 * }
 * @endcode
 */
class MQ2Sensor {
private:
  uint8_t pin;                    ///< Pin ADC
  MQ2Status status;               ///< État du capteur
  MQ2Data currentData;            ///< Données courantes
  
  unsigned long initTime;         ///< Temps d'initialisation
  unsigned long lastSample;       ///< Dernier échantillon
  uint16_t sampleInterval;        ///< Intervalle échantillonnage (ms)
  
  float r0;                       ///< Résistance R0 (calibration air pur)
  float rl;                       ///< Résistance de charge (kΩ)
  
public:
  /**
   * @brief Constructeur
   * @param analogPin Pin ADC du capteur (A0-A15)
   * @param loadResistor Résistance de charge en kΩ (défaut 10kΩ)
   * @param interval Intervalle de lecture en ms (défaut 1000ms)
   */
  MQ2Sensor(uint8_t analogPin, float loadResistor = 10.0, uint16_t interval = MQ2_SAMPLE_INTERVAL)
    : pin(analogPin),
      status(MQ2Status::NOT_INITIALIZED),
      initTime(0),
      lastSample(0),
      sampleInterval(interval),
      r0(9.8),  // Valeur typique, à calibrer
      rl(loadResistor)
  {
    memset(&currentData, 0, sizeof(MQ2Data));
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
   * - Le capteur sera READY après pré-chauffe complète
   */
  bool begin() {
    pinMode(pin, INPUT);
    
    initTime = millis();
    status = MQ2Status::PREHEATING;
    
    return true;
  }

  /**
   * @brief Met à jour l'état du capteur
   * @return true si nouvelle lecture effectuée
   * 
   * @details
   * À appeler dans loop() régulièrement.
   * Gère le pré-chauffage et les lectures.
   */
  bool update() {
    unsigned long now = millis();
    
    // Vérifier fin pré-chauffe
    if (status == MQ2Status::PREHEATING) {
      if (now - initTime >= MQ2_PREHEAT_TIME) {
        status = MQ2Status::READY;
      }
    }
    
    // Échantillonner si intervalle écoulé
    if (now - lastSample >= sampleInterval) {
      lastSample = now;
      return readSensor();
    }
    
    return false;
  }

  /**
   * @brief Force une lecture immédiate
   * @return true si succès
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
    return status == MQ2Status::READY;
  }

  /**
   * @brief Retourne l'état du capteur
   * @return État actuel
   */
  MQ2Status getStatus() const {
    return status;
  }

  /**
   * @brief Retourne le temps restant de pré-chauffe
   * @return Temps en secondes (0 si terminé)
   */
  uint32_t getPreheatTimeRemaining() const {
    if (status != MQ2Status::PREHEATING) return 0;
    
    unsigned long elapsed = millis() - initTime;
    if (elapsed >= MQ2_PREHEAT_TIME) return 0;
    
    return (MQ2_PREHEAT_TIME - elapsed) / 1000;
  }

  // GETTERS - DONNÉES
  // ------------------------------------------
  /**
   * @brief Retourne les données courantes
   * @return Structure MQ2Data
   */
  MQ2Data getData() const {
    return currentData;
  }

  /**
   * @brief Retourne la concentration GPL
   * @return Concentration en ppm
   */
  float getLPG() const {
    return currentData.lpg;
  }

  /**
   * @brief Retourne la concentration méthane
   * @return Concentration en ppm
   */
  float getMethane() const {
    return currentData.methane;
  }

  /**
   * @brief Retourne la concentration fumée
   * @return Concentration en ppm
   */
  float getSmoke() const {
    return currentData.smoke;
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

  // DÉTECTION GAZ
  // ------------------------------------------
  /**
   * @brief Vérifie si GPL détecté
   * @return true si GPL > seuil
   */
  bool isLPGDetected() const {
    return currentData.lpg > MQ2_LPG_THRESHOLD;
  }

  /**
   * @brief Vérifie si méthane détecté
   * @return true si CH4 > seuil
   */
  bool isMethaneDetected() const {
    return currentData.methane > MQ2_CH4_THRESHOLD;
  }

  /**
   * @brief Vérifie si fumée détectée
   * @return true si fumée > seuil
   */
  bool isSmokeDetected() const {
    return currentData.smoke > MQ2_SMOKE_THRESHOLD;
  }

  /**
   * @brief Vérifie si un gaz quelconque est détecté
   * @return true si détection
   */
  bool isAnyGasDetected() const {
    return isLPGDetected() || isMethaneDetected() || isSmokeDetected();
  }

  /**
   * @brief Détermine le type de gaz dominant
   * @return Type de gaz détecté
   */
  MQ2GasType getDominantGas() const {
    if (!isAnyGasDetected()) return MQ2GasType::NONE;
    
    // Compter combien de gaz détectés
    uint8_t count = 0;
    if (isLPGDetected()) count++;
    if (isMethaneDetected()) count++;
    if (isSmokeDetected()) count++;
    
    if (count > 1) return MQ2GasType::MULTIPLE;
    
    // Déterminer le dominant
    if (isLPGDetected()) return MQ2GasType::LPG;
    if (isMethaneDetected()) return MQ2GasType::METHANE;
    if (isSmokeDetected()) return MQ2GasType::SMOKE;
    
    return MQ2GasType::NONE;
  }

  /**
   * @brief Détermine le niveau d'alerte global
   * @return Niveau d'alerte
   */
  MQ2AlertLevel getAlertLevel() const {
    if (!isAnyGasDetected()) return MQ2AlertLevel::SAFE;
    
    // Vérifier niveaux critiques
    if (currentData.lpg >= MQ2_LPG_DANGER || 
        currentData.methane >= MQ2_CH4_DANGER ||
        currentData.smoke >= MQ2_SMOKE_DANGER) {
      return MQ2AlertLevel::CRITICAL;
    }
    
    // Vérifier niveaux dangereux (>50% du seuil danger)
    if (currentData.lpg >= MQ2_LPG_DANGER * 0.5 ||
        currentData.methane >= MQ2_CH4_DANGER * 0.5 ||
        currentData.smoke >= MQ2_SMOKE_DANGER * 0.5) {
      return MQ2AlertLevel::DANGER;
    }
    
    // Détection faible
    return MQ2AlertLevel::WARNING;
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
   * Capteur éloigné de toute source de gaz.
   */
  float calibrate(uint16_t samples = 50) {
    if (!isReady()) return r0;
    
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
      // R0 = Rs dans l'air pur (ratio Rs/R0 ≈ 9.8 selon datasheet)
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
  static const char* statusToString(MQ2Status s) {
    switch (s) {
      case MQ2Status::NOT_INITIALIZED: return "NON INITIALISE";
      case MQ2Status::PREHEATING:      return "PRE-CHAUFFE";
      case MQ2Status::READY:           return "PRET";
      case MQ2Status::ERROR_READ:      return "ERREUR LECTURE";
      default:                         return "INCONNU";
    }
  }

  /**
   * @brief Convertit le type de gaz en texte
   * @param gas Type de gaz
   * @return Chaîne de caractères
   */
  static const char* gasTypeToString(MQ2GasType gas) {
    switch (gas) {
      case MQ2GasType::NONE:     return "Aucun";
      case MQ2GasType::LPG:      return "GPL";
      case MQ2GasType::METHANE:  return "Methane";
      case MQ2GasType::SMOKE:    return "Fumee";
      case MQ2GasType::MULTIPLE: return "Plusieurs";
      default:                   return "Inconnu";
    }
  }

  /**
   * @brief Convertit le niveau d'alerte en texte
   * @param level Niveau d'alerte
   * @return Chaîne de caractères
   */
  static const char* alertLevelToString(MQ2AlertLevel level) {
    switch (level) {
      case MQ2AlertLevel::SAFE:     return "SUR";
      case MQ2AlertLevel::WARNING:  return "ATTENTION";
      case MQ2AlertLevel::DANGER:   return "DANGER";
      case MQ2AlertLevel::CRITICAL: return "CRITIQUE";
      default:                      return "INCONNU";
    }
  }

private:
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
      return false;
    }
    
    currentData.rs = ((5.0 - currentData.voltage) / currentData.voltage) * rl;
    
    // Calculer ratio Rs/R0
    currentData.ratio = currentData.rs / r0;
    
    // Calculer concentrations selon courbes datasheet MQ2
    // Les formules sont approximatives basées sur les graphiques
    
    // GPL (LPG) : Rs/R0 ≈ 0.5 à 1000ppm
    // Courbe : Rs/R0 = 2.5 * (ppm)^(-0.48)
    if (currentData.ratio > 0.1) {
      currentData.lpg = pow(currentData.ratio / 2.5, -2.08);
    } else {
      currentData.lpg = 0;
    }
    
    // Méthane (CH4) : Rs/R0 ≈ 0.7 à 1000ppm
    // Courbe : Rs/R0 = 3.3 * (ppm)^(-0.38)
    if (currentData.ratio > 0.15) {
      currentData.methane = pow(currentData.ratio / 3.3, -2.63);
    } else {
      currentData.methane = 0;
    }
    
    // Fumée : Rs/R0 ≈ 0.4 à 1000ppm
    // Courbe : Rs/R0 = 2.0 * (ppm)^(-0.45)
    if (currentData.ratio > 0.1) {
      currentData.smoke = pow(currentData.ratio / 2.0, -2.22);
    } else {
      currentData.smoke = 0;
    }
    
    // Limiter aux plages du capteur
    if (currentData.lpg < 200) currentData.lpg = 0;
    if (currentData.lpg > 10000) currentData.lpg = 10000;
    
    if (currentData.methane < 200) currentData.methane = 0;
    if (currentData.methane > 10000) currentData.methane = 10000;
    
    if (currentData.smoke < 100) currentData.smoke = 0;
    if (currentData.smoke > 10000) currentData.smoke = 10000;
    
    currentData.timestamp = millis();
    
    return true;
  }
};

#endif // MQ2_SENSOR_H
