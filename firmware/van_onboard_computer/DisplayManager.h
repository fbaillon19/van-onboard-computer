/**
 * @file DisplayManager.h
 * @brief Gestionnaire de l'écran LCD 20x4 et navigation
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-26
 * 
 * @details
 * Classe responsable de :
 * - Affichage de tous les écrans (HOME, ENVIRONMENT, ENERGY, SAFETY, LEVEL, SETTINGS)
 * - Navigation avec encodeur rotatif
 * - Gestion du rétro-éclairage
 * - Affichage des alertes
 * - Écran de pré-chauffage
 * 
 * Écrans disponibles :
 * - HOME : Températures + Tensions + Horizontalité (écran principal)
 * - ENVIRONMENT : Détails environnement (temp/humidité/pression/point de rosée)
 * - ENERGY : Détails énergie (tensions/courants/puissances)
 * - SAFETY : Détails sécurité (CO/GPL/fumée en temps réel)
 * - LEVEL : Détails horizontalité (Roll/Pitch détaillés)
 * - SETTINGS : Paramètres et calibration
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "SystemData.h"
#include "LCDDisplay.h"
#include "KY040Encoder.h"

// ============================================
// CLASSE DisplayManager
// ============================================
/**
 * @class DisplayManager
 * @brief Gestionnaire de l'écran LCD et navigation
 */
class DisplayManager {
private:
  // LCD et encodeur
  LCDDisplay* lcd;
  KY040Encoder* encoder;
  
  // Référence à l'état système
  SystemState& state;
  
  // Timing
  unsigned long lastUpdate;
  unsigned long lastEncoderActivity;
  
  // Flags
  bool initialized;
  bool forceRedraw;
  
public:
  /**
   * @brief Constructeur
   * @param sysState Référence à l'état système
   */
  DisplayManager(SystemState& sysState) 
    : lcd(nullptr),
      encoder(nullptr),
      state(sysState),
      lastUpdate(0),
      lastEncoderActivity(0),
      initialized(false),
      forceRedraw(true)
  {
  }
  
  /**
   * @brief Destructeur
   */
  ~DisplayManager() {
    if (lcd) delete lcd;
    if (encoder) delete encoder;
  }
  
  // ============================================
  // INITIALISATION
  // ============================================
  
  /**
   * @brief Initialise le LCD et l'encodeur
   * @return true si succès
   */
  bool begin() {
    DEBUG_PRINTLN(F("=== INITIALISATION AFFICHAGE ==="));
    
    // Initialiser LCD
    lcd = new LCDDisplay(I2C_LCD);
    if (lcd->begin()) {
      state.sensors.lcd = true;
      DEBUG_PRINTLN(F("[OK] LCD initialise"));
      
      // Afficher écran de démarrage
      showBootScreen();
    } else {
      DEBUG_PRINTLN(F("[ECHEC] LCD non detecte"));
      state.sensors.lcd = false;
      return false;
    }
    
    // Initialiser encodeur
    encoder = new KY040Encoder(PIN_ENCODER_CLK, PIN_ENCODER_DT, PIN_ENCODER_SW);
    if (encoder->begin()) {
      state.sensors.encoder = true;
      DEBUG_PRINTLN(F("[OK] Encodeur initialise"));
    } else {
      DEBUG_PRINTLN(F("[ECHEC] Encodeur non initialise"));
      state.sensors.encoder = false;
    }
    
    // Créer caractères personnalisés
    lcd->createChar(0, (uint8_t*)CHAR_DEGREE);
    lcd->createChar(1, (uint8_t*)CHAR_ALERT);
    lcd->createChar(2, (uint8_t*)CHAR_BATTERY);
    
    initialized = true;
    lastEncoderActivity = millis();
    
    return true;
  }
  
  /**
   * @brief Affiche l'écran de démarrage
   */
  void showBootScreen() {
    if (!state.sensors.lcd) return;
    
    lcd->clear();
    lcd->printCenter("VAN COMPUTER", 0);
    lcd->printCenter("v" FIRMWARE_VERSION, 1);
    lcd->printCenter("Initialisation...", 3);
  }
  
  // ============================================
  // MISE À JOUR
  // ============================================
  
