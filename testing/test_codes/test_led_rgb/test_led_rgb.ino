/**
 * @file test_led_rgb.ino
 * @brief Test validation bandeau LED WS2812B - Version pragmatique
 * @author Frédéric BAILLON
 * @version 2.0.0
 * @date 2024-12-10
 * 
 * @details
 * Test focalisé sur validation matérielle + simulation concept projet.
 * 
 * Philosophie :
 * 1. Valider que le matériel fonctionne (8 LEDs, couleurs OK)
 * 2. Montrer le concept final avec scénarios réalistes
 * 
 * Structure des tests :
 * - Test 1-2 : Validation hardware (LEDs individuelles, couleurs)
 * - Test 3-5 : Scénarios réalistes d'alertes dans le van
 * 
 * Scénarios réalistes :
 * - Scénario 1 : Nuit normale (humidité progressive → extracteur auto)
 * - Scénario 2 : Décharge batterie (alerte progressive)
 * - Scénario 3 : Urgence CO (alerte immédiate)
 * 
 * Connexions :
 * - VCC  → 5V
 * - GND  → GND  
 * - DIN  → Pin 6 (Arduino MEGA)
 * ATTENTION
 * Il est impérative d'alimenter le ruban LED avec une source 5V externe
 * L'Arduino MEGA ne fournit pas assez de puissance.
 * 
 * @note Librairie FastLED requise
 */

#include <FastLED.h>

// ============================================
// CONFIGURATION
// ============================================
#define LED_PIN         6       // Pin data WS2812B
#define LED_COUNT       8       // Nombre de LEDs
#define LED_BRIGHTNESS  80      // Luminosité (0-255)
#define SERIAL_BAUD     115200

// Mapping des LEDs (représentation visuelle)
// LED 0-1 : Humidité/Température
// LED 2-3 : Batterie 12V/5V
// LED 4-5 : Gaz (CO/GPL)
// LED 6-7 : État système/Mode

// ============================================
// VARIABLES GLOBALES
// ============================================
CRGB leds[LED_COUNT];

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(3000);
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   TEST BANDEAU LED WS2812B             ║"));
  Serial.println(F("║   Version 2.0 - Pragmatique            ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  // Initialiser FastLED
  Serial.println(F("Initialisation FastLED..."));
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  
  Serial.print(F("✓ Pin data : GPIO "));
  Serial.println(LED_PIN);
  Serial.print(F("✓ Nombre LEDs : "));
  Serial.println(LED_COUNT);
  Serial.print(F("✓ Luminosité : "));
  Serial.print(LED_BRIGHTNESS);
  Serial.println(F("/255"));
  Serial.println();
  
  delay(1000);
  printMenu();
}

// ============================================
// LOOP
// ============================================
void loop() {
  if (Serial.available()) {
    handleCommand(Serial.read());
    while (Serial.available()) Serial.read();
  }
  
  delay(10);
}

// ============================================
// TESTS VALIDATION MATÉRIELLE
// ============================================
/**
 * @brief Test 1 : Validation LEDs individuelles
 */
void testIndividual() {
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║ TEST 1 : VALIDATION LEDS INDIVIDUELLES ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F("Objectif : Vérifier que les 8 LEDs fonctionnent"));
  Serial.println(F("Chaque LED s'allume en blanc, une par une."));
  Serial.println();
  
  FastLED.clear();
  FastLED.show();
  delay(500);
  
  for (int i = 0; i < LED_COUNT; i++) {
    Serial.print(F("LED #"));
    Serial.print(i);
    Serial.print(F(" : "));
    
    leds[i] = CRGB::White;
    FastLED.show();
    delay(600);
    
    Serial.println(F("✓"));
    
    leds[i] = CRGB::Black;
    FastLED.show();
    delay(200);
  }
  
  Serial.println(F("\n✓ Test terminé"));
  Serial.println(F("Résultat attendu : Les 8 LEDs se sont allumées"));
  Serial.println(F("Si une LED ne s'allume pas → problème hardware\n"));
  
  waitForKey();
}

