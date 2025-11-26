/**
 * @file test_mq2.ino
 * @brief Programme de test du capteur MQ2 (GPL, méthane, fumée)
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-25
 * 
 * @details
 * Programme de test pour validation matérielle du MQ2.
 * 
 * Objectifs :
 * - Vérifier le fonctionnement du capteur
 * - Observer les réactions aux différents gaz
 * - Valider la détection GPL, méthane, fumée
 * - Calibrer le capteur
 * - Comprendre les seuils d'alerte
 * 
 * Connexions :
 * - VCC  → 5V
 * - GND  → GND
 * - AOUT → A1 (ou autre pin analogique)
 * - DOUT → Non utilisé (sortie numérique)
 * 
 * ⚠️ IMPORTANT SÉCURITÉ :
 * - GPL et CH4 = GAZ EXPLOSIFS !
 * - Ne PAS tester avec fuites importantes
 * - Test en extérieur ou pièce TRÈS bien ventilée
 * - Pas de flamme à proximité pendant tests
 * - Couper gaz après test
 * 
 * Sources de test SÛRES :
 * - Briquet au butane (sans flamme, courte durée)
 * - Allumette soufflée (fumée)
 * - Réchaud gaz (à distance, ventilé)
 * 
 * @warning Ce capteur nécessite 24-48h de première chauffe pour stabilisation
 * @note Le MQ2 fonctionne en chauffage continu (pas de cycle comme MQ7)
 */

#include "MQ2Sensor.h"

// ============================================
// CONFIGURATION
// ============================================
#define MQ2_PIN       A1          ///< Pin analogique du MQ2
#define LED_PIN       13          ///< LED d'indication
#define BUZZER_PIN    8           ///< Buzzer d'alarme (optionnel)
#define SERIAL_BAUD   115200

// ============================================
// VARIABLES GLOBALES
// ============================================
MQ2Sensor mq2(MQ2_PIN);

unsigned long lastPrint = 0;
bool detailMode = false;
bool alarmEnabled = true;
bool buzzerPresent = false;

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(3000);
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   TEST MQ2 - Détection Gaz Multi       ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Configuration buzzer (optionnel)
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Test buzzer
  Serial.print(F("Test buzzer... "));
  tone(BUZZER_PIN, 1000, 100);
  delay(150);
  Serial.println(F("✓"));
  buzzerPresent = true;
  
  Serial.println();
  Serial.println(F("⚠️  AVERTISSEMENT SÉCURITÉ :"));
  Serial.println(F("Le GPL et le méthane sont des gaz EXPLOSIFS."));
  Serial.println(F("- Pas de flamme pendant les tests"));
  Serial.println(F("- Test uniquement en extérieur ou zone ventilée"));
  Serial.println(F("- Petites quantités de gaz uniquement"));
  Serial.println(F("- Couper le gaz après test"));
  Serial.println();
  
  delay(2000);
  
  // Initialiser capteur
  Serial.println(F("Initialisation MQ2..."));
  
  if (!mq2.begin()) {
    Serial.println(F("❌ ÉCHEC initialisation"));
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }
  
  Serial.println(F("✓ MQ2 initialisé"));
  Serial.println();
  
  // Pré-chauffe
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      PRÉ-CHAUFFE EN COURS              ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Le capteur doit chauffer pendant 3 minutes"));
  Serial.println(F("avant d'être opérationnel."));
  Serial.println();
  Serial.println(F("⏱️  Temps restant : "));
  
  // Attendre fin pré-chauffe avec affichage progression
  while (mq2.getStatus() == MQ2Status::PREHEATING) {
    mq2.update();
    
    uint32_t remaining = mq2.getPreheatTimeRemaining();
    
    static uint32_t lastRemaining = 999;
    if (remaining != lastRemaining) {
      lastRemaining = remaining;
      
      Serial.print(F("\r⏱️  Temps restant : "));
      Serial.print(remaining);
      Serial.print(F(" secondes   "));
      
      // LED clignotante pendant pré-chauffe
      digitalWrite(LED_PIN, (remaining % 2 == 0));
    }
    
    delay(100);
  }
  
  Serial.println();
  Serial.println();
  Serial.println(F("✓ Pré-chauffe terminée !"));
  Serial.println();
  
  digitalWrite(LED_PIN, HIGH);
  
  // Informations capteur
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      CAPACITÉS DU MQ2                  ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Le MQ2 peut détecter :"));
  Serial.println(F("  • GPL (butane, propane) : 200-10000 ppm"));
  Serial.println(F("  • Méthane (CH4)         : 200-10000 ppm"));
  Serial.println(F("  • Fumée                 : 100-10000 ppm"));
  Serial.println();
  Serial.println(F("Le capteur fonctionne en chauffage continu."));
  Serial.println(F("Toutes les lectures sont valides."));
  Serial.println();
  
  delay(3000);
  
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      MESURES EN TEMPS RÉEL             ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  printMenu();
}