  /**
   * @brief Met à jour l'affichage et gère la navigation
   * 
   * @details
   * À appeler dans loop().
   * Gère l'encodeur et rafraîchit l'écran.
   */
  void update() {
    if (!initialized) return;
    
    unsigned long now = millis();
    
    // Mettre à jour encodeur
    if (state.sensors.encoder) {
      encoder->update();
      handleEncoder();
    }
    
    // Gérer timeout rétro-éclairage
    handleBacklightTimeout();
    
    // Rafraîchir écran selon intervalle
    if (now - lastUpdate >= INTERVAL_DISPLAY || forceRedraw) {
      lastUpdate = now;
      refreshScreen();
      forceRedraw = false;
    }
  }
  
  /**
   * @brief Gère les événements de l'encodeur
   */
  void handleEncoder() {
    // Rotation détectée
    if (encoder->hasRotated()) {
      lastEncoderActivity = millis();
      state.lastEncoderActivity = lastEncoderActivity;
      
      // Activer rétro-éclairage si éteint
      if (!state.backlightOn) {
        lcd->backlightOn();
        state.backlightOn = true;
        return; // Premier clic juste rallume
      }
      
      // Pas de navigation si alerte bloquante
      if (state.alerts.blockNavigation) {
        return;
      }
      
      // Changer d'écran selon direction
      RotationDirection dir = encoder->getDirection();
      if (dir == RotationDirection::CLOCKWISE) {
        nextScreen();
      } else if (dir == RotationDirection::COUNTER_CLOCKWISE) {
        previousScreen();
      }
      
      forceRedraw = true;
    }
    
    // Bouton pressé
    ButtonEvent event = encoder->getButtonEvent();
    if (event != ButtonEvent::NONE) {
      lastEncoderActivity = millis();
      state.lastEncoderActivity = lastEncoderActivity;
      
      handleButtonEvent(event);
      forceRedraw = true;
    }
  }
  
  /**
   * @brief Gère les événements bouton
   * @param event Type d'événement
   */
  void handleButtonEvent(ButtonEvent event) {
    switch (event) {
      case ButtonEvent::CLICKED:
        // Clic court : action contextuelle
        handleButtonClick();
        break;
        
      case ButtonEvent::LONG_PRESS:
        // Appui long : menu paramètres
        if (state.mode == SystemMode::MODE_SETTINGS) {
          state.mode = SystemMode::MODE_NORMAL;
          state.currentScreen = Screen::SCREEN_HOME;
        } else if (!state.alerts.blockNavigation) {
          state.mode = SystemMode::MODE_SETTINGS;
          state.currentScreen = Screen::SCREEN_SETTINGS;
        }
        break;
        
      default:
        break;
    }
  }
  
  /**
   * @brief Gère le clic court selon l'écran actuel
   */
  void handleButtonClick() {
    switch (state.currentScreen) {
      case Screen::SCREEN_SETTINGS:
        // Lancer calibration MPU6050
        state.calibrationMode = true;
        break;
        
      case Screen::SCREEN_SAFETY:
        // Acquitter alertes (silence buzzer temporaire)
        // À implémenter si besoin
        break;
        
      default:
        // Aucune action par défaut
        break;
    }
  }
  
  /**
   * @brief Passe à l'écran suivant
   */
  void nextScreen() {
    switch (state.currentScreen) {
      case Screen::SCREEN_HOME:
        state.currentScreen = Screen::SCREEN_ENVIRONMENT;
        break;
      case Screen::SCREEN_ENVIRONMENT:
        state.currentScreen = Screen::SCREEN_ENERGY;
        break;
      case Screen::SCREEN_ENERGY:
        state.currentScreen = Screen::SCREEN_SAFETY;
        break;
      case Screen::SCREEN_SAFETY:
        state.currentScreen = Screen::SCREEN_LEVEL;
        break;
      case Screen::SCREEN_LEVEL:
        state.currentScreen = Screen::SCREEN_HOME;
        break;
      default:
        state.currentScreen = Screen::SCREEN_HOME;
        break;
    }
  }
  
