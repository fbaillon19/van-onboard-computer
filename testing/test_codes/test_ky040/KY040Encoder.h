/**
 * @file KY040Encoder.h
 * @brief Classe de gestion de l'encodeur rotatif KY040 avec librairie Encoder
 * @author Frédéric BAILLON
 * @version 3.0.0 - Utilisation de la librairie Encoder de Paul Stoffregen
 * @date 2024-12-05
 * 
 * @details
 * VERSION ULTRA-FIABLE basée sur la librairie Encoder de Paul Stoffregen.
 * Cette librairie est la référence dans l'Arduino pour les encodeurs rotatifs.
 * 
 * IMPORTANT:
 * - Nécessite l'installation de la librairie "Encoder" par Paul Stoffregen
 * - Dans l'IDE Arduino : Croquis > Inclure une bibliothèque > Gérer les bibliothèques
 * - Chercher "Encoder" et installer celle de Paul Stoffregen
 * Ajoutez des condensateurs céramiques 0.1µF (100nF) entre :
 * CLK ---||--- GND et DT ---||--- GND
 * Pour filtrer le bruit électrique à la source.
 * 
 * Avantages:
 * - Code ultra-optimisé en assembleur pour de nombreuses plateformes
 * - Gestion complète de la quadrature (4 états par détente)
 * - Élimine presque tous les glitches
 * - Utilisée par des milliers de projets Arduino
 * 
 * Note: La librairie Encoder ne gère que la rotation, pas le bouton.
 * Le bouton est géré par notre classe KY040Encoder.
 */

#ifndef KY040_ENCODER_H
#define KY040_ENCODER_H

#include <Arduino.h>
#include <Encoder.h>  // Librairie de Paul Stoffregen

// ============================================
// CONFIGURATION
// ============================================
#define KY040_ENCODER_DIVISOR   4     ///< Diviseur pour 1 clic = 1 position (mettre à 2 ou 1 si besoin)
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
 * 
 * NOUVEAU: Utilise la librairie Encoder de Paul Stoffregen pour une fiabilité maximale.
 */
class KY040Encoder {
private:
  // Librairie Encoder (gestion de la rotation)
  Encoder* encoder;             ///< Pointeur vers l'objet Encoder
  
  // Pins
  uint8_t pinCLK;               ///< Broche CLK (sortie A)
  uint8_t pinDT;                ///< Broche DT (sortie B)
  uint8_t pinSW;                ///< Broche SW (bouton)
  
  // État rotation
  int32_t position;             ///< Position courante (divisée)
  int32_t lastRawPosition;      ///< Dernière position brute (avant division)
  bool rotationDetected;        ///< Flag rotation détectée
  RotationDirection lastDirection;  ///< Dernière direction
  
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
   * @brief Lit la position de l'encodeur et détecte les changements
   */
  void readEncoderPosition() {
    if (!encoder) return;
    
    // Lire position brute de la librairie Encoder
    int32_t rawPosition = encoder->read();
    
    // Diviser pour obtenir 1 clic = 1 position
    int32_t newPosition = rawPosition / KY040_ENCODER_DIVISOR;
    
    // Vérifier si la position a changé
    if (newPosition != position) {
      // Déterminer direction
      if (newPosition > position) {
        lastDirection = reverseDirection ? 
          RotationDirection::COUNTER_CLOCKWISE : 
          RotationDirection::CLOCKWISE;
      } else {
        lastDirection = reverseDirection ? 
          RotationDirection::CLOCKWISE : 
          RotationDirection::COUNTER_CLOCKWISE;
      }
      
      // Appliquer limites si activées
      if (limitEnabled) {
        if (newPosition < minPosition) {
          newPosition = minPosition;
          encoder->write(newPosition * KY040_ENCODER_DIVISOR);
        } else if (newPosition > maxPosition) {
          newPosition = maxPosition;
          encoder->write(newPosition * KY040_ENCODER_DIVISOR);
        }
      }
      
      position = newPosition;
      rotationDetected = true;
    }
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
   * @param clk Broche CLK (sortie A) - DOIT être sur une broche d'interruption
   * @param dt Broche DT (sortie B) - DOIT être sur une broche d'interruption
   * @param sw Broche SW (bouton)
   * 
   * @warning Sur Arduino Mega, les broches d'interruption sont : 2, 3, 18, 19, 20, 21
   */
  KY040Encoder(uint8_t clk, uint8_t dt, uint8_t sw)
    : encoder(nullptr),
      pinCLK(clk),
      pinDT(dt),
      pinSW(sw),
      position(0),
      lastRawPosition(0),
      rotationDetected(false),
      lastDirection(RotationDirection::NONE),
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
  
  /**
   * @brief Destructeur
   */
  ~KY040Encoder() {
    if (encoder) delete encoder;
  }

  // INITIALISATION
  // ------------------------------------------
  /**
   * @brief Initialise l'encodeur avec la librairie Encoder
   * @return true si succès
   */
  bool begin() {
    // Créer l'objet Encoder (gère automatiquement les interruptions)
    encoder = new Encoder(pinCLK, pinDT);
    
    if (!encoder) return false;
    
    // Configuration du bouton
    pinMode(pinSW, INPUT_PULLUP);
    buttonState = digitalRead(pinSW) == LOW;
    
    initialized = true;
    return true;
  }

  // LECTURE DONNÉES
  // ------------------------------------------
  /**
   * @brief Met à jour l'état de l'encodeur
   * 
   * @details 
   * La rotation est gérée automatiquement par la librairie Encoder.
   * Cette fonction lit la position et gère le bouton.
   */
  void update() {
    if (!initialized) return;
    
    readEncoderPosition();
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
    if (encoder) {
      encoder->write(pos * KY040_ENCODER_DIVISOR);
    }
  }

  /**
   * @brief Réinitialise la position à zéro
   */
  void resetPosition() {
    setPosition(0);
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
    if (position < minPosition) setPosition(minPosition);
    if (position > maxPosition) setPosition(maxPosition);
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