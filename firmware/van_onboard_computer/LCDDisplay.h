/**
 * @file LCDDisplay.h
 * @brief Classe de gestion de l'écran LCD I2C 20x4
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-25
 * 
 * @details
 * Classe décou plée pour l'écran LCD 20x4 avec interface I2C (PCF8574).
 * Simplifie l'utilisation de l'écran avec des méthodes de haut niveau.
 * 
 * Caractéristiques :
 * - Affichage 20 colonnes × 4 lignes
 * - Interface I2C (adresse 0x27 ou 0x3F selon module)
 * - Rétro-éclairage contrôlable
 * - Caractères personnalisés (8 maximum)
 * 
 * @note Compatible avec la bibliothèque LiquidCrystal_I2C
 * @warning Vérifier l'adresse I2C de votre module avant utilisation
 */

#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================
// CONFIGURATION MATÉRIELLE
// ============================================
#define LCD_I2C_ADDR        0x27      ///< Adresse I2C (0x27 ou 0x3F selon module)
#define LCD_COLS            20        ///< Nombre de colonnes
#define LCD_ROWS            4         ///< Nombre de lignes

// ============================================
// TYPES ET STRUCTURES
// ============================================
/**
 * @enum LCDStatus
 * @brief État de l'écran LCD
 */
enum class LCDStatus {
  NOT_INITIALIZED,   ///< LCD non initialisé
  READY,            ///< LCD prêt et fonctionnel
  ERROR_NOT_FOUND,  ///< LCD non détecté sur I2C
  ERROR_COMM        ///< Erreur de communication
};

/**
 * @enum LCDAlignment
 * @brief Alignement du texte
 */
enum class LCDAlignment {
  LEFT,     ///< Alignement à gauche
  CENTER,   ///< Alignement centré
  RIGHT     ///< Alignement à droite
};

// ============================================
// DÉFINITION CLASSE LCDDisplay
// ============================================
/**
 * @class LCDDisplay
 * @brief Classe de gestion de l'écran LCD 20x4 I2C
 * 
 * @details
 * Classe découplée avec méthodes de haut niveau pour simplifier l'utilisation.
 * Gère l'affichage, le positionnement, l'alignement et le rétro-éclairage.
 * 
 * Exemple d'utilisation :
 * @code
 * LCDDisplay lcd;
 * 
 * void setup() {
 *   if (lcd.begin()) {
 *     lcd.printCenter("Bonjour", 0);
 *     lcd.printCenter("Van Computer", 1);
 *   }
 * }
 * 
 * void loop() {
 *   lcd.printAt(0, 3, "Temp: 23.5°C");
 *   delay(1000);
 * }
 * @endcode
 */
class LCDDisplay {
private:
  LiquidCrystal_I2C lcd;          ///< Instance bibliothèque LCD
  LCDStatus status;               ///< État du LCD
  uint8_t i2cAddress;             ///< Adresse I2C
  bool backlightState;            ///< État du rétro-éclairage
  
public:
  /**
   * @brief Constructeur
   * @param addr Adresse I2C du LCD (0x27 ou 0x3F)
   */
  LCDDisplay(uint8_t addr = LCD_I2C_ADDR) 
    : lcd(addr, LCD_COLS, LCD_ROWS),
      status(LCDStatus::NOT_INITIALIZED),
      i2cAddress(addr),
      backlightState(true)
  {
  }

  // INITIALISATION
  // ------------------------------------------
  /**
   * @brief Initialise l'écran LCD
   * @return true si succès, false si échec
   * 
   * @details
   * - Vérifie la présence du LCD sur le bus I2C
   * - Initialise la communication
   * - Active le rétro-éclairage
   * - Efface l'écran
   */
  bool begin() {
    // Vérifier présence I2C
    Wire.beginTransmission(i2cAddress);
    if (Wire.endTransmission() != 0) {
      status = LCDStatus::ERROR_NOT_FOUND;
      return false;
    }
    
    // Initialiser LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    
    status = LCDStatus::READY;
    backlightState = true;
    
    return true;
  }

  /**
   * @brief Vérifie si le LCD est initialisé
   * @return true si prêt
   */
  bool isReady() const {
    return status == LCDStatus::READY;
  }

  /**
   * @brief Retourne l'état du LCD
   * @return État actuel
   */
  LCDStatus getStatus() const {
    return status;
  }

