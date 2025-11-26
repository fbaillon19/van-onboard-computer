/**
 * @file KY040Encoder.h
 * @brief Classe de gestion de l'encodeur rotatif KY040
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-21
 * 
 * @details
 * Utilisable dans le programme de test et le programme principal.
 * 
 * Principes de conception :
 * - Gestion des rotations (CW/CCW) et du bouton poussoir
 * - Détection des changements avec debouncing
 * - Responsabilité unique : lire et traiter événements encodeur
 */

#ifndef KY040_ENCODER_H
#define KY040_ENCODER_H

#include <Arduino.h>

// ============================================
// CONFIGURATION
// ============================================
#define KY040_DEBOUNCE_DELAY    5     ///< Délai anti-rebond rotation (ms)
#define KY040_BUTTON_DEBOUNCE   50    ///< Délai anti-rebond bouton (ms)
#define KY040_LONG_PRESS_TIME   1000  ///< Durée appui long (ms)

// ============================================
// TYPES ET STRUCTURES
// ============================================
/**
 * @enum RotationDirection
 * @brief Direction de rotation de l'encodeur
 */
enum class RotationDirection {
  NONE = 0,         ///< Pas de rotation
  CLOCKWISE = 1,    ///< Rotation horaire (CW)
  COUNTER_CLOCKWISE = -1  ///< Rotation anti-horaire (CCW)
};

/**
 * @enum ButtonEvent
 * @brief Type d'événement bouton
 */
enum class ButtonEvent {
  NONE,           ///< Aucun événement
  PRESSED,        ///< Bouton appuyé
  RELEASED,       ///< Bouton relâché
  CLICKED,        ///< Clic court
  LONG_PRESS      ///< Appui long
};

/**
 * @struct EncoderData
 * @brief Structure contenant l'état de l'encodeur
 */
struct EncoderData {
  int32_t position;                 ///< Position actuelle
  RotationDirection lastDirection;  ///< Dernière direction
  ButtonEvent lastButtonEvent;      ///< Dernier événement bouton
  bool buttonPressed;               ///< État actuel du bouton
  unsigned long timestamp;          ///< Timestamp de la dernière MAJ
};

// ============================================
// DEFINITION CLASSE KY040Encoder
// ============================================
/**
 * @class KY040Encoder
 * @brief Classe de gestion de l'encodeur rotatif KY040
 * 
 * Cette classe gère un encodeur rotatif avec bouton poussoir intégré.
 * Elle détecte les rotations (CW/CCW) et les événements bouton.
 */
class KY040Encoder {
private:
  // Pins
  uint8_t pinCLK;               ///< Broche CLK (sortie A)
  uint8_t pinDT;                ///< Broche DT (sortie B)
  uint8_t pinSW;                ///< Broche SW (bouton)
  
  // État rotation
  int32_t position;             ///< Position courante
  volatile uint8_t lastStateCLK;     ///< Dernier état CLK
  volatile uint8_t lastStateDT;      ///< Dernier état DT
  volatile bool rotationDetected;    ///< Flag rotation détectée
  volatile RotationDirection lastDirection;  ///< Dernière direction
  unsigned long lastRotationTime;    ///< Timestamp dernière rotation
  
  // État bouton
  bool buttonState;             ///< État actuel du bouton
  bool lastButtonState;         ///< État précédent du bouton
  unsigned long lastButtonChange;    ///< Timestamp dernier changement
  unsigned long buttonPressTime;     ///< Timestamp début appui
  bool longPressDetected;       ///< Flag appui long détecté
  ButtonEvent lastButtonEvent;  ///< Dernier événement bouton
  
  // Configuration
  bool initialized;             ///< État initialisation
  bool reverseDirection;        ///< Inverser sens de rotation
  int32_t minPosition;          ///< Position minimale
  int32_t maxPosition;          ///< Position maximale
  bool limitEnabled;            ///< Activer limites position
  
