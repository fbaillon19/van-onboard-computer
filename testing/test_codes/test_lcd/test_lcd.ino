/**
 * @file test_lcd.ino
 * @brief Programme de test de l'écran LCD 20x4 I2C
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-25
 * 
 * @details
 * Programme de test pour validation matérielle du LCD 20x4.
 * 
 * Objectifs :
 * - Vérifier la présence du LCD sur I2C
 * - Valider le câblage et l'alimentation
 * - Tester l'affichage sur les 4 lignes
 * - Tester le rétro-éclairage
 * - Tester les différents types d'affichage (centré, aligné, etc.)
 * - Comprendre le comportement du LCD
 * 
 * Connexions :
 * - VCC  → 5V
 * - GND  → GND
 * - SCL  → Pin 21 (Arduino MEGA)
 * - SDA  → Pin 20 (Arduino MEGA)
 * 
 * @warning Le LCD fonctionne en 5V uniquement
 * @note Vérifier l'adresse I2C de votre module (0x27 ou 0x3F)
 */

#include "LCDDisplay.h"

// ============================================
// CONFIGURATION
// ============================================
#define SERIAL_BAUD   115200

// ============================================
// VARIABLES GLOBALES
// ============================================
LCDDisplay lcd;

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(3000);
  
  Serial.println();
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   TEST LCD 20x4 - Vérification Hardware║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  // Initialiser I2C
  Serial.println(F("Initialisation I2C..."));
  Wire.begin();
  Serial.println(F("✓ I2C initialisé\n"));
  
  // Scanner I2C
  scanI2C();
  
  // Initialiser LCD
  Serial.print(F("Initialisation LCD (adresse 0x"));
  Serial.print(LCD_I2C_ADDR, HEX);
  Serial.println(F(")..."));
  Serial.println();
  
  if (!lcd.begin()) {
    Serial.println(F("❌ ÉCHEC initialisation LCD"));
    Serial.println();
    Serial.println(F("Vérifications à faire :"));
    Serial.println(F("  1. Câblage VCC (5V), GND, SDA, SCL"));
    Serial.println(F("  2. Adresse I2C (0x27 ou 0x3F ?)"));
    Serial.println(F("  3. Contraste du LCD (potentiomètre au dos)"));
    Serial.println(F("  4. Pull-ups I2C présentes"));
    Serial.println();
    Serial.println(F("Programme arrêté."));
    
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println(F("✓ LCD prêt !"));
  Serial.println();
  
  // Afficher message de bienvenue sur LCD
  lcd.clear();
  lcd.printCenter("TEST LCD 20x4", 0);
  lcd.printCenter("Initialisation OK", 1);
  delay(2000);
  
  // Menu
  printMenu();
}

// ============================================
// LOOP
// ============================================
void loop() {
  // Attendre commandes série
  if (Serial.available()) {
    handleCommand(Serial.read());
    while (Serial.available()) Serial.read(); // Vider buffer
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
      testBasicText();
      break;
      
    case '2':
      testAlignment();
      break;
      
    case '3':
      testAllLines();
      break;
      
    case '4':
      testBacklight();
      break;
      
    case '5':
      testProgressBar();
      break;
      
    case '6':
      testScrolling();
      break;
      
    case '7':
      testCustomChars();
      break;
      
    case '8':
      testValues();
      break;
      
    case '9':
      testTitle();
      break;
      
    case 'c':
    case 'C':
      lcd.clear();
      Serial.println(F("✓ Écran effacé"));
      break;
      
    case 'b':
    case 'B':
      lcd.backlightToggle();
      Serial.print(F("✓ Rétro-éclairage : "));
      Serial.println(lcd.isBacklightOn() ? F("ON") : F("OFF"));
      break;
      
    case 'i':
    case 'I':
      scanI2C();
      break;
      
    case 'h':
    case 'H':
      printMenu();
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
 * @brief Affiche le menu des commandes
 */
void printMenu() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║      MENU TEST LCD 20x4                ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("  1 - Test texte basique (4 lignes)"));
  Serial.println(F("  2 - Test alignement (gauche/centre/droite)"));
  Serial.println(F("  3 - Test toutes les lignes"));
  Serial.println(F("  4 - Test rétro-éclairage"));
  Serial.println(F("  5 - Test barre de progression"));
  Serial.println(F("  6 - Test défilement de texte"));
  Serial.println(F("  7 - Test caractères personnalisés"));
  Serial.println(F("  8 - Test affichage valeurs"));
  Serial.println(F("  9 - Test titre encadré"));
  Serial.println(F("  C - Effacer écran"));
  Serial.println(F("  B - Toggle rétro-éclairage"));
  Serial.println(F("  H - Afficher cette aide"));
  Serial.println(F("  I - Scanner I2C"));
  Serial.println();
}

