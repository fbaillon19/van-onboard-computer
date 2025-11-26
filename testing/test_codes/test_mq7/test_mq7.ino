/**
 * @file test_mq7.ino
 * @brief Programme de test du capteur MQ7 (monoxyde de carbone)
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-25
 * 
 * @details
 * Programme de test pour validation matérielle du MQ7.
 * 
 * Objectifs :
 * - Vérifier le fonctionnement du capteur
 * - Observer le cycle de chauffe
 * - Valider la détection de CO
 * - Calibrer le capteur
 * - Comprendre les seuils d'alerte
 * 
 * Connexions :
 * - VCC  → 5V
 * - GND  → GND
 * - AOUT → A0 (ou autre pin analogique)
 * - DOUT → Non utilisé (sortie numérique)
 * 
 * ⚠️ IMPORTANT SÉCURITÉ :
 * - CO = GAZ MORTEL, inodore et incolore
 * - Ne PAS tester avec de vraies sources de CO dangereuses
 * - Test en extérieur ou pièce TRÈS bien ventilée
 * - Avoir un détecteur CO commercial en backup
 * 
 * Sources de test SÛRES :
 * - Moteur de voiture (à l'extérieur, à distance)
 * - Briquet au butane (petite flamme, 10-15 cm)
 * - Encens (génère peu de CO)
 * 
 * @warning Ce capteur nécessite 24-48h de première chauffe pour stabilisation
 * @note Le cycle de chauffe est simplifié dans ce test (pas de PWM 1.4V)
 */

#include "MQ7Sensor.h"

// ============================================
// CONFIGURATION
// ============================================
#define MQ7_PIN       A0          ///< Pin analogique du MQ7
#define LED_PIN       13          ///< LED d'indication
#define BUZZER_PIN    8           ///< Buzzer d'alarme (optionnel)
#define SERIAL_BAUD   115200