// ============================================
// LOOP
// ============================================
void loop() {
  // Mettre à jour capteur
  if (mq2.update()) {
    // Affichage périodique
    if (millis() - lastPrint >= 2000) {
      lastPrint = millis();
      printValues();
      
      // Gestion LED et alarme
      updateIndicators();
    }
  }
  
  // Commandes
  if (Serial.available()) {
    handleCommand(Serial.read());
    while (Serial.available()) Serial.read();
  }
  
  delay(10);
}

// ============================================
// GESTION COMMANDES
// ============================================
/**
 * @brief Traite les commandes série
 */
void handleCommand(char cmd) {
  Serial.println();
  
  switch (cmd) {
    case '1':
      mq2.setSampleInterval(500);
      Serial.println(F("✓ Mode rapide (0.5s)"));
      break;
      
    case '2':
      mq2.setSampleInterval(1000);
      Serial.println(F("✓ Mode normal (1s)"));
      break;
      
    case '3':
      mq2.setSampleInterval(2000);
      Serial.println(F("✓ Mode lent (2s)"));
      break;
      
    case 'd':
    case 'D':
      detailMode = !detailMode;
      Serial.print(F("✓ Affichage détaillé : "));
      Serial.println(detailMode ? F("ACTIVÉ") : F("DÉSACTIVÉ"));
      break;
      
    case 'a':
    case 'A':
      alarmEnabled = !alarmEnabled;
      Serial.print(F("✓ Alarme : "));
      Serial.println(alarmEnabled ? F("ACTIVÉE") : F("DÉSACTIVÉE"));
      if (!alarmEnabled) {
        noTone(BUZZER_PIN);
      }
      break;
      
    case 'c':
    case 'C':
      calibrateSensor();
      break;
      
    case 'l':
    case 'L':
      testLPG();
      break;
      
    case 'm':
    case 'M':
      testMethane();
      break;
      
    case 's':
    case 'S':
      testSmoke();
      break;
      
    case 't':
    case 'T':
      testAllGases();
      break;
      
    case 'h':
    case 'H':
      printMenu();
      break;
      
    case 'i':
    case 'I':
      showInfo();
      break;
      
    case 'r':
    case 'R':
      Serial.println(F("Redémarrage..."));
      delay(100);
      asm volatile ("jmp 0");
      break;
      
    default:
      if (cmd >= 32 && cmd <= 126) {
        Serial.print(F("Commande inconnue: "));
        Serial.println(cmd);
      }
      break;
  }
}

// ============================================
// FONCTIONS AFFICHAGE
// ============================================
/**
 * @brief Affiche le menu
 */
void printMenu() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      MENU TEST MQ2                     ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("  1 - Mode rapide (lectures 0.5s)"));
  Serial.println(F("  2 - Mode normal (lectures 1s)"));
  Serial.println(F("  3 - Mode lent (lectures 2s)"));
  Serial.println(F("  D - Toggle affichage détaillé"));
  Serial.println(F("  A - Toggle alarme sonore"));
  Serial.println(F("  C - Calibrer capteur (air pur)"));
  Serial.println(F("  L - Test détection GPL"));
  Serial.println(F("  M - Test détection méthane"));
  Serial.println(F("  S - Test détection fumée"));
  Serial.println(F("  T - Test tous les gaz (30s)"));
  Serial.println(F("  I - Informations capteur"));
  Serial.println(F("  H - Afficher cette aide"));
  Serial.println(F("  R - Redémarrer"));
  Serial.println();
}

/**
 * @brief Affiche les valeurs courantes
 */