/**
 * @brief Test 2 : Validation couleurs primaires
 */
void testPrimaryColors() {
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║ TEST 2 : VALIDATION COULEURS           ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F("Objectif : Vérifier RGB correct."));
  Serial.println();
  
  // Rouge
  Serial.println(F("Couleur : ROUGE"));
  fill_solid(leds, LED_COUNT, CRGB::Red);
  FastLED.show();
  waitForKey();
  
  // Vert
  Serial.println(F("Couleur : VERT"));
  fill_solid(leds, LED_COUNT, CRGB::Green);
  FastLED.show();
  waitForKey();
  
  // Bleu
  Serial.println(F("Couleur : BLEU"));
  fill_solid(leds, LED_COUNT, CRGB::Blue);
  FastLED.show();
  waitForKey();
  
  // Orange
  Serial.println(F("Couleur : ORANGE (alerte)"));
  fill_solid(leds, LED_COUNT, CRGB::Orange);
  FastLED.show();
  waitForKey();
  
  FastLED.clear();
  FastLED.show();
  
  Serial.println(F("\n✓ Test terminé"));
  Serial.println(F("Résultat attendu : 4 couleurs distinctes et correctes\n"));
}

// ============================================
// SCÉNARIOS RÉALISTES
// ============================================
/**
 * @brief Scénario 1 : Nuit normale dans le van
 * 
 * @details
 * Simulation d'une nuit complète :
 * - 22h : Tout OK
 * - Minuit : Humidité commence à monter
 * - 3h : Humidité > 70% → Extracteur devrait se déclencher
 * - 4h : Extracteur a fonctionné, humidité redescend
 * - 7h : Tout rentré dans l'ordre
 */