  /**
   * @brief Passe à l'écran précédent
   */
  void previousScreen() {
    switch (state.currentScreen) {
      case Screen::SCREEN_HOME:
        state.currentScreen = Screen::SCREEN_LEVEL;
        break;
      case Screen::SCREEN_LEVEL:
        state.currentScreen = Screen::SCREEN_SAFETY;
        break;
      case Screen::SCREEN_SAFETY:
        state.currentScreen = Screen::SCREEN_ENERGY;
        break;
      case Screen::SCREEN_ENERGY:
        state.currentScreen = Screen::SCREEN_ENVIRONMENT;
        break;
      case Screen::SCREEN_ENVIRONMENT:
        state.currentScreen = Screen::SCREEN_HOME;
        break;
      default:
        state.currentScreen = Screen::SCREEN_HOME;
        break;
    }
  }
  
  /**
   * @brief Gère le timeout du rétro-éclairage
   */
  void handleBacklightTimeout() {
    if (LCD_BACKLIGHT_TIMEOUT == 0) return; // Désactivé
    
    unsigned long now = millis();
    unsigned long idle = now - lastEncoderActivity;
    
    if (idle >= LCD_BACKLIGHT_TIMEOUT && state.backlightOn) {
      lcd->backlightOff();
      state.backlightOn = false;
    }
  }
  
  /**
   * @brief Gère le retour automatique à l'écran HOME
   */
  void handleScreenTimeout() {
    unsigned long now = millis();
    unsigned long idle = now - lastEncoderActivity;
    
    // Retour HOME après 5 min d'inactivité
    if (idle >= ENCODER_TIMEOUT && state.currentScreen != Screen::SCREEN_HOME) {
      state.currentScreen = Screen::SCREEN_HOME;
      forceRedraw = true;
    }
  }
  
  // ============================================
  // AFFICHAGE ÉCRANS
  // ============================================
  
  /**
   * @brief Rafraîchit l'écran actuel
   */
  void refreshScreen() {
    if (!state.sensors.lcd) return;
    
    // Mode pré-chauffage
    if (state.mode == SystemMode::MODE_PREHEAT) {
      showPreheatScreen();
      return;
    }
    
    // Mode alerte bloquante
    if (state.alerts.blockNavigation) {
      showAlertScreen();
      return;
    }
    
    // Afficher l'écran courant
    switch (state.currentScreen) {
      case Screen::SCREEN_HOME:
        showHomeScreen();
        break;
      case Screen::SCREEN_ENVIRONMENT:
        showEnvironmentScreen();
        break;
      case Screen::SCREEN_ENERGY:
        showEnergyScreen();
        break;
      case Screen::SCREEN_SAFETY:
        showSafetyScreen();
        break;
      case Screen::SCREEN_LEVEL:
        showLevelScreen();
        break;
      case Screen::SCREEN_SETTINGS:
        showSettingsScreen();
        break;
      default:
        showHomeScreen();
        break;
    }
  }
  
  /**
   * @brief Affiche l'écran HOME
   * 
   * Format :
   * ┌────────────────────┐
   * │INT:23° - EXT:15°   │  Ligne 0 : Températures
   * │65% - P: 1013       │  Ligne 1 : Humidité + Pression
   * │ X:+2° Y:-1°        │  Ligne 2 : Horizontalité
   * │12.3V-5.0V ████     │  Ligne 3 : Tensions + Barre
   * └────────────────────┘
   */
  void showHomeScreen() {
    lcd->clear();
    
    char buffer[21];
    
    // Ligne 0 : Températures
    snprintf(buffer, sizeof(buffer), "INT:%d%c - EXT:%d%c",
             (int)state.environment.tempInterior, (char)0xDF,
             (int)state.environment.tempExterior, (char)0xDF);
    lcd->printAt(0, 0, buffer);
    
    // Ligne 1 : Humidité + Pression
    snprintf(buffer, sizeof(buffer), "%d%% - P:%d",
             (int)state.environment.humidity,
             (int)state.environment.pressure);
    lcd->printAt(0, 1, buffer);
    
    // Ligne 2 : Horizontalité
    snprintf(buffer, sizeof(buffer), " X:%+d%c Y:%+d%c",
             (int)state.level.roll, (char)0xDF,
             (int)state.level.pitch, (char)0xDF);
    lcd->printAt(0, 2, buffer);
    
    // Ligne 3 : Tensions + Barre puissance
    snprintf(buffer, sizeof(buffer), "%.1fV-%.1fV ",
             state.power.voltage12V,
             state.power.voltage5V);
    lcd->printAt(0, 3, buffer);
    
    // Barre puissance (4 caractères)
    drawPowerBar(11, 3, 4);
    
    // Icône alerte si WARNING/INFO
    if (state.alerts.currentLevel == AlertLevel::WARNING ||
        state.alerts.currentLevel == AlertLevel::INFO) {
      lcd->printCustomChar(19, 0, 1); // Icône alerte
    }
  }
  