// ============================================
// FONCTIONS DE TEST
// ============================================
/**
 * @brief Test 1 : Affichage texte basique
 */
void testBasicText() {
  Serial.println(F("=== TEST 1 : AFFICHAGE TEXTE BASIQUE ==="));
  
  lcd.clear();
  lcd.printAt(0, 0, "Ligne 1 - Debut");
  lcd.printAt(0, 1, "Ligne 2 - Milieu");
  lcd.printAt(0, 2, "Ligne 3 - Encore");
  lcd.printAt(0, 3, "Ligne 4 - Fin");
  
  Serial.println(F("✓ Texte affiché sur les 4 lignes"));
  Serial.println(F("  Vérifiez que tout est lisible"));
  Serial.println();
}

/**
 * @brief Test 2 : Alignement du texte
 */
void testAlignment() {
  Serial.println(F("=== TEST 2 : ALIGNEMENT ==="));
  
  lcd.clear();
  lcd.printLine("Gauche", 0, LCDAlignment::LEFT);
  lcd.printCenter("Centre", 1);
  lcd.printRight("Droite", 2);
  lcd.printAt(5, 3, "Position 5");
  
  Serial.println(F("✓ Ligne 0 : Aligné à gauche"));
  Serial.println(F("✓ Ligne 1 : Centré"));
  Serial.println(F("✓ Ligne 2 : Aligné à droite"));
  Serial.println(F("✓ Ligne 3 : Position spécifique"));
  Serial.println();
}

/**
 * @brief Test 3 : Toutes les lignes avec numérotation
 */
void testAllLines() {
  Serial.println(F("=== TEST 3 : TOUTES LES LIGNES ==="));
  
  lcd.clear();
  
  for (uint8_t i = 0; i < 4; i++) {
    char buffer[21];
    snprintf(buffer, sizeof(buffer), "Ligne %d : 0123456789", i);
    lcd.printAt(0, i, buffer);
    delay(500);
  }
  
  Serial.println(F("✓ Affichage séquentiel des 4 lignes"));
  Serial.println(F("  Vérifiez la numérotation 0-9"));
  Serial.println();
}

/**
 * @brief Test 4 : Rétro-éclairage
 */
void testBacklight() {
  Serial.println(F("=== TEST 4 : RÉTRO-ÉCLAIRAGE ==="));
  
  lcd.clear();
  lcd.printCenter("Test retro-eclairage", 1);
  
  Serial.println(F("Clignotement 5 fois..."));
  
  for (int i = 0; i < 5; i++) {
    lcd.backlightOff();
    delay(500);
    lcd.backlightOn();
    delay(500);
  }
  
  Serial.println(F("✓ Test rétro-éclairage terminé"));
  Serial.println();
}

/**
 * @brief Test 5 : Barre de progression
 */
void testProgressBar() {
  Serial.println(F("=== TEST 5 : BARRE DE PROGRESSION ==="));
  
  lcd.clear();
  lcd.printCenter("Barre de progression", 0);
  
  Serial.println(F("Animation 0% → 100%..."));
  
  for (uint8_t i = 0; i <= 100; i += 5) {
    lcd.printProgressBar(2, i, "Load");
    
    // Afficher pourcentage sur ligne 3
    char buffer[21];
    snprintf(buffer, sizeof(buffer), "%d%%", i);
    lcd.printCenter(buffer, 3);
    
    delay(100);
  }
  
  delay(1000);
  
  Serial.println(F("✓ Animation terminée"));
  Serial.println();
}

/**
 * @brief Test 6 : Défilement de texte
 */