void printValues() {
  MQ2Data data = mq2.getData();
  MQ2Status status = mq2.getStatus();
  MQ2AlertLevel alert = mq2.getAlertLevel();
  MQ2GasType gas = mq2.getDominantGas();
  
  Serial.println(F("┌─────────────────────────────────────┐"));
  
  // État
  Serial.print(F("│ État : "));
  Serial.print(MQ2Sensor::statusToString(status));
  
  // Padding
  for (int i = strlen(MQ2Sensor::statusToString(status)); i < 28; i++) {
    Serial.print(F(" "));
  }
  Serial.println(F("│"));
  
  Serial.println(F("├─────────────────────────────────────┤"));
  
  // GPL
  Serial.print(F("│ GPL      : "));
  printFloat(data.lpg, 0, 6);
  Serial.print(F(" ppm "));
  if (mq2.isLPGDetected()) Serial.print(F("⚠️ "));
  else Serial.print(F("✓"));
  Serial.println(F("  │"));
  
  // Méthane
  Serial.print(F("│ Méthane  : "));
  printFloat(data.methane, 0, 6);
  Serial.print(F(" ppm "));
  if (mq2.isMethaneDetected()) Serial.print(F("⚠️ "));
  else Serial.print(F("✓"));
  Serial.println(F("  │"));
  
  // Fumée
  Serial.print(F("│ Fumée    : "));
  printFloat(data.smoke, 0, 6);
  Serial.print(F(" ppm "));
  if (mq2.isSmokeDetected()) Serial.print(F("⚠️ "));
  else Serial.print(F("✓"));
  Serial.println(F("  │"));
  
  Serial.println(F("├─────────────────────────────────────┤"));
  
  // Gaz dominant
  Serial.print(F("│ Gaz détecté : "));
  Serial.print(MQ2Sensor::gasTypeToString(gas));
  
  // Padding
  for (int i = strlen(MQ2Sensor::gasTypeToString(gas)); i < 22; i++) {
    Serial.print(F(" "));
  }
  Serial.println(F("│"));
  
  // Niveau d'alerte
  Serial.print(F("│ Alerte      : "));
  Serial.print(MQ2Sensor::alertLevelToString(alert));
  
  // Indicateur visuel
  Serial.print(F(" "));
  switch (alert) {
    case MQ2AlertLevel::SAFE:
      Serial.print(F("✓"));
      break;
    case MQ2AlertLevel::WARNING:
      Serial.print(F("⚠️ "));
      break;
    case MQ2AlertLevel::DANGER:
      Serial.print(F("🔴"));
      break;
    case MQ2AlertLevel::CRITICAL:
      Serial.print(F("💀"));
      break;
  }
  
  // Padding
  for (int i = strlen(MQ2Sensor::alertLevelToString(alert)); i < 14; i++) {
    Serial.print(F(" "));
  }
  Serial.println(F("│"));
  
  if (detailMode) {
    Serial.println(F("├─────────────────────────────────────┤"));
    
    // Valeur brute
    Serial.print(F("│ ADC brut : "));
    printInt(data.rawValue, 4);
    Serial.println(F("                  │"));
    
    // Tension
    Serial.print(F("│ Tension  : "));
    printFloat(data.voltage, 2, 6);
    Serial.println(F(" V              │"));
    
    // Résistance Rs
    Serial.print(F("│ Rs       : "));
    printFloat(data.rs, 2, 6);
    Serial.println(F(" kΩ             │"));
    
    // Ratio Rs/R0
    Serial.print(F("│ Rs/R0    : "));
    printFloat(data.ratio, 3, 6);
    Serial.println(F("                 │"));
    
    // R0
    Serial.print(F("│ R0       : "));
    printFloat(mq2.getR0(), 2, 6);
    Serial.println(F(" kΩ             │"));
  }
  
  Serial.println(F("└─────────────────────────────────────┘"));
  Serial.println();
}

/**
 * @brief Affiche un float avec padding
 */
void printFloat(float value, int decimals, int width) {
  char buffer[16];
  dtostrf(value, width, decimals, buffer);
  Serial.print(buffer);
}

/**
 * @brief Affiche un int avec padding
 */
void printInt(int value, int width) {
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%*d", width, value);
  Serial.print(buffer);
}

/**
 * @brief Affiche les informations du capteur
 */
void showInfo() {
  Serial.println(F("\n=== INFORMATIONS MQ2 ===\n"));
  
  Serial.println(F("Gaz détectables :"));
  Serial.println(F("  • GPL (butane/propane)"));
  Serial.println(F("  • Méthane (CH4)"));
  Serial.println(F("  • Fumée"));
  Serial.println(F("  • Hydrogène (H2)"));
  Serial.println(F("  • Alcool"));
  Serial.println();
  
  Serial.println(F("Seuils détection :"));
  Serial.println(F("  GPL      : > 1000 ppm"));
  Serial.println(F("  Méthane  : > 1000 ppm"));
  Serial.println(F("  Fumée    : > 500 ppm"));
  Serial.println();
  
  Serial.println(F("Seuils danger :"));
  Serial.println(F("  GPL      : > 3000 ppm"));
  Serial.println(F("  Méthane  : > 5000 ppm"));
  Serial.println(F("  Fumée    : > 2000 ppm"));
  Serial.println();
  
  Serial.println(F("⚠️  RAPPEL SÉCURITÉ :"));
  Serial.println(F("  GPL explosif : 1.8-8.5% dans l'air"));
  Serial.println(F("  CH4 explosif : 5-15% dans l'air"));
  Serial.println(F("  → Toujours ventiler en cas de détection"));
  Serial.println();
}