  /**
   * @brief Affiche l'écran ENVIRONMENT
   * 
   * Format :
   * ┌────────────────────┐
   * │=== ENVIRONNEMENT ===│
   * │Int:23.5° Ext:15.2° │
   * │Humid: 65%          │
   * │Press: 1013 hPa     │
   * └────────────────────┘
   */
  void showEnvironmentScreen() {
    lcd->clear();
    
    char buffer[21];
    
    // Titre
    lcd->printCenter("ENVIRONNEMENT", 0);
    
    // Températures
    snprintf(buffer, sizeof(buffer), "Int:%.1f%c Ext:%.1f%c",
             state.environment.tempInterior, (char)0xDF,
             state.environment.tempExterior, (char)0xDF);
    lcd->printAt(0, 1, buffer);
    
    // Humidité
    snprintf(buffer, sizeof(buffer), "Humid: %d%%",
             (int)state.environment.humidity);
    lcd->printAt(0, 2, buffer);
    
    // Pression
    snprintf(buffer, sizeof(buffer), "Press: %d hPa",
             (int)state.environment.pressure);
    lcd->printAt(0, 3, buffer);
  }
  
  /**
   * @brief Affiche l'écran ENERGY
   * 
   * Format :
   * ┌────────────────────┐
   * │===== ENERGIE ======│
   * │12V: 12.3V - 5.2A   │
   * │ 5V:  5.0V - 1.8A   │
   * │Total: 68.5 W       │
   * └────────────────────┘
   */
  void showEnergyScreen() {
    lcd->clear();
    
    char buffer[21];
    
    // Titre
    lcd->printCenter("ENERGIE", 0);
    
    // Rail 12V
    snprintf(buffer, sizeof(buffer), "12V: %.1fV - %.1fA",
             state.power.voltage12V,
             state.power.current12V);
    lcd->printAt(0, 1, buffer);
    
    // Rail 5V
    snprintf(buffer, sizeof(buffer), " 5V: %.1fV - %.1fA",
             state.power.voltage5V,
             state.power.current5V);
    lcd->printAt(0, 2, buffer);
    
    // Puissance totale
    snprintf(buffer, sizeof(buffer), "Total: %.1f W",
             state.power.powerTotal);
    lcd->printAt(0, 3, buffer);
  }
  
  /**
   * @brief Affiche l'écran SAFETY
   * 
   * Format :
   * ┌────────────────────┐
   * │===== SECURITE =====│
   * │CO:    50 ppm  [OK] │
   * │GPL:  150 ppm  [OK] │
   * │Fumee: 80 ppm  [OK] │
   * └────────────────────┘
   */
  void showSafetyScreen() {
    lcd->clear();
    
    char buffer[21];
    
    // Titre
    lcd->printCenter("SECURITE", 0);
    
    // CO
    const char* coStatus = getGasStatus(state.safety.coPPM, 
                                        CO_THRESHOLD_WARNING,
                                        CO_THRESHOLD_DANGER);
    snprintf(buffer, sizeof(buffer), "CO:  %4d ppm %s",
             (int)state.safety.coPPM, coStatus);
    lcd->printAt(0, 1, buffer);
    
    // GPL
    const char* gplStatus = getGasStatus(state.safety.gplPPM,
                                         GPL_THRESHOLD_WARNING,
                                         GPL_THRESHOLD_DANGER);
    snprintf(buffer, sizeof(buffer), "GPL: %4d ppm %s",
             (int)state.safety.gplPPM, gplStatus);
    lcd->printAt(0, 2, buffer);
    
    // Fumée
    const char* smokeStatus = getGasStatus(state.safety.smokePPM,
                                           SMOKE_THRESHOLD_WARNING,
                                           SMOKE_THRESHOLD_DANGER);
    snprintf(buffer, sizeof(buffer), "Fum: %4d ppm %s",
             (int)state.safety.smokePPM, smokeStatus);
    lcd->printAt(0, 3, buffer);
  }
  
