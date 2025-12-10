/**
 * @file test_ky040.ino
 * @brief Programme de test pour l'encodeur rotatif KY040
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-21
 * 
 * @details
 * Objectifs du test :
 * - Validation de la connexion matérielle
 * - Vérification de la détection des rotations (CW/CCW)
 * - Test du bouton poussoir (clic court et appui long)
 * - Test des limites de position
 * - Test de l'inversion de direction
 * 
 * Configuration matérielle :
 * - Arduino Mega
 * - Encodeur KY040
 * - Connexions : CLK, DT, SW, +, GND
 */

#include "KY040Encoder.h"

// ============================================
// CONFIGURATION
// ============================================
#define ENCODER_CLK   2      ///< Broche CLK (sortie A)
#define ENCODER_DT    3      ///< Broche DT (sortie B)
#define ENCODER_SW    4      ///< Broche SW (bouton)
#define SERIAL_BAUD   115200  ///< Vitesse série

// ============================================
// VARIABLES GLOBALES
// ============================================
KY040Encoder encoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW);

bool testMode = true;         ///< Mode test interactif
bool monitoringActive = false;


// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);  // Initialisation série
  delay(3000);
    
  Serial.println();
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   TEST KY040 - Vérification Hardware   ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  // Configuration
  Serial.println(F("Broches configurées :"));
  Serial.print(F("  CLK (A) : GPIO "));
  Serial.println(ENCODER_CLK);
  Serial.print(F("  DT  (B) : GPIO "));
  Serial.println(ENCODER_DT);
  Serial.print(F("  SW      : GPIO "));
  Serial.println(ENCODER_SW);
  Serial.println();
  
  // Initialisation encodeur
  Serial.print(F("Initialisation encodeur... "));
  
  if (encoder.begin()) {
    Serial.println(F("✓ OK"));
  } else {
    Serial.println(F("✗ ÉCHEC"));
    Serial.println(F("Vérifiez le câblage !"));
    while (1) delay(1000);
  }
  
  Serial.println();
  Serial.println(F("✓ Initialisation terminée"));
  
  // État initial
  Serial.println();
  Serial.println(F("État initial :"));
  printEncoderState();
  
  // Menu
  printMenu();
}

// ============================================
// LOOP
// ============================================
void loop() {
  // Mode monitoring
  if (monitoringActive) {
    printMonitoring();
    delay(10);
    return;
  }
  
  // Mode test
  if (!testMode) {
    encoder.update();
    delay(10);
    return;
  }
  
  // Commandes
  if (Serial.available()) {
    handleCommand(Serial.read());
    while (Serial.available())  Serial.read();  // Vider buffer
    
    if (testMode && !monitoringActive) {
      delay(500);
      printMenu();
    }
  }
  
  delay(10);
}

// ============================================
// GESTION DES COMMANDES
// ============================================
/**
 * @brief Gère les commandes
 */
void handleCommand(char cmd) {
  Serial.println();

    // Exécuter test
  switch (cmd) {
    case '1':
      testRotationBasique();
      break;
    case '2':
      testCompteur();
      break;
    case '3':
      testBouton();
      break;
    case '4':
      testDirectionInversee();
      break;
    case '5':
      testLimitesPosition();
      break;
    case '6':
      testMonitoring();
      break;
    case '7':
      testReinitialiser();
      break;
    default:
      Serial.println(F("Commande inconnue"));
      break;
    }
  
  Serial.println();
}

// ============================================
// FONCTIONS AFFICHAGE
// ============================================
/**
 * @brief Affiche le menu des tests
 */
void printMenu() {
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      MENU TEST KY040 ENCODER           ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("1. Test rotation basique"));
  Serial.println(F("2. Test compteur (0-10)"));
  Serial.println(F("3. Test bouton (clic/long)"));
  Serial.println(F("4. Test direction inversée"));
  Serial.println(F("5. Test limites position"));
  Serial.println(F("6. Test monitoring continu"));
  Serial.println(F("7. Réinitialiser position"));
  Serial.println();
  Serial.println(F("Tapez le numéro du test..."));
  Serial.println();
}

/**
 * @brief Affiche l'état actuel de l'encodeur
 */
void printEncoderState() {
  Serial.print(F("Position : "));
  Serial.print(encoder.getPosition());
  Serial.print(F(" | Direction : "));
  Serial.print(KY040Encoder::directionToString(encoder.getDirection()));
  Serial.print(F(" | Bouton : "));
  Serial.println(encoder.isButtonPressed() ? F("APPUYÉ") : F("RELÂCHÉ"));
}

// ============================================
// FONCTIONS DE TEST
// ============================================
/**
 * @brief Test 1 : Rotation basique
 */