void scenario1_NormalNight() {
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║ SCÉNARIO 1 : NUIT NORMALE DANS LE VAN  ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F("Simulation : Évolution naturelle humidité/température"));
  Serial.println();
  
  // État 1 : 22h - Tout OK
  Serial.println(F("─── 22h00 : Coucher ───"));
  Serial.println(F("État : Tout OK"));
  Serial.println(F("  • Température : 18°C"));
  Serial.println(F("  • Humidité : 55%"));
  Serial.println(F("  • Batterie : 12.8V"));
  Serial.println(F("  • Gaz : RAS"));
  Serial.println();
  
  fill_solid(leds, LED_COUNT, CRGB::Green);
  FastLED.show();
  delay(3000);
  
  // État 2 : Minuit - Humidité monte
  Serial.println(F("\n─── 00h00 : Milieu de nuit ───"));
  Serial.println(F("État : Humidité commence à monter (respiration)"));
  Serial.println(F("  • Température : 16°C"));
  Serial.println(F("  • Humidité : 65% ⚠"));
  Serial.println(F("  • Batterie : 12.7V"));
  Serial.println(F("  • Gaz : RAS"));
  Serial.println();
  Serial.println(F("Affichage : LEDs 0-1 passent ORANGE (zone humidité)"));
  Serial.println();
  
  leds[0] = CRGB::Orange;  // Humidité
  leds[1] = CRGB::Orange;
  leds[2] = CRGB::Green;   // Batterie OK
  leds[3] = CRGB::Green;
  leds[4] = CRGB::Green;   // Gaz OK
  leds[5] = CRGB::Green;
  leds[6] = CRGB::Green;   // Système OK
  leds[7] = CRGB::Green;
  FastLED.show();
  delay(3000);
  
  // État 3 : 3h - Alerte humidité
  Serial.println(F("\n─── 03h00 : Humidité haute ───"));
  Serial.println(F("État : ALERTE Humidité > 70%"));
  Serial.println(F("  • Température : 15°C ⚠"));
  Serial.println(F("  • Humidité : 72% ❌"));
  Serial.println(F("  • Batterie : 12.6V"));
  Serial.println(F("  • Gaz : RAS"));
  Serial.println();
  Serial.println(F("→ EXTRACTEUR AUTO DEVRAIT SE DÉCLENCHER"));
  Serial.println(F("Affichage : Plus de LEDs ORANGE (alerte étendue)"));
  Serial.println();
  
  leds[0] = CRGB::Orange;
  leds[1] = CRGB::Orange;
  leds[2] = CRGB::Orange;  // Température aussi
  leds[3] = CRGB::Orange;
  leds[4] = CRGB::Green;
  leds[5] = CRGB::Green;
  leds[6] = CRGB::Orange;  // Système en action
  leds[7] = CRGB::Green;
  FastLED.show();
  delay(3000);
  
  // État 4 : 4h - Extracteur a fonctionné
  Serial.println(F("\n─── 04h00 : Extracteur a fonctionné ───"));
  Serial.println(F("État : Amélioration en cours"));
  Serial.println(F("  • Température : 14°C (air extérieur)"));
  Serial.println(F("  • Humidité : 62% → en baisse"));
  Serial.println(F("  • Batterie : 12.5V"));
  Serial.println(F("  • Gaz : RAS"));
  Serial.println();
  Serial.println(F("Affichage : Retour progressif au vert"));
  Serial.println();
  
  leds[0] = CRGB::Orange;
  leds[1] = CRGB::Green;   // Amélioration
  leds[2] = CRGB::Green;
  leds[3] = CRGB::Green;
  leds[4] = CRGB::Green;
  leds[5] = CRGB::Green;
  leds[6] = CRGB::Green;
  leds[7] = CRGB::Green;
  FastLED.show();
  delay(3000);
  
  // État 5 : 7h - Tout OK
  Serial.println(F("\n─── 07h00 : Réveil ───"));
  Serial.println(F("État : Tout rentré dans l'ordre"));
  Serial.println(F("  • Température : 15°C"));
  Serial.println(F("  • Humidité : 58%"));
  Serial.println(F("  • Batterie : 12.5V"));
  Serial.println(F("  • Gaz : RAS"));
  Serial.println();
  Serial.println(F("Affichage : Tout VERT"));
  Serial.println();
  
  fill_solid(leds, LED_COUNT, CRGB::Green);
  FastLED.show();
  delay(3000);
  
  FastLED.clear();
  FastLED.show();
  
  Serial.println(F("\n✓ Scénario terminé"));
  Serial.println(F("Concept démontré : Gestion automatique extracteur\n"));
}

/**
 * @brief Scénario 2 : Décharge batterie progressive
 */