void testScrolling() {
  Serial.println(F("=== TEST 6 : DÉFILEMENT ==="));
  
  lcd.clear();
  lcd.printCenter("Test defilement", 0);
  
  const char* longText = "Ce texte est trop long pour l'ecran et va defiler...";
  uint8_t len = strlen(longText);
  
  Serial.println(F("Défilement horizontal..."));
  
  for (uint8_t i = 0; i <= len - LCD_COLS; i++) {
    lcd.clearLine(2);
    lcd.printAt(0, 2, longText + i);
    delay(200);
  }
  
  delay(1000);
  
  Serial.println(F("✓ Défilement terminé"));
  Serial.println();
}

/**
 * @brief Test 7 : Caractères personnalisés
 */
void testCustomChars() {
  Serial.println(F("=== TEST 7 : CARACTÈRES PERSONNALISÉS ==="));
  
  // Définir caractères personnalisés
  
  // Cœur
  uint8_t heart[8] = {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000
  };
  
  // Smiley
  uint8_t smiley[8] = {
    0b00000,
    0b01010,
    0b01010,
    0b00000,
    0b10001,
    0b01110,
    0b00000,
    0b00000
  };
  
  // Thermomètre
  uint8_t thermometer[8] = {
    0b00100,
    0b01010,
    0b01010,
    0b01110,
    0b01110,
    0b11111,
    0b11111,
    0b01110
  };
  
  lcd.createChar(0, heart);
  lcd.createChar(1, smiley);
  lcd.createChar(2, thermometer);
  
  lcd.clear();
  lcd.printCenter("Caracteres custom", 0);
  
  lcd.printAt(5, 2, "Coeur:");
  lcd.printCustomChar(12, 2, 0);
  
  lcd.printAt(5, 3, "Smiley:");
  lcd.printCustomChar(13, 3, 1);
  
  lcd.printAt(0, 1, "Temp:");
  lcd.printCustomChar(6, 1, 2);
  lcd.printAt(8, 1, "23.5C");
  
  Serial.println(F("✓ Caractères personnalisés affichés"));
  Serial.println(F("  Vérifiez les icônes : cœur, smiley, thermomètre"));
  Serial.println();
}

/**
 * @brief Test 8 : Affichage de valeurs
 */
void testValues() {
  Serial.println(F("=== TEST 8 : AFFICHAGE VALEURS ==="));
  
  lcd.clear();
  lcd.printCenter("Valeurs simulees", 0);
  
  Serial.println(F("Animation de valeurs..."));
  
  for (int i = 0; i < 20; i++) {
    float temp = 20.0 + (i * 0.5);
    float humidity = 45.0 + (i * 1.0);
    int battery = 80 + i;
    
    lcd.printValue(1, "Temp", temp, "C", 1);
    lcd.printValue(2, "Humid", humidity, "%", 0);
    lcd.printValue(3, "Batt", battery, "%", 0);
    
    delay(200);
  }
  
  Serial.println(F("✓ Animation valeurs terminée"));
  Serial.println();
}

/**
 * @brief Test 9 : Titre encadré
 */
void testTitle() {
  Serial.println(F("=== TEST 9 : TITRE ENCADRÉ ==="));
  
  lcd.clear();
  lcd.printTitle("VAN COMPUTER");
  delay(2000);
  
  lcd.printTitle("SYSTEME OK");
  
  Serial.println(F("✓ Titres affichés"));
  Serial.println();
}

// ============================================
// FONCTIONS UTILITAIRES
// ============================================
/**
 * @brief Scanne le bus I2C
 */
void scanI2C() {
  Serial.println(F("\n=== SCAN I2C ==="));
  byte count = 0;
  
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  0x"));
      if (addr < 16) Serial.print(F("0"));
      Serial.print(addr, HEX);
      
      if (addr == 0x27 || addr == 0x3F) {
        Serial.print(F(" - LCD ✓"));
      }
      Serial.println();
      count++;
    }
  }
  
  Serial.print(F("\nTotal: "));
  Serial.print(count);
  Serial.println(F(" périphérique(s)\n"));
  
  if (count == 0) {
    Serial.println(F("⚠️  Aucun périphérique I2C détecté !"));
  }
}