// ============================================
// FONCTIONS DE TEST
// ============================================
/**
 * @brief Calibration du capteur
 */
void calibrateSensor() {
  Serial.println(F("=== CALIBRATION CAPTEUR ==="));
  Serial.println();
  Serial.println(F("⚠️  Conditions requises :"));
  Serial.println(F("  - Air pur (extérieur, loin des routes)"));
  Serial.println(F("  - Capteur stabilisé (30 min fonctionnement)"));
  Serial.println(F("  - Pas de source de gaz à proximité"));
  Serial.println(F("  - Pas de fumée"));
  Serial.println();
  Serial.println(F("Continuer ? (o/n)"));
  
  while (!Serial.available());
  char response = Serial.read();
  while (Serial.available()) Serial.read();
  
  if (response != 'o' && response != 'O') {
    Serial.println(F("Calibration annulée"));
    return;
  }
  
  Serial.println();
  Serial.println(F("Calibration en cours (50 échantillons)..."));
  Serial.println(F("Ne pas approcher de source gaz/fumée !"));
  Serial.println();
  
  float r0 = mq2.calibrate(50);
  
  Serial.println();
  Serial.print(F("✓ Calibration terminée !"));
  Serial.println();
  Serial.print(F("  R0 = "));
  Serial.print(r0, 2);
  Serial.println(F(" kΩ"));
  Serial.println();
  Serial.println(F("💾 Notez cette valeur pour usage futur."));
  Serial.println();
}

/**
 * @brief Test détection GPL
 */
void testLPG() {
  Serial.println(F("=== TEST DÉTECTION GPL ==="));
  Serial.println();
  Serial.println(F("Source de test SÛRE :"));
  Serial.println(F("  • Briquet (bouton pressé SANS flamme)"));
  Serial.println(F("  • Approcher 5-10 cm du capteur"));
  Serial.println(F("  • Durée courte (2-3 secondes)"));
  Serial.println();
  Serial.println(F("⚠️  Pas de flamme ! Zone ventilée !"));
  Serial.println();
  Serial.println(F("Surveillance pendant 20 secondes..."));
  Serial.println();
  
  runGasTest(20, "GPL");
}

/**
 * @brief Test détection méthane
 */
void testMethane() {
  Serial.println(F("=== TEST DÉTECTION MÉTHANE ==="));
  Serial.println();
  Serial.println(F("Note : Le méthane est difficile à tester"));
  Serial.println(F("sans source dédiée (gaz naturel)."));
  Serial.println();
  Serial.println(F("Si vous avez accès à gaz naturel :"));
  Serial.println(F("  • Réchaud gaz allumé à distance"));
  Serial.println(F("  • Zone TRÈS bien ventilée"));
  Serial.println();
  Serial.println(F("⚠️  Pas d'approche directe ! Danger explosion !"));
  Serial.println();
  Serial.println(F("Surveillance pendant 20 secondes..."));
  Serial.println();
  
  runGasTest(20, "CH4");
}

/**
 * @brief Test détection fumée
 */
void testSmoke() {
  Serial.println(F("=== TEST DÉTECTION FUMÉE ==="));
  Serial.println();
  Serial.println(F("Sources de test SÛRES :"));
  Serial.println(F("  • Allumette soufflée (fumée résiduelle)"));
  Serial.println(F("  • Encens"));
  Serial.println(F("  • Bougie soufflée"));
  Serial.println();
  Serial.println(F("Approcher source de fumée à 10-15 cm."));
  Serial.println();
  Serial.println(F("Surveillance pendant 20 secondes..."));
  Serial.println();
  
  runGasTest(20, "Fumee");
}

/**
 * @brief Test tous les gaz
 */
void testAllGases() {
  Serial.println(F("=== TEST TOUS LES GAZ ==="));
  Serial.println();
  Serial.println(F("Surveillance étendue pendant 30 secondes."));
  Serial.println(F("Vous pouvez tester plusieurs sources."));
  Serial.println();
  Serial.println(F("⚠️  Toujours en zone ventilée !"));
  Serial.println();
  
  runGasTest(30, "Tous");
}

/**
 * @brief Fonction commune de test gaz
 */