  /**
   * @brief Affiche l'écran LEVEL
   * 
   * Format :
   * ┌────────────────────┐
   * │=== HORIZONTALITE ===│
   * │Roll:  +2.5°        │
   * │Pitch: -1.3°        │
   * │Total:  2.8°        │
   * └────────────────────┘
   */
  void showLevelScreen() {
    lcd->clear();
    
    char buffer[21];
    
    // Titre
    lcd->printCenter("HORIZONTALITE", 0);
    
    // Roll
    snprintf(buffer, sizeof(buffer), "Roll:  %+.1f%c",
             state.level.roll, (char)0xDF);
    lcd->printAt(0, 1, buffer);
    
    // Pitch
    snprintf(buffer, sizeof(buffer), "Pitch: %+.1f%c",
             state.level.pitch, (char)0xDF);
    lcd->printAt(0, 2, buffer);
    
    // Inclinaison totale
    snprintf(buffer, sizeof(buffer), "Total: %.1f%c",
             state.level.totalTilt, (char)0xDF);
    lcd->printAt(0, 3, buffer);
  }
  
  /**
   * @brief Affiche l'écran SETTINGS
   * 
   * Format :
   * ┌────────────────────┐
   * │===== PARAMETRES ====│
   * │MPU6050:            │
   * │ [Clic:Calibration] │
   * │ [Long:Quitter]     │
   * └────────────────────┘
   */
  void showSettingsScreen() {
    lcd->clear();
    
    // Titre
    lcd->printCenter("PARAMETRES", 0);
    
    // Options
    lcd->printAt(0, 1, "MPU6050:");
    lcd->printAt(1, 2, "[Clic:Calibration]");
    lcd->printAt(1, 3, "[Long:Quitter]");
    
    // Afficher état calibration
    if (state.level.calibrated) {
      lcd->printAt(8, 1, "CAL OK");
    } else {
      lcd->printAt(8, 1, "NON CAL");
    }
  }
  
  /**
   * @brief Affiche l'écran de pré-chauffage
   * 
   * Format :
   * ┌────────────────────┐
   * │  PRE-CHAUFFE GAZ   │
   * │                    │
   * │ [████████░░░░░░]   │
   * │   2min 30s         │
   * └────────────────────┘
   */
  void showPreheatScreen() {
    lcd->clear();
    
    char buffer[21];
    
    // Titre
    lcd->printCenter("PRE-CHAUFFE GAZ", 0);
    
    // Calculer temps restant
    unsigned long elapsed = millis() - state.preheatStartTime;
    unsigned long maxTime = max(PREHEAT_MQ7_TIME, PREHEAT_MQ2_TIME);
    unsigned long remaining = 0;
    
    if (elapsed < maxTime) {
      remaining = maxTime - elapsed;
    }
    
    // Barre de progression
    uint8_t percent = (elapsed * 100) / maxTime;
    if (percent > 100) percent = 100;
    
    lcd->setCursor(1, 2);
    lcd->getLCD().print('[');
    uint8_t filled = (14 * percent) / 100;
    for (uint8_t i = 0; i < 14; i++) {
      if (i < filled) {
        lcd->getLCD().write(0xFF);
      } else {
        lcd->getLCD().write('.');
      }
    }
    lcd->getLCD().print(']');
    
    // Temps restant
    uint16_t remainingSec = remaining / 1000;
    uint8_t minutes = remainingSec / 60;
    uint8_t seconds = remainingSec % 60;
    snprintf(buffer, sizeof(buffer), "   %dmin %02ds", minutes, seconds);
    lcd->printAt(0, 3, buffer);
  }
  