  /**
   * @brief Lit l'état actuel des pins CLK et DT
   */
  void readEncoderState() {
    uint8_t clkState = digitalRead(pinCLK);
    uint8_t dtState = digitalRead(pinDT);
    
    // Détection de rotation (transition sur CLK)
    if (clkState != lastStateCLK) {
      unsigned long now = millis();
      
      // Anti-rebond
      if (now - lastRotationTime >= KY040_DEBOUNCE_DELAY) {
        lastRotationTime = now;
        
        // Déterminer direction
        if (clkState != dtState) {
          // Rotation horaire
          lastDirection = reverseDirection ? 
            RotationDirection::COUNTER_CLOCKWISE : 
            RotationDirection::CLOCKWISE;
          
          if (!limitEnabled || position < maxPosition) {
            position++;
          }
        } else {
          // Rotation anti-horaire
          lastDirection = reverseDirection ? 
            RotationDirection::CLOCKWISE : 
            RotationDirection::COUNTER_CLOCKWISE;
          
          if (!limitEnabled || position > minPosition) {
            position--;
          }
        }
        
        rotationDetected = true;
      }
    }
    
    lastStateCLK = clkState;
    lastStateDT = dtState;
  }
  
  /**
   * @brief Traite les événements du bouton
   */
  void processButton() {
    unsigned long now = millis();
    bool currentState = digitalRead(pinSW) == LOW;  // Actif bas
    
    // Anti-rebond
    if (now - lastButtonChange < KY040_BUTTON_DEBOUNCE) {
      return;
    }
    
    // Changement d'état détecté
    if (currentState != buttonState) {
      lastButtonChange = now;
      buttonState = currentState;
      
      if (buttonState) {
        // Bouton appuyé
        buttonPressTime = now;
        longPressDetected = false;
        lastButtonEvent = ButtonEvent::PRESSED;
      } else {
        // Bouton relâché
        unsigned long pressDuration = now - buttonPressTime;
        
        if (pressDuration >= KY040_LONG_PRESS_TIME) {
          lastButtonEvent = ButtonEvent::LONG_PRESS;
        } else {
          lastButtonEvent = ButtonEvent::CLICKED;
        }
      }
    }
    
    // Détection appui long en cours
    if (buttonState && !longPressDetected) {
      if (now - buttonPressTime >= KY040_LONG_PRESS_TIME) {
        longPressDetected = true;
        lastButtonEvent = ButtonEvent::LONG_PRESS;
      }
    }
  }

public:
  /**
   * @brief Constructeur
   * @param clk Broche CLK (sortie A)
   * @param dt Broche DT (sortie B)
   * @param sw Broche SW (bouton)
   */
  KY040Encoder(uint8_t clk, uint8_t dt, uint8_t sw)
    : pinCLK(clk),
      pinDT(dt),
      pinSW(sw),
      position(0),
      lastStateCLK(HIGH),
      lastStateDT(HIGH),
      rotationDetected(false),
      lastDirection(RotationDirection::NONE),
      lastRotationTime(0),
      buttonState(false),
      lastButtonState(false),
      lastButtonChange(0),
      buttonPressTime(0),
      longPressDetected(false),
      lastButtonEvent(ButtonEvent::NONE),
      initialized(false),
      reverseDirection(false),
      minPosition(INT32_MIN),
      maxPosition(INT32_MAX),
      limitEnabled(false)
  {}

  // INITIALISATION
  // ------------------------------------------
  /**
   * @brief Initialise l'encodeur
   * @return true si succès
   */
  bool begin() {
    // Configuration des pins
    pinMode(pinCLK, INPUT_PULLUP);
    pinMode(pinDT, INPUT_PULLUP);
    pinMode(pinSW, INPUT_PULLUP);
    
    // Lecture état initial
    lastStateCLK = digitalRead(pinCLK);
    lastStateDT = digitalRead(pinDT);
    buttonState = digitalRead(pinSW) == LOW;
    
    initialized = true;
    return true;
  }

  // LECTURE DONNÉES
  // ------------------------------------------
  /**
   * @brief Met à jour l'état de l'encodeur
   * 
   * @details À appeler dans loop() aussi souvent que possible
   * pour détecter les rotations et événements bouton.
   */
  void update() {
    if (!initialized) return;
    
    readEncoderState();
    processButton();
  }

  /**
   * @brief Obtient la position actuelle
   * @return Position de l'encodeur
   */
  int32_t getPosition() const {
    return position;
  }

