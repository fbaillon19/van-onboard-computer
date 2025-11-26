/**
 * @file LEDManager.h
 * @brief Gestionnaire du bandeau LED WS2812B
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-26
 * 
 * @details
 * Classe responsable de :
 * - Gestion du bandeau 8 LEDs WS2812B
 * - Barre de puissance (LEDs 0-3) avec gradient vert→rouge
 * - Indicateurs gaz : CO (LED 4), GPL (LED 5)
 * - Indicateurs tensions : 12V (LED 6), 5V (LED 7)
 * - Animation en mode alerte
 * 
 * Répartition des LEDs :
 * LED 0-3 : Barre puissance (gradient selon charge)
 * LED 4   : CO (Vert/Jaune/Orange/Rouge selon ppm)
 * LED 5   : GPL (idem)
 * LED 6   : 12V (Rouge/Orange/Vert/Bleu selon tension)
 * LED 7   : 5V (idem)
 */

#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include <FastLED.h>
#include "config.h"
#include "SystemData.h"

// ============================================
// CLASSE LEDManager
// ============================================
/**
 * @class LEDManager
 * @brief Gestionnaire du bandeau LED WS2812B
 */
class LEDManager {
private:
  // LEDs
  CRGB leds[LED_COUNT];
  
  // Référence à l'état système
  SystemState& state;
  
  // Timing
  unsigned long lastUpdate;
  unsigned long lastBlink;
  
  // Animation alerte
  bool blinkState;
  
  // Flags
  bool initialized;
  
  // ============================================
  // COULEURS PRÉDÉFINIES
  // ============================================
  
  // États OK
  const CRGB COLOR_OK = CRGB::Green;
  const CRGB COLOR_INFO = CRGB::Blue;
  
  // Niveaux de concentration gaz
  const CRGB COLOR_GAS_SAFE = CRGB::Green;      // Sécuritaire
  const CRGB COLOR_GAS_LOW = CRGB::Yellow;      // Détection faible
  const CRGB COLOR_GAS_MEDIUM = CRGB::Orange;   // Attention
  const CRGB COLOR_GAS_HIGH = CRGB::Red;        // Danger
  
  // Niveaux de tension
  const CRGB COLOR_VOLTAGE_LOW = CRGB::Red;     // Sous-tension
  const CRGB COLOR_VOLTAGE_WARN = CRGB::Orange; // Avertissement
  const CRGB COLOR_VOLTAGE_OK = CRGB::Green;    // Normal
  const CRGB COLOR_VOLTAGE_HIGH = CRGB::Blue;   // En charge
  
  // Alertes
  const CRGB COLOR_ALERT_CRITICAL = CRGB::Red;
  const CRGB COLOR_ALERT_DANGER = CRGB::Orange;
  const CRGB COLOR_ALERT_WARNING = CRGB::Yellow;
  
  // Éteint
  const CRGB COLOR_OFF = CRGB::Black;
  
public:
  /**
   * @brief Constructeur
   * @param sysState Référence à l'état système
   */
  LEDManager(SystemState& sysState) 
    : state(sysState),
      lastUpdate(0),
      lastBlink(0),
      blinkState(false),
      initialized(false)
  {
  }
  
  // ============================================
  // INITIALISATION
  // ============================================
  
  /**
   * @brief Initialise le bandeau LED
   * @return true si succès
   */
  bool begin() {
    DEBUG_PRINTLN(F("=== INITIALISATION LED WS2812B ==="));
    
    // Initialiser FastLED
    FastLED.addLeds<WS2812B, PIN_WS2812B, GRB>(leds, LED_COUNT);
    FastLED.setBrightness(LED_BRIGHTNESS);
    
    // Test : toutes les LEDs en blanc
    fill_solid(leds, LED_COUNT, CRGB::White);
    FastLED.show();
    delay(500);
    
    // Éteindre
    fill_solid(leds, LED_COUNT, CRGB::Black);
    FastLED.show();
    
    state.sensors.leds = true;
    initialized = true;
    
    DEBUG_PRINTLN(F("[OK] LED WS2812B initialisees"));
    return true;
  }
  
  // ============================================
  // MISE À JOUR
  // ============================================
  
  /**
   * @brief Met à jour l'affichage des LEDs
   * 
   * @details
   * À appeler régulièrement dans loop().
   * Rafraîchit selon INTERVAL_LEDS (50ms).
   */
  void update() {
    if (!initialized) return;
    
    unsigned long now = millis();
    
    // Limiter fréquence rafraîchissement
    if (now - lastUpdate < INTERVAL_LEDS) {
      return;
    }
    lastUpdate = now;
    
    // Mode alerte : animation clignotante
    if (state.mode == SystemMode::MODE_ALERT && 
        state.alerts.currentLevel >= AlertLevel::DANGER) {
      updateAlertAnimation();
      return;
    }
    
    // Mode normal : affichage standard
    updatePowerBar();
    updateGasIndicators();
    updateVoltageIndicators();
    
    FastLED.show();
  }
  