void testRotationBasique() {
  Serial.println();
  Serial.println(F("=== TEST 1 : ROTATION BASIQUE ==="));
  Serial.println(F("Tournez l'encodeur dans les deux sens"));
  Serial.println(F("Appuyez sur le bouton pour terminer"));
  Serial.println();
  
  encoder.resetPosition();
  
  while (true) {
    encoder.update();
    
    // Détecter rotation
    if (encoder.hasRotated()) {
      Serial.print(F("Rotation détectée : "));
      Serial.print(KY040Encoder::directionToString(encoder.getDirection()));
      Serial.print(F(" -> Position = "));
      Serial.println(encoder.getPosition());
    }
    
    // Détecter événement bouton
    ButtonEvent event = encoder.getButtonEvent();
    if (event == ButtonEvent::CLICKED) {
      Serial.println(F("\n✓ Test terminé"));
      break;
    }
    
    delay(10);
  }
}

/**
 * @brief Test 2 : Compteur avec limites
 */
void testCompteur() {
  Serial.println();
  Serial.println(F("=== TEST 2 : COMPTEUR (0-10) ==="));
  Serial.println(F("Compteur limité entre 0 et 10"));
  Serial.println(F("Tournez l'encodeur pour tester"));
  Serial.println(F("Appuyez sur le bouton pour terminer"));
  Serial.println();
  
  encoder.setPosition(5);
  encoder.setLimits(0, 10);
  
  Serial.print(F("Position initiale : "));
  Serial.println(encoder.getPosition());
  Serial.println();
  
  int32_t lastPos = encoder.getPosition();
  
  while (true) {
    encoder.update();
    
    int32_t currentPos = encoder.getPosition();
    
    // Afficher si changement
    if (currentPos != lastPos) {
      Serial.print(F("Compteur : "));
      Serial.print(currentPos);
      
      // Indicateurs limites
      if (currentPos == 0) {
        Serial.print(F(" [MIN]"));
      } else if (currentPos == 10) {
        Serial.print(F(" [MAX]"));
      }
      
      Serial.println();
      lastPos = currentPos;
    }
    
    // Détecter événement bouton
    ButtonEvent event = encoder.getButtonEvent();
    if (event == ButtonEvent::CLICKED) {
      encoder.removeLimits();
      Serial.println(F("\n✓ Test terminé"));
      Serial.println(F("Limites désactivées"));
      break;
    }
    
    delay(10);
  }
}

/**
 * @brief Test 3 : Événements bouton
 */
void testBouton() {
  Serial.println();
  Serial.println(F("=== TEST 3 : BOUTON (CLIC/LONG) ==="));
  Serial.println(F("Test des événements bouton :"));
  Serial.println(F("- Clic court : < 1 seconde"));
  Serial.println(F("- Appui long : ≥ 1 seconde"));
  Serial.println(F("Faites 3 clics pour terminer"));
  Serial.println();
  
  uint8_t clickCount = 0;
  
  while (clickCount < 3) {
    encoder.update();
    
    ButtonEvent event = encoder.getButtonEvent();
    
    switch (event) {
      case ButtonEvent::PRESSED:
        Serial.println(F("→ Bouton APPUYÉ"));
        break;
        
      case ButtonEvent::CLICKED:
        clickCount++;
        Serial.print(F("✓ CLIC détecté ("));
        Serial.print(clickCount);
        Serial.println(F("/3)"));
        break;
        
      case ButtonEvent::LONG_PRESS:
        Serial.println(F("⏱ APPUI LONG détecté"));
        break;
        
      default:
        break;
    }
    
    delay(10);
  }
  
  Serial.println(F("\n✓ Test terminé"));
}

/**
 * @brief Test 4 : Direction inversée
 */
void testDirectionInversee() {
  Serial.println();
  Serial.println(F("TEST 4 : DIRECTION INVERSÉE"));
  Serial.println(F("Test de l'inversion du sens de rotation"));
  Serial.println();
  
  encoder.resetPosition();
  encoder.setReverseDirection(true);
  
  Serial.println(F("✓ Direction INVERSÉE activée"));
  Serial.println(F("Tournez à droite (CW) -> le compteur DÉCRÉMENTE"));
  Serial.println(F("Tournez à gauche (CCW) -> le compteur INCRÉMENTE"));
  Serial.println(F("Appuyez sur le bouton pour terminer"));
  Serial.println();
  
  while (true) {
    encoder.update();
    
    if (encoder.hasRotated()) {
      Serial.print(F("Rotation physique : "));
      
      // Afficher direction inversée
      RotationDirection dir = encoder.getDirection();
      if (dir == RotationDirection::CLOCKWISE) {
        Serial.print(F("CW (droite)"));
      } else {
        Serial.print(F("CCW (gauche)"));
      }
      
      Serial.print(F(" -> Position = "));
      Serial.println(encoder.getPosition());
    }
    
    ButtonEvent event = encoder.getButtonEvent();
    if (event == ButtonEvent::CLICKED) {
      encoder.setReverseDirection(false);
      Serial.println(F("\n✓ Test terminé"));
      Serial.println(F("Direction normale restaurée"));
      break;
    }
    
    delay(10);
  }
}