  /**
   * @brief Définit la position actuelle
   * @param pos Nouvelle position
   */
  void setPosition(int32_t pos) {
    position = pos;
  }

  /**
   * @brief Réinitialise la position à zéro
   */
  void resetPosition() {
    position = 0;
  }

  /**
   * @brief Vérifie si une rotation a été détectée
   * @return true si rotation depuis dernier appel
   * 
   * @note Cette fonction consomme le flag (reset automatique)
   */
  bool hasRotated() {
    if (rotationDetected) {
      rotationDetected = false;
      return true;
    }
    return false;
  }

  /**
   * @brief Obtient la dernière direction de rotation
   * @return Direction (CW, CCW ou NONE)
   */
  RotationDirection getDirection() const {
    return lastDirection;
  }

  /**
   * @brief Vérifie si le bouton est appuyé
   * @return true si appuyé
   */
  bool isButtonPressed() const {
    return buttonState;
  }

  /**
   * @brief Obtient le dernier événement bouton
   * @return Type d'événement
   * 
   * @note Cette fonction consomme l'événement (reset automatique)
   */
  ButtonEvent getButtonEvent() {
    ButtonEvent event = lastButtonEvent;
    lastButtonEvent = ButtonEvent::NONE;
    return event;
  }

  /**
   * @brief Vérifie si un événement bouton est disponible
   * @return true si événement en attente
   */
  bool hasButtonEvent() const {
    return lastButtonEvent != ButtonEvent::NONE;
  }

  /**
   * @brief Inverse le sens de rotation
   * @param reverse true pour inverser
   */
  void setReverseDirection(bool reverse) {
    reverseDirection = reverse;
  }

  /**
   * @brief Active les limites de position
   * @param min Position minimale
   * @param max Position maximale
   */
  void setLimits(int32_t min, int32_t max) {
    minPosition = min;
    maxPosition = max;
    limitEnabled = true;
    
    // Ajuster position si hors limites
    if (position < minPosition) position = minPosition;
    if (position > maxPosition) position = maxPosition;
  }

  /**
   * @brief Désactive les limites de position
   */
  void removeLimits() {
    limitEnabled = false;
    minPosition = INT32_MIN;
    maxPosition = INT32_MAX;
  }

  /**
   * @brief Obtient les limites actuelles
   * @param min [out] Position minimale
   * @param max [out] Position maximale
   * @return true si limites actives
   */
  bool getLimits(int32_t &min, int32_t &max) const {
    min = minPosition;
    max = maxPosition;
    return limitEnabled;
  }

  /**
   * @brief Vérifie si l'encodeur est initialisé
   * @return true si initialisé
   */
  bool isInitialized() const {
    return initialized;
  }

  /**
   * @brief Obtient toutes les données de l'encodeur
   * @return Structure EncoderData
   */
  EncoderData getData() const {
    EncoderData data;
    data.position = position;
    data.lastDirection = lastDirection;
    data.lastButtonEvent = lastButtonEvent;
    data.buttonPressed = buttonState;
    data.timestamp = millis();
    return data;
  }

  /**
   * @brief Convertit la direction en texte
   * @param dir Direction
   * @return Chaîne de caractères
   */
  static const char* directionToString(RotationDirection dir) {
    switch (dir) {
      case RotationDirection::CLOCKWISE:         return "CW";
      case RotationDirection::COUNTER_CLOCKWISE: return "CCW";
      case RotationDirection::NONE:              return "NONE";
      default:                                   return "UNKNOWN";
    }
  }

  /**
   * @brief Convertit l'événement bouton en texte
   * @param event Événement
   * @return Chaîne de caractères
   */
  static const char* buttonEventToString(ButtonEvent event) {
    switch (event) {
      case ButtonEvent::PRESSED:    return "PRESSED";
      case ButtonEvent::RELEASED:   return "RELEASED";
      case ButtonEvent::CLICKED:    return "CLICKED";
      case ButtonEvent::LONG_PRESS: return "LONG_PRESS";
      case ButtonEvent::NONE:       return "NONE";
      default:                      return "UNKNOWN";
    }
  }
};

#endif // KY040_ENCODER_H