void scenario2_BatteryDischarge() {
  Serial.println(F("\n╔════════════════════════════════════════╗"));
  Serial.println(F("║ SCÉNARIO 2 : DÉCHARGE BATTERIE        ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F("\nSimulation : Utilisation prolongée sans recharge\n"));
  
  // État 1 : Batterie pleine
  Serial.println(F("─── Départ : Batterie OK ───"));
  Serial.println(F("  • Batterie 12V : 12.8V"));
  Serial.println(F("  • Tout fonctionne normalement"));
  Serial.println();
  
  fill_solid(leds, LED_COUNT, CRGB::Green);
  FastLED.show();
  waitForKey();
  
  // État 2 : Batterie moyenne
  Serial.println(F("\n─── Après 2h d'utilisation ───"));
  Serial.println(F("  • Batterie 12V : 12.2V ⚠"));
  Serial.println(F("  • Attention : batterie se décharge"));
  Serial.println();
  Serial.println(F("Affichage : LEDs batterie ORANGE"));
  Serial.println();
  
  leds[0] = CRGB::Green;
  leds[1] = CRGB::Green;
  leds[2] = CRGB::Orange;  // Batterie
  leds[3] = CRGB::Orange;
  leds[4] = CRGB::Green;
  leds[5] = CRGB::Green;
  leds[6] = CRGB::Green;
  leds[7] = CRGB::Green;
  FastLED.show();
  waitForKey();
  
  // État 3 : Batterie faible
  Serial.println(F("\n─── Après 4h : ALERTE ───"));
  Serial.println(F("  • Batterie 12V : 11.8V ❌"));
  Serial.println(F("  • ALERTE : Risque décharge profonde"));
  Serial.println(F("  • Action : Couper charges non essentielles"));
  Serial.println();
  Serial.println(F("Affichage : LEDs batterie ROUGE"));
  Serial.println();
  
  leds[0] = CRGB::Green;
  leds[1] = CRGB::Green;
  leds[2] = CRGB::Red;     // Alerte batterie
  leds[3] = CRGB::Red;
  leds[4] = CRGB::Green;
  leds[5] = CRGB::Green;
  leds[6] = CRGB::Orange;  // Système alerte
  leds[7] = CRGB::Green;
  FastLED.show();
  waitForKey();
  
  // État 4 : Recharge
  Serial.println(F("\n─── Démarrage moteur : Recharge ───"));
  Serial.println(F("  • Batterie 12V : 13.8V (en charge)"));
  Serial.println(F("  • Alternateur actif"));
  Serial.println();
  Serial.println(F("Affichage : Clignotement VERT (recharge en cours)"));
  Serial.println();
  
  for (int i = 0; i < 6; i++) {
    leds[2] = CRGB::Green;
    leds[3] = CRGB::Green;
    FastLED.show();
    delay(300);
    
    leds[2] = CRGB::Black;
    leds[3] = CRGB::Black;
    FastLED.show();
    delay(300);
  }
  
  fill_solid(leds, LED_COUNT, CRGB::Green);
  FastLED.show();
  waitForKey();
  
  FastLED.clear();
  FastLED.show();
  
  Serial.println(F("\n✓ Scénario terminé"));
  Serial.println(F("Concept démontré : Surveillance batterie + alertes\n"));
}

/**
 * @brief Scénario 3 : Détection CO (urgence)
 */
void scenario3_COEmergency() {
  Serial.println(F("\n╔════════════════════════════════════════╗"));
  Serial.println(F("║ SCÉNARIO 3 : DÉTECTION CO (URGENCE)   ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F("\nSimulation : Alerte CO critique\n"));
  
  // État 1 : Normal
  Serial.println(F("─── État normal ───"));
  Serial.println(F("  • CO : 0 ppm"));
  Serial.println(F("  • Tout OK"));
  Serial.println();
  
  fill_solid(leds, LED_COUNT, CRGB::Green);
  FastLED.show();
  waitForKey();
  
  // État 2 : Début détection
  Serial.println(F("\n─── Début détection CO ───"));
  Serial.println(F("  • CO : 35 ppm ⚠"));
  Serial.println(F("  • Attention : niveau élevé"));
  Serial.println();
  Serial.println(F("Affichage : LEDs gaz ORANGE"));
  Serial.println();
  
  leds[0] = CRGB::Green;
  leds[1] = CRGB::Green;
  leds[2] = CRGB::Green;
  leds[3] = CRGB::Green;
  leds[4] = CRGB::Orange;  // Gaz
  leds[5] = CRGB::Orange;
  leds[6] = CRGB::Green;
  leds[7] = CRGB::Green;
  FastLED.show();
  waitForKey();
  
  // État 3 : ALERTE CRITIQUE
  Serial.println(F("\n─── ❌ ALERTE CO CRITIQUE ❌ ───"));
  Serial.println(F("  • CO : 150 ppm ❌ DANGER !"));
  Serial.println(F("  • ACTION IMMÉDIATE :"));
  Serial.println(F("    1. OUVRIR PORTES/FENÊTRES"));
  Serial.println(F("    2. COUPER CHAUFFAGE"));
  Serial.println(F("    3. ÉVACUER SI NÉCESSAIRE"));
  Serial.println();
  Serial.println(F("Affichage : TOUT ROUGE + CLIGNOTEMENT RAPIDE"));
  Serial.println(F("            + BUZZER CONTINU"));
  Serial.println();
  
  // Clignotement rapide urgent
  for (int i = 0; i < 15; i++) {
    fill_solid(leds, LED_COUNT, CRGB::Red);
    FastLED.show();
    delay(150);
    
    FastLED.clear();
    FastLED.show();
    delay(150);
  }
  
  fill_solid(leds, LED_COUNT, CRGB::Red);
  FastLED.show();
  waitForKey();
  
  // État 4 : Aération, niveau baisse
  Serial.println(F("\n─── Aération en cours ───"));
  Serial.println(F("  • CO : 80 ppm → en baisse"));
  Serial.println(F("  • Portes/fenêtres ouvertes"));
  Serial.println();
  Serial.println(F("Affichage : ORANGE (danger diminue)"));
  Serial.println();
  
  leds[4] = CRGB::Orange;
  leds[5] = CRGB::Orange;
  leds[6] = CRGB::Orange;
  leds[7] = CRGB::Orange;
  FastLED.show();
  waitForKey();
  
  // État 5 : Retour normal
  Serial.println(F("\n─── Retour à la normale ───"));
  Serial.println(F("  • CO : 5 ppm"));
  Serial.println(F("  • Danger écarté"));
  Serial.println();
  
  fill_solid(leds, LED_COUNT, CRGB::Green);
  FastLED.show();
  waitForKey();
  
  FastLED.clear();
  FastLED.show();
  
  Serial.println(F("\n✓ Scénario terminé"));
  Serial.println(F("Concept démontré : Alerte urgence + actions immédiates\n"));
}

// ============================================
// FONCTIONS UTILITAIRES
// ============================================

/**
 * @brief Attend appui sur n'importe quelle touche
 */
void waitForKey() {
  Serial.println(F("  [Appuie sur une touche pour continuer]"));
  while (!Serial.available()) delay(10);
  while (Serial.available()) Serial.read();
  Serial.println();
}

/**
 * @brief Affiche menu
 */
void printMenu() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      MENU TEST BANDEAU LED             ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("═══ TESTS VALIDATION MATÉRIELLE ═══"));
  Serial.println(F("  1 - Test LEDs individuelles"));
  Serial.println(F("  2 - Test couleurs primaires"));
  Serial.println();
  Serial.println(F("═══ SCÉNARIOS RÉALISTES PROJET ═══"));
  Serial.println(F("  3 - Scénario 1 : Nuit normale (humidité)"));
  Serial.println(F("  4 - Scénario 2 : Décharge batterie"));
  Serial.println(F("  5 - Scénario 3 : Détection CO (urgence)"));
  Serial.println();
  Serial.println(F("  0 - Éteindre LEDs"));
  Serial.println(F("  H - Afficher cette aide"));
  Serial.println();
}

/**
 * @brief Gestion commandes
 */
void handleCommand(char cmd) {
  switch (cmd) {
    case '1':
      testIndividual();
      printMenu();
      break;
    case '2':
      testPrimaryColors();
      printMenu();
      break;
    case '3':
      scenario1_NormalNight();
      printMenu();
      break;
    case '4':
      scenario2_BatteryDischarge();
      printMenu();
      break;
    case '5':
      scenario3_COEmergency();
      printMenu();
      break;
    case '0':
      Serial.println(F("\n✓ LEDs éteintes\n"));
      FastLED.clear();
      FastLED.show();
      break;
    case 'h':
    case 'H':
      printMenu();
      break;
    default:
      if (cmd >= 32 && cmd <= 126) {
        Serial.print(F("Commande inconnue : "));
        Serial.println(cmd);
      }
      break;
  }
}