  /**
   * @brief Affiche l'écran d'alerte
   * 
   * Format :
   * ┌────────────────────┐
   * │!!! ALERTE !!!      │
   * │CO CRITIQUE!        │
   * │450 ppm             │
   * │EVACUEZ MAINTENANT! │
   * └────────────────────┘
   */
  void showAlertScreen() {
    lcd->clear();
    
    char buffer[21];
    
    // Titre avec niveau
    snprintf(buffer, sizeof(buffer), "!!! %s !!!",
             alertLevelToString(state.alerts.currentLevel));
    lcd->printCenter(buffer, 0);
    
    // Message de l'alerte principale
    if (state.alerts.activeAlertCount > 0) {
      Alert alert = state.alerts.alerts[0];
      
      // Message
      lcd->printCenter(alert.message, 1);
      
      // Valeur
      snprintf(buffer, sizeof(buffer), "%.0f", alert.value);
      lcd->printCenter(buffer, 2);
      
      // Action selon niveau
      if (alert.level == AlertLevel::CRITICAL) {
        lcd->printCenter("EVACUEZ!", 3);
      } else if (alert.level == AlertLevel::DANGER) {
        lcd->printCenter("ATTENTION!", 3);
      }
    }
  }
  
  // ============================================
  // FONCTIONS UTILITAIRES AFFICHAGE
  // ============================================
  
  /**
   * @brief Dessine une barre de puissance
   * @param col Colonne de départ
   * @param row Ligne
   * @param width Largeur en caractères
   */
  void drawPowerBar(uint8_t col, uint8_t row, uint8_t width) {
    // Calculer pourcentage charge
    const float MAX_POWER = 200.0;
    float percent = (state.power.powerTotal / MAX_POWER) * 100.0;
    percent = constrain(percent, 0.0, 100.0);
    
    uint8_t filled = (width * percent) / 100;
    
    lcd->setCursor(col, row);
    for (uint8_t i = 0; i < width; i++) {
      if (i < filled) {
        lcd->getLCD().write(0xFF); // Bloc plein
      } else {
        lcd->getLCD().write('.');  // Vide
      }
    }
  }
  
  /**
   * @brief Retourne le statut textuel d'un gaz
   * @param ppm Concentration
   * @param warning Seuil WARNING
   * @param danger Seuil DANGER
   * @return Statut "[OK]" / "[!]" / "[X]"
   */
  const char* getGasStatus(float ppm, float warning, float danger) {
    if (ppm >= danger) return "[X]";      // Danger
    if (ppm >= warning) return "[!]";     // Warning
    return "[OK]";                         // OK
  }
  
  // ============================================
  // CONTRÔLE AFFICHAGE
  // ============================================
  
  /**
   * @brief Force le rafraîchissement immédiat
   */
  void forceRefresh() {
    forceRedraw = true;
  }
  
  /**
   * @brief Change d'écran manuellement
   * @param screen Nouvel écran
   */
  void setScreen(Screen screen) {
    state.currentScreen = screen;
    forceRedraw = true;
    lastEncoderActivity = millis();
  }
  
  /**
   * @brief Active/désactive le rétro-éclairage
   * @param on true pour allumer
   */
  void setBacklight(bool on) {
    if (!state.sensors.lcd) return;
    
    if (on) {
      lcd->backlightOn();
    } else {
      lcd->backlightOff();
    }
    state.backlightOn = on;
  }
  
  /**
   * @brief Affiche un message temporaire
   * @param message Message à afficher
   * @param duration Durée d'affichage (ms)
   */
  void showMessage(const char* message, uint16_t duration = 2000) {
    if (!state.sensors.lcd) return;
    
    lcd->clear();
    lcd->printCenter(message, 1);
    delay(duration);
    forceRedraw = true;
  }
  
  // ============================================
  // GETTERS
  // ============================================
  
  /**
   * @brief Vérifie si le display est initialisé
   * @return true si initialisé
   */
  bool isInitialized() const {
    return initialized;
  }
  
  /**
   * @brief Obtient l'écran actuel
   * @return Écran courant
   */
  Screen getCurrentScreen() const {
    return state.currentScreen;
  }
  
  /**
   * @brief Vérifie si le rétro-éclairage est allumé
   * @return true si allumé
   */
  bool isBacklightOn() const {
    return state.backlightOn;
  }
};

#endif // DISPLAY_MANAGER_H