  /**
   * @brief Retourne l'adresse I2C
   * @return Adresse I2C
   */
  uint8_t getI2CAddress() const {
    return i2cAddress;
  }

  // AFFICHAGE TEXTE
  // ------------------------------------------
  /**
   * @brief Efface l'écran
   */
  void clear() {
    if (status != LCDStatus::READY) return;
    lcd.clear();
  }

  /**
   * @brief Efface une ligne spécifique
   * @param row Numéro de ligne (0-3)
   */
  void clearLine(uint8_t row) {
    if (status != LCDStatus::READY) return;
    if (row >= LCD_ROWS) return;
    
    lcd.setCursor(0, row);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
      lcd.print(' ');
    }
  }

  /**
   * @brief Positionne le curseur
   * @param col Colonne (0-19)
   * @param row Ligne (0-3)
   */
  void setCursor(uint8_t col, uint8_t row) {
    if (status != LCDStatus::READY) return;
    lcd.setCursor(col, row);
  }

  /**
   * @brief Affiche du texte à une position
   * @param col Colonne (0-19)
   * @param row Ligne (0-3)
   * @param text Texte à afficher
   */
  void printAt(uint8_t col, uint8_t row, const char* text) {
    if (status != LCDStatus::READY) return;
    if (row >= LCD_ROWS) return;
    
    lcd.setCursor(col, row);
    lcd.print(text);
  }

  /**
   * @brief Affiche du texte à une position (version String)
   * @param col Colonne (0-19)
   * @param row Ligne (0-3)
   * @param text Texte à afficher
   */
  void printAt(uint8_t col, uint8_t row, const String& text) {
    printAt(col, row, text.c_str());
  }

  /**
   * @brief Affiche du texte centré sur une ligne
   * @param text Texte à afficher
   * @param row Ligne (0-3)
   */
  void printCenter(const char* text, uint8_t row) {
    if (status != LCDStatus::READY) return;
    if (row >= LCD_ROWS) return;
    
    uint8_t len = strlen(text);
    if (len > LCD_COLS) len = LCD_COLS;
    
    uint8_t startCol = (LCD_COLS - len) / 2;
    
    // Effacer la ligne
    clearLine(row);
    
    // Afficher centré
    lcd.setCursor(startCol, row);
    lcd.print(text);
  }

  /**
   * @brief Affiche du texte centré (version String)
   * @param text Texte à afficher
   * @param row Ligne (0-3)
   */
  void printCenter(const String& text, uint8_t row) {
    printCenter(text.c_str(), row);
  }

  /**
   * @brief Affiche du texte aligné à droite
   * @param text Texte à afficher
   * @param row Ligne (0-3)
   */
  void printRight(const char* text, uint8_t row) {
    if (status != LCDStatus::READY) return;
    if (row >= LCD_ROWS) return;
    
    uint8_t len = strlen(text);
    if (len > LCD_COLS) len = LCD_COLS;
    
    uint8_t startCol = LCD_COLS - len;
    
    lcd.setCursor(startCol, row);
    lcd.print(text);
  }

  /**
   * @brief Affiche du texte aligné (version String)
   * @param text Texte à afficher
   * @param row Ligne (0-3)
   */
  void printRight(const String& text, uint8_t row) {
    printRight(text.c_str(), row);
  }

  /**
   * @brief Affiche une ligne complète avec alignement
   * @param text Texte à afficher
   * @param row Ligne (0-3)
   * @param align Alignement (LEFT, CENTER, RIGHT)
   */
  void printLine(const char* text, uint8_t row, LCDAlignment align = LCDAlignment::LEFT) {
    if (status != LCDStatus::READY) return;
    
    clearLine(row);
    
    switch (align) {
      case LCDAlignment::LEFT:
        printAt(0, row, text);
        break;
      case LCDAlignment::CENTER:
        printCenter(text, row);
        break;
      case LCDAlignment::RIGHT:
        printRight(text, row);
        break;
    }
  }

  // RÉTRO-ÉCLAIRAGE
  // ------------------------------------------
  /**
   * @brief Active le rétro-éclairage
   */
  void backlightOn() {
    if (status != LCDStatus::READY) return;
    lcd.backlight();
    backlightState = true;
  }

  /**
   * @brief Désactive le rétro-éclairage
   */
  void backlightOff() {
    if (status != LCDStatus::READY) return;
    lcd.noBacklight();
    backlightState = false;
  }

  /**
   * @brief Inverse l'état du rétro-éclairage
   */
  void backlightToggle() {
    if (backlightState) {
      backlightOff();
    } else {
      backlightOn();
    }
  }

  /**
   * @brief Définit l'état du rétro-éclairage
   * @param state true pour allumer, false pour éteindre
   */
  void setBacklight(bool state) {
    if (state) {
      backlightOn();
    } else {
      backlightOff();
    }
  }

  /**
   * @brief Retourne l'état du rétro-éclairage
   * @return true si allumé
   */
  bool isBacklightOn() const {
    return backlightState;
  }

  // AFFICHAGE SPÉCIALISÉ
  // ------------------------------------------
  /**
   * @brief Affiche une barre de progression
   * @param row Ligne (0-3)
   * @param percent Pourcentage (0-100)
   * @param label Étiquette optionnelle
   */
  void printProgressBar(uint8_t row, uint8_t percent, const char* label = nullptr) {
    if (status != LCDStatus::READY) return;
    if (row >= LCD_ROWS) return;
    if (percent > 100) percent = 100;
    
    clearLine(row);
    
    uint8_t startCol = 0;
    uint8_t barWidth = LCD_COLS;
    
    // Si label, réduire largeur barre
    if (label != nullptr) {
      uint8_t labelLen = strlen(label);
      lcd.setCursor(0, row);
      lcd.print(label);
      startCol = labelLen + 1;
      barWidth = LCD_COLS - startCol;
    }
    
    // Calculer nombre de blocs pleins
    uint8_t filledBlocks = (barWidth * percent) / 100;
    
    lcd.setCursor(startCol, row);
    lcd.print('[');
    
    for (uint8_t i = 0; i < barWidth - 2; i++) {
      if (i < filledBlocks) {
        lcd.write(0xFF); // Bloc plein
      } else {
        lcd.print(' ');
      }
    }
    
    lcd.print(']');
  }

  /**
   * @brief Affiche un titre encadré
   * @param title Titre à afficher
   * @param row Ligne de départ (0-2)
   */
  void printTitle(const char* title) {
    if (status != LCDStatus::READY) return;
    
    // Ligne du haut
    clearLine(0);
    lcd.setCursor(0, 0);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
      lcd.print('=');
    }
    
    // Titre centré
    printCenter(title, 1);
    
    // Ligne du bas
    clearLine(2);
    lcd.setCursor(0, 2);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
      lcd.print('=');
    }
  }

  /**
   * @brief Affiche une valeur avec unité
   * @param row Ligne (0-3)
   * @param label Étiquette
   * @param value Valeur
   * @param unit Unité
   * @param decimals Nombre de décimales
   */
  void printValue(uint8_t row, const char* label, float value, const char* unit, uint8_t decimals = 1) {
    if (status != LCDStatus::READY) return;
    if (row >= LCD_ROWS) return;
    
    char buffer[21];
    char valueStr[10];
    
    dtostrf(value, 0, decimals, valueStr);
    snprintf(buffer, sizeof(buffer), "%s: %s%s", label, valueStr, unit);
    
    clearLine(row);
    printAt(0, row, buffer);
  }

  // CARACTÈRES PERSONNALISÉS
  // ------------------------------------------
  /**
   * @brief Crée un caractère personnalisé
   * @param location Emplacement (0-7)
   * @param charmap Tableau de 8 bytes définissant le caractère
   */
  void createChar(uint8_t location, uint8_t charmap[]) {
    if (status != LCDStatus::READY) return;
    if (location > 7) return;
    lcd.createChar(location, charmap);
  }

  /**
   * @brief Affiche un caractère personnalisé
   * @param col Colonne (0-19)
   * @param row Ligne (0-3)
   * @param location Emplacement du caractère (0-7)
   */
  void printCustomChar(uint8_t col, uint8_t row, uint8_t location) {
    if (status != LCDStatus::READY) return;
    if (location > 7) return;
    
    lcd.setCursor(col, row);
    lcd.write(location);
  }

  // ACCÈS DIRECT À LA BIBLIOTHÈQUE
  // ------------------------------------------
  /**
   * @brief Retourne une référence à l'objet LiquidCrystal_I2C
   * @return Référence à l'objet LCD
   * 
   * @details Permet d'accéder aux fonctions avancées de la bibliothèque
   */
  LiquidCrystal_I2C& getLCD() {
    return lcd;
  }
};

#endif // LCD_DISPLAY_H