/**
 * @brief Test 5 : Limites de position
 */
void testLimitesPosition() {
  Serial.println();
  Serial.println(F("TEST 5 : LIMITES POSITION"));
  Serial.println(F("Test des limites [-5, +5]"));
  Serial.println(F("L'encodeur ne peut pas sortir de cette plage"));
  Serial.println(F("Appuyez sur le bouton pour terminer"));
  Serial.println();
  
  encoder.setPosition(0);
  encoder.setLimits(-5, 5);
  
  int32_t min, max;
  if (encoder.getLimits(min, max)) {
    Serial.print(F("Limites configurées : ["));
    Serial.print(min);
    Serial.print(F(", "));
    Serial.print(max);
    Serial.println(F("]"));
    Serial.println();
  }
  
  int32_t lastPos = 999;
  
  while (true) {
    encoder.update();
    
    int32_t currentPos = encoder.getPosition();
    
    if (currentPos != lastPos) {
      Serial.print(F("Position : "));
      
      // Barre de progression visuelle
      Serial.print(F("["));
      for (int i = -5; i <= 5; i++) {
        if (i == currentPos) {
          Serial.print(F("█"));
        } else {
          Serial.print(F("─"));
        }
      }
      Serial.print(F("] "));
      Serial.print(currentPos);
      
      if (currentPos == -5) {
        Serial.print(F(" ⚠ LIMITE MIN"));
      } else if (currentPos == 5) {
        Serial.print(F(" ⚠ LIMITE MAX"));
      }
      
      Serial.println();
      lastPos = currentPos;
    }
    
    ButtonEvent event = encoder.getButtonEvent();
    if (event == ButtonEvent::CLICKED) {
      encoder.removeLimits();
      Serial.println(F("\n✓ Test terminé"));
      break;
    }
    
    delay(10);
  }
}

/**
 * @brief Test 6 : Monitoring continu
 */
void testMonitoring() {
  Serial.println();
  Serial.println(F("TEST 6 : MONITORING CONTINU"));
  Serial.println(F("Affichage continu de l'état"));
  Serial.println(F("Tournez l'encodeur ou appuyez sur le bouton"));
  Serial.println(F("Envoyez 'q' pour quitter"));
  Serial.println();
  
  encoder.resetPosition();
  monitoringActive = true;
  testMode = false;
  
  Serial.println(F("Format : [Position] Direction | Bouton | Événement"));
  Serial.println();
}

/**
 * @brief Test 7 : Réinitialiser position
 */
void testReinitialiser() {
  Serial.println();
  Serial.println(F("TEST 7 : RÉINITIALISATION"));
  Serial.print(F("Position actuelle : "));
  Serial.println(encoder.getPosition());
  
  encoder.resetPosition();
  encoder.removeLimits();
  
  Serial.print(F("Position après reset : "));
  Serial.println(encoder.getPosition());
  
  Serial.println(F("\n✓ Position réinitialisée à 0"));
  Serial.println(F("✓ Limites désactivées"));
}

// ============================================
// AFFICHAGE MONITORING
// ============================================
/**
 * @brief Affiche les données en mode monitoring
 */
void printMonitoring() {
  encoder.update();
  
  // Détecter rotation
  if (encoder.hasRotated()) {
    Serial.print(F("["));
    Serial.print(encoder.getPosition());
    Serial.print(F("] "));
    Serial.print(KY040Encoder::directionToString(encoder.getDirection()));
    Serial.print(F(" | "));
    Serial.print(encoder.isButtonPressed() ? F("BTN:ON ") : F("BTN:OFF"));
    Serial.println();
  }
  
  // Détecter événement bouton
  ButtonEvent event = encoder.getButtonEvent();
  if (event != ButtonEvent::NONE) {
    Serial.print(F("     → Événement bouton : "));
    Serial.println(KY040Encoder::buttonEventToString(event));
  }
  
  // Quitter monitoring
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'q' || c == 'Q') {
      monitoringActive = false;
      testMode = true;
      Serial.println();
      Serial.println(F("Monitoring arrêté"));
      printMenu();
    }
    while (Serial.available()) Serial.read();
  }
}