  /**
   * @brief Met à jour la barre de puissance (LEDs 0-3)
   * 
   * @details
   * Affiche un gradient vert→jaune→rouge selon la charge totale.
   * Calcul basé sur la puissance totale (12V + 5V).
   */
  void updatePowerBar() {
    // Calculer pourcentage de charge (estimation)
    // Hypothèse : charge max ~200W (réglable selon installation)
    const float MAX_POWER = 200.0; // Watts
    float powerPercent = (state.power.powerTotal / MAX_POWER) * 100.0;
    powerPercent = constrain(powerPercent, 0.0, 100.0);
    
    // Nombre de LEDs à allumer (0-4)
    uint8_t activeLeds = map(powerPercent, 0, 100, 0, LED_POWER_COUNT);
    
    // Allumer les LEDs avec gradient
    for (uint8_t i = 0; i < LED_POWER_COUNT; i++) {
      if (i < activeLeds) {
        // Gradient : Vert (0%) → Jaune (50%) → Rouge (100%)
        float ledPercent = (float)i / (LED_POWER_COUNT - 1) * 100.0;
        leds[LED_POWER_START + i] = getPowerColor(ledPercent);
      } else {
        leds[LED_POWER_START + i] = COLOR_OFF;
      }
    }
  }
  
  /**
   * @brief Met à jour les indicateurs gaz (LEDs 4-5)
   */
  void updateGasIndicators() {
    // LED 4 : CO
    if (state.safety.coValid && state.safety.mq7Preheated) {
      leds[LED_CO] = getGasColor(state.safety.coPPM, 
                                  CO_THRESHOLD_INFO,
                                  CO_THRESHOLD_WARNING,
                                  CO_THRESHOLD_DANGER);
    } else {
      leds[LED_CO] = COLOR_OFF; // Pré-chauffage en cours
    }
    
    // LED 5 : GPL
    if (state.safety.gplValid && state.safety.mq2Preheated) {
      leds[LED_GPL] = getGasColor(state.safety.gplPPM, 
                                   GPL_THRESHOLD_INFO,
                                   GPL_THRESHOLD_WARNING,
                                   GPL_THRESHOLD_DANGER);
    } else {
      leds[LED_GPL] = COLOR_OFF; // Pré-chauffage en cours
    }
  }
  
  /**
   * @brief Met à jour les indicateurs tensions (LEDs 6-7)
   */
  void updateVoltageIndicators() {
    // LED 6 : 12V
    if (state.power.voltage12VValid) {
      leds[LED_VOLTAGE_12V] = getVoltageColor(state.power.voltage12V,
                                               VOLTAGE_12V_MIN,
                                               VOLTAGE_12V_WARNING,
                                               VOLTAGE_12V_NOMINAL,
                                               VOLTAGE_12V_CHARGING);
    } else {
      leds[LED_VOLTAGE_12V] = COLOR_OFF;
    }
    
    // LED 7 : 5V
    if (state.power.voltage5VValid) {
      leds[LED_VOLTAGE_5V] = getVoltageColor(state.power.voltage5V,
                                              VOLTAGE_5V_MIN,
                                              VOLTAGE_5V_MIN + 0.2,
                                              VOLTAGE_5V_NOMINAL,
                                              VOLTAGE_5V_MAX);
    } else {
      leds[LED_VOLTAGE_5V] = COLOR_OFF;
    }
  }
  
  /**
   * @brief Animation en mode alerte
   * 
   * @details
   * Toutes les LEDs clignotent en rouge (CRITICAL) ou orange (DANGER).
   */
  void updateAlertAnimation() {
    unsigned long now = millis();
    
    // Clignotement toutes les 500ms
    if (now - lastBlink >= ALERT_BLINK_INTERVAL) {
      lastBlink = now;
      blinkState = !blinkState;
    }
    
    CRGB alertColor;
    if (state.alerts.currentLevel == AlertLevel::CRITICAL) {
      alertColor = COLOR_ALERT_CRITICAL;
    } else {
      alertColor = COLOR_ALERT_DANGER;
    }
    
    if (blinkState) {
      fill_solid(leds, LED_COUNT, alertColor);
    } else {
      fill_solid(leds, LED_COUNT, COLOR_OFF);
    }
    
    FastLED.show();
  }
  
  // ============================================
  // CALCUL COULEURS
  // ============================================
  
  /**
   * @brief Calcule la couleur pour la barre de puissance
   * @param percent Pourcentage de charge (0-100)
   * @return Couleur CRGB
   */
  CRGB getPowerColor(float percent) {
    if (percent < 25) {
      // 0-25% : Vert
      return CRGB::Green;
    } else if (percent < 50) {
      // 25-50% : Vert → Jaune
      return blend(CRGB::Green, CRGB::Yellow, map(percent, 25, 50, 0, 255));
    } else if (percent < 75) {
      // 50-75% : Jaune → Orange
      return blend(CRGB::Yellow, CRGB::Orange, map(percent, 50, 75, 0, 255));
    } else {
      // 75-100% : Orange → Rouge
      return blend(CRGB::Orange, CRGB::Red, map(percent, 75, 100, 0, 255));
    }
  }
  