// ============================================
// VARIABLES GLOBALES
// ============================================
MQ7Sensor mq7(MQ7_PIN);

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
  Serial.println(F("║   TEST MQ7 - Détection CO              ║"));
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
  Serial.println(F("Le CO (monoxyde de carbone) est un gaz MORTEL."));
  Serial.println(F("- Inodore et incolore"));
  Serial.println(F("- Test uniquement en extérieur ou zone ventilée"));
  Serial.println(F("- Avoir détecteur CO commercial en backup"));
  Serial.println();
  
  delay(2000);
  
  // Initialiser capteur
  Serial.println(F("Initialisation MQ7..."));
  
  if (!mq7.begin()) {
    Serial.println(F("❌ ÉCHEC initialisation"));
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }
  
  Serial.println(F("✓ MQ7 initialisé"));
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
  while (mq7.getStatus() == MQ7Status::PREHEATING) {
    mq7.update();
    
    uint32_t remaining = mq7.getPreheatTimeRemaining();
    
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
  
  // Informations cycle de chauffe
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      CYCLE DE CHAUFFE                  ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Le MQ7 fonctionne en cycle alterné :"));
  Serial.println(F("  • Phase HAUTE : 60s (nettoyage)"));
  Serial.println(F("  • Phase BASSE : 90s (mesure CO)"));
  Serial.println();
  Serial.println(F("⚠️  Les lectures ne sont valides QUE"));
  Serial.println(F("    pendant la phase BASSE."));
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
  mq7.update();
  
  // Affichage périodique
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    printValues();
    
    // Gestion LED et alarme
    updateIndicators();
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
      mq7.setSampleInterval(500);
      Serial.println(F("✓ Mode rapide (0.5s)"));
      break;
      
    case '2':
      mq7.setSampleInterval(1000);
      Serial.println(F("✓ Mode normal (1s)"));
      break;
      
    case '3':
      mq7.setSampleInterval(2000);
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
      
    case 't':
    case 'T':
      testDetection();
      break;
      
    case 's':
    case 'S':
      showThresholds();
      break;
      
    case 'h':
    case 'H':
      printMenu();
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
  Serial.println(F("║      MENU TEST MQ7                     ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("  1 - Mode rapide (lectures 0.5s)"));
  Serial.println(F("  2 - Mode normal (lectures 1s)"));
  Serial.println(F("  3 - Mode lent (lectures 2s)"));
  Serial.println(F("  D - Toggle affichage détaillé"));
  Serial.println(F("  A - Toggle alarme sonore"));
  Serial.println(F("  C - Calibrer capteur (air pur)"));
  Serial.println(F("  T - Test détection CO"));
  Serial.println(F("  S - Afficher seuils sécurité"));
  Serial.println(F("  H - Afficher cette aide"));
  Serial.println(F("  R - Redémarrer"));
  Serial.println();
}

/**
 * @brief Affiche les valeurs courantes
 */
void printValues() {
  MQ7Data data = mq7.getData();
  MQ7Status status = mq7.getStatus();
  MQ7AlertLevel alert = mq7.getAlertLevel();
  
  Serial.println(F("┌─────────────────────────────────────┐"));
  
  // État
  Serial.print(F("│ État : "));
  Serial.print(MQ7Sensor::statusToString(status));
  
  // Indicateur phase
  if (mq7.isReadingValid()) {
    Serial.print(F(" [MESURE]"));
  } else {
    Serial.print(F(" [ATTENTE]"));
  }
  
  // Padding
  for (int i = strlen(MQ7Sensor::statusToString(status)) + 10; i < 34; i++) {
    Serial.print(F(" "));
  }
  Serial.println(F("│"));
  
  Serial.println(F("├─────────────────────────────────────┤"));
  
  // Concentration CO
  Serial.print(F("│ CO : "));
  if (mq7.isReadingValid()) {
    printFloat(data.ppm, 0, 6);
    Serial.print(F(" ppm  "));
    
    // Indicateur niveau
    switch (alert) {
      case MQ7AlertLevel::SAFE:
        Serial.print(F("✓"));
        break;
      case MQ7AlertLevel::WARNING:
        Serial.print(F("⚠️ "));
        break;
      case MQ7AlertLevel::DANGER:
        Serial.print(F("🔴"));
        break;
      case MQ7AlertLevel::CRITICAL:
        Serial.print(F("💀"));
        break;
    }
  } else {
    Serial.print(F("------ ppm  ⏳"));
  }
  Serial.println(F("  │"));
  
  // Niveau d'alerte
  Serial.print(F("│ Alerte : "));
  Serial.print(MQ7Sensor::alertLevelToString(alert));
  
  // Padding
  for (int i = strlen(MQ7Sensor::alertLevelToString(alert)); i < 26; i++) {
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
    printFloat(mq7.getR0(), 2, 6);
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

// ============================================
// FONCTIONS DE TEST
// ============================================
/**
 * @brief Test de détection
 */
void testDetection() {
  Serial.println(F("=== TEST DÉTECTION CO ==="));
  Serial.println();
  Serial.println(F("Sources de test SÛRES :"));
  Serial.println(F("  • Briquet (petite flamme, 10-15 cm)"));
  Serial.println(F("  • Encens (combustion incomplète)"));
  Serial.println(F("  • Moteur voiture (EXTÉRIEUR, à distance)"));
  Serial.println();
  Serial.println(F("⚠️  NE PAS utiliser de vraies sources dangereuses !"));
  Serial.println();
  Serial.println(F("Surveillance pendant 30 secondes..."));
  Serial.println();
  
  float baseline = mq7.getPPM();
  float maxPPM = baseline;
  
  unsigned long start = millis();
  
  while (millis() - start < 30000) {
    mq7.update();
    
    if (mq7.isReadingValid()) {
      float currentPPM = mq7.getPPM();
      
      if (currentPPM > maxPPM) {
        maxPPM = currentPPM;
      }
      
      Serial.print(F("CO : "));
      Serial.print(currentPPM, 0);
      Serial.print(F(" ppm (max: "));
      Serial.print(maxPPM, 0);
      Serial.print(F(" ppm)"));
      
      if (currentPPM > baseline + 20) {
        Serial.print(F(" ⚠️  DÉTECTION !"));
      }
      
      Serial.println();
      
      delay(1000);
    }
  }
  
  Serial.println();
  Serial.print(F("✓ Test terminé"));
  Serial.println();
  Serial.print(F("  Baseline : "));
  Serial.print(baseline, 0);
  Serial.println(F(" ppm"));
  Serial.print(F("  Maximum  : "));
  Serial.print(maxPPM, 0);
  Serial.println(F(" ppm"));
  Serial.print(F("  Variation: +"));
  Serial.print(maxPPM - baseline, 0);
  Serial.println(F(" ppm"));
  Serial.println();
  
  if (maxPPM - baseline > 50) {
    Serial.println(F("✓ Capteur réactif !"));
  } else {
    Serial.println(F("⚠️  Faible variation détectée"));
    Serial.println(F("   Essayez avec source plus proche"));
  }
  Serial.println();
}

/**
 * @brief Affiche les seuils de sécurité
 */
void showThresholds() {
  Serial.println(F("\n=== SEUILS SÉCURITÉ CO ===\n"));
  Serial.println(F("Concentration   Danger         Temps"));
  Serial.println(F("─────────────────────────────────────"));
  Serial.println(F("< 50 ppm        ✓ SÛR          Illimité"));
  Serial.println(F("50-200 ppm      ⚠️ Attention    8 heures max"));
  Serial.println(F("200-400 ppm     🔴 Danger      2-3 heures"));
  Serial.println(F("400-800 ppm     🔴 Grave       1-2 heures"));
  Serial.println(F("800-1600 ppm    💀 Critique    45 minutes"));
  Serial.println(F("> 1600 ppm      💀 MORTEL      20 minutes"));
  Serial.println(F("─────────────────────────────────────"));
  Serial.println();
  Serial.println(F("⚠️  RAPPEL : CO = Inodore, incolore, mortel"));
  Serial.println(F("   Toujours avoir détecteur CO fonctionnel !"));
  Serial.println();
}

// ============================================
// FONCTIONS UTILITAIRES
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
  Serial.println(F("  - Pas de source de CO à proximité"));
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
  Serial.println(F("Ne pas approcher de source CO !"));
  Serial.println();
  
  float r0 = mq7.calibrate(50);
  
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
 * @brief Met à jour les indicateurs (LED, buzzer)
 */
void updateIndicators() {
  MQ7AlertLevel alert = mq7.getAlertLevel();
  
  // LED
  switch (alert) {
    case MQ7AlertLevel::SAFE:
      digitalWrite(LED_PIN, HIGH);  // Fixe
      break;
      
    case MQ7AlertLevel::WARNING:
      digitalWrite(LED_PIN, (millis() / 500) % 2);  // Clignotement lent
      break;
      
    case MQ7AlertLevel::DANGER:
      digitalWrite(LED_PIN, (millis() / 250) % 2);  // Clignotement rapide
      break;
      
    case MQ7AlertLevel::CRITICAL:
      digitalWrite(LED_PIN, (millis() / 100) % 2);  // Clignotement très rapide
      break;
  }
  
  // Buzzer
  if (alarmEnabled && buzzerPresent) {
    if (alert == MQ7AlertLevel::CRITICAL) {
      // Alarme continue en critique
      tone(BUZZER_PIN, 2000);
    } else if (alert == MQ7AlertLevel::DANGER) {
      // Bips répétés en danger
      if ((millis() / 500) % 2) {
        tone(BUZZER_PIN, 1500, 200);
      }
    } else if (alert == MQ7AlertLevel::WARNING) {
      // Bips espacés en warning
      if ((millis() / 2000) % 2 == 0 && (millis() % 2000) < 100) {
        tone(BUZZER_PIN, 1000, 100);
      }
    } else {
      noTone(BUZZER_PIN);
    }
  }
}