void runGasTest(uint8_t duration, const char* gasName) {
  float baselineLPG = mq2.getLPG();
  float baselineCH4 = mq2.getMethane();
  float baselineSmoke = mq2.getSmoke();
  
  float maxLPG = baselineLPG;
  float maxCH4 = baselineCH4;
  float maxSmoke = baselineSmoke;
  
  unsigned long start = millis();
  
  while (millis() - start < duration * 1000UL) {
    mq2.update();
    
    float currentLPG = mq2.getLPG();
    float currentCH4 = mq2.getMethane();
    float currentSmoke = mq2.getSmoke();
    
    if (currentLPG > maxLPG) maxLPG = currentLPG;
    if (currentCH4 > maxCH4) maxCH4 = currentCH4;
    if (currentSmoke > maxSmoke) maxSmoke = currentSmoke;
    
    Serial.print(F("GPL:"));
    Serial.print(currentLPG, 0);
    Serial.print(F(" CH4:"));
    Serial.print(currentCH4, 0);
    Serial.print(F(" Fumée:"));
    Serial.print(currentSmoke, 0);
    
    if (mq2.isAnyGasDetected()) {
      Serial.print(F(" ["));
      Serial.print(MQ2Sensor::gasTypeToString(mq2.getDominantGas()));
      Serial.print(F(" détecté!]"));
    }
    
    Serial.println();
    
    delay(1000);
  }
  
  Serial.println();
  Serial.println(F("✓ Test terminé"));
  Serial.println();
  Serial.println(F("Résumé :"));
  Serial.print(F("  GPL      : "));
  Serial.print(baselineLPG, 0);
  Serial.print(F(" → "));
  Serial.print(maxLPG, 0);
  Serial.print(F(" ppm (+"));
  Serial.print(maxLPG - baselineLPG, 0);
  Serial.println(F(")"));
  
  Serial.print(F("  Méthane  : "));
  Serial.print(baselineCH4, 0);
  Serial.print(F(" → "));
  Serial.print(maxCH4, 0);
  Serial.print(F(" ppm (+"));
  Serial.print(maxCH4 - baselineCH4, 0);
  Serial.println(F(")"));
  
  Serial.print(F("  Fumée    : "));
  Serial.print(baselineSmoke, 0);
  Serial.print(F(" → "));
  Serial.print(maxSmoke, 0);
  Serial.print(F(" ppm (+"));
  Serial.print(maxSmoke - baselineSmoke, 0);
  Serial.println(F(")"));
  Serial.println();
  
  // Évaluation
  bool detected = false;
  if (maxLPG - baselineLPG > 500) {
    Serial.println(F("✓ GPL détecté !"));
    detected = true;
  }
  if (maxCH4 - baselineCH4 > 500) {
    Serial.println(F("✓ Méthane détecté !"));
    detected = true;
  }
  if (maxSmoke - baselineSmoke > 300) {
    Serial.println(F("✓ Fumée détectée !"));
    detected = true;
  }
  
  if (!detected) {
    Serial.println(F("⚠️  Aucune variation significative"));
    Serial.println(F("   Essayez avec source plus proche"));
  }
  Serial.println();
}

// ============================================
// FONCTIONS UTILITAIRES
// ============================================
/**
 * @brief Met à jour les indicateurs (LED, buzzer)
 */
void updateIndicators() {
  MQ2AlertLevel alert = mq2.getAlertLevel();
  
  // LED
  switch (alert) {
    case MQ2AlertLevel::SAFE:
      digitalWrite(LED_PIN, HIGH);  // Fixe
      break;
      
    case MQ2AlertLevel::WARNING:
      digitalWrite(LED_PIN, (millis() / 500) % 2);  // Clignotement lent
      break;
      
    case MQ2AlertLevel::DANGER:
      digitalWrite(LED_PIN, (millis() / 250) % 2);  // Clignotement rapide
      break;
      
    case MQ2AlertLevel::CRITICAL:
      digitalWrite(LED_PIN, (millis() / 100) % 2);  // Clignotement très rapide
      break;
  }
  
  // Buzzer
  if (alarmEnabled && buzzerPresent) {
    if (alert == MQ2AlertLevel::CRITICAL) {
      // Alarme continue en critique
      tone(BUZZER_PIN, 2500);
    } else if (alert == MQ2AlertLevel::DANGER) {
      // Bips répétés en danger
      if ((millis() / 500) % 2) {
        tone(BUZZER_PIN, 2000, 200);
      }
    } else if (alert == MQ2AlertLevel::WARNING) {
      // Bips espacés en warning
      if ((millis() / 2000) % 2 == 0 && (millis() % 2000) < 100) {
        tone(BUZZER_PIN, 1500, 100);
      }
    } else {
      noTone(BUZZER_PIN);
    }
  }
}