  /**
   * @brief Calcule la couleur pour un indicateur gaz
   * @param ppm Concentration en ppm
   * @param thresholdInfo Seuil INFO
   * @param thresholdWarning Seuil WARNING
   * @param thresholdDanger Seuil DANGER
   * @return Couleur CRGB
   */
  CRGB getGasColor(float ppm, float thresholdInfo, float thresholdWarning, float thresholdDanger) {
    if (ppm >= thresholdDanger) {
      return COLOR_GAS_HIGH;        // Rouge : Danger
    } else if (ppm >= thresholdWarning) {
      return COLOR_GAS_MEDIUM;      // Orange : Attention
    } else if (ppm >= thresholdInfo) {
      return COLOR_GAS_LOW;         // Jaune : Détection
    } else {
      return COLOR_GAS_SAFE;        // Vert : Sécuritaire
    }
  }
  
  /**
   * @brief Calcule la couleur pour un indicateur tension
   * @param voltage Tension en V
   * @param min Tension minimale
   * @param warning Seuil avertissement
   * @param nominal Tension nominale
   * @param charging Tension en charge
   * @return Couleur CRGB
   */
  CRGB getVoltageColor(float voltage, float min, float warning, float nominal, float charging) {
    if (voltage < min) {
      return COLOR_VOLTAGE_LOW;     // Rouge : Sous-tension critique
    } else if (voltage < warning) {
      return COLOR_VOLTAGE_WARN;    // Orange : Avertissement
    } else if (voltage < charging) {
      return COLOR_VOLTAGE_OK;      // Vert : Normal
    } else {
      return COLOR_VOLTAGE_HIGH;    // Bleu : En charge
    }
  }
  
  // ============================================
  // EFFETS SPÉCIAUX
  // ============================================
  
  /**
   * @brief Affiche une animation de démarrage
   */
  void bootAnimation() {
    if (!initialized) return;
    
    // Balayage gauche → droite
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      leds[i] = CRGB::Blue;
      FastLED.show();
      delay(100);
      leds[i] = COLOR_OFF;
    }
    
    // Flash final
    fill_solid(leds, LED_COUNT, CRGB::Green);
    FastLED.show();
    delay(200);
    fill_solid(leds, LED_COUNT, COLOR_OFF);
    FastLED.show();
  }
  
  /**
   * @brief Affiche une animation de pré-chauffage
   * @param percent Pourcentage de pré-chauffage (0-100)
   */
  void preheatAnimation(uint8_t percent) {
    if (!initialized) return;
    
    // Nombre de LEDs à allumer
    uint8_t activeLeds = map(percent, 0, 100, 0, LED_COUNT);
    
    // Allumer progressivement en orange
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      if (i < activeLeds) {
        leds[i] = CRGB::Orange;
      } else {
        leds[i] = COLOR_OFF;
      }
    }
    
    FastLED.show();
  }
  
  /**
   * @brief Affiche un pattern de test
   */
  void testPattern() {
    if (!initialized) return;
    
    // Afficher chaque LED en couleur différente
    leds[0] = CRGB::Red;
    leds[1] = CRGB::Green;
    leds[2] = CRGB::Blue;
    leds[3] = CRGB::Yellow;
    leds[4] = CRGB::Cyan;
    leds[5] = CRGB::Magenta;
    leds[6] = CRGB::White;
    leds[7] = CRGB::Orange;
    
    FastLED.show();
    delay(2000);
    
    // Éteindre
    fill_solid(leds, LED_COUNT, COLOR_OFF);
    FastLED.show();
  }
  
  /**
   * @brief Éteint toutes les LEDs
   */
  void clear() {
    if (!initialized) return;
    fill_solid(leds, LED_COUNT, COLOR_OFF);
    FastLED.show();
  }
  
  /**
   * @brief Définit la luminosité
   * @param brightness Luminosité (0-255)
   */
  void setBrightness(uint8_t brightness) {
    if (!initialized) return;
    FastLED.setBrightness(brightness);
    FastLED.show();
  }
  
  /**
   * @brief Définit une LED spécifique
   * @param index Index de la LED (0-7)
   * @param color Couleur
   */
  void setLED(uint8_t index, CRGB color) {
    if (!initialized || index >= LED_COUNT) return;
    leds[index] = color;
    FastLED.show();
  }
  
  /**
   * @brief Définit toutes les LEDs
   * @param color Couleur
   */
  void setAll(CRGB color) {
    if (!initialized) return;
    fill_solid(leds, LED_COUNT, color);
    FastLED.show();
  }
  
  // ============================================
  // GETTERS
  // ============================================
  
  /**
   * @brief Vérifie si les LEDs sont initialisées
   * @return true si initialisées
   */
  bool isInitialized() const {
    return initialized;
  }
  
  /**
   * @brief Obtient la luminosité actuelle
   * @return Luminosité (0-255)
   */
  uint8_t getBrightness() const {
    return FastLED.getBrightness();
  }
};

#endif // LED_MANAGER_H
