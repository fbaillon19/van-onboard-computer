/**
 * @file config.h
 * @brief Configuration centralisée du système Van Onboard Computer
 * @author Frédéric BAILLON
 * @version 0.1.0
 * @date 2024-11-26
 * 
 * @details
 * Fichier de configuration centralisé contenant :
 * - Adresses I2C de tous les périphériques
 * - Pins GPIO utilisées
 * - Seuils d'alerte (CO, GPL, tensions, températures)
 * - Intervalles d'acquisition
 * - Paramètres d'affichage
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================
// VERSION FIRMWARE
// ============================================
#define FIRMWARE_VERSION        "1.0.0"
#define FIRMWARE_DATE           "2024-11-26"

// ============================================
// CONFIGURATION MATÉRIELLE - ADRESSES I2C
// ============================================
#define I2C_BME280              0x76    ///< Capteur environnemental intérieur
#define I2C_MPU6050             0x68    ///< Accéléromètre/gyroscope
#define I2C_INA226_12V          0x40    ///< Surveillance rail 12V
#define I2C_INA226_5V           0x41    ///< Surveillance rail 5V
#define I2C_LCD                 0x27    ///< Écran LCD 20x4

// ============================================
// CONFIGURATION MATÉRIELLE - PINS GPIO
// ============================================
// Capteurs analogiques/numériques
#define PIN_MQ7                 A0      ///< Capteur CO (analogique)
#define PIN_MQ2                 A1      ///< Capteur GPL/fumée (analogique)
#define PIN_DS18B20             22      ///< Capteur température extérieure (OneWire)

// Encodeur rotatif KY040
#define PIN_ENCODER_CLK         2       ///< Encodeur CLK (interruption)
#define PIN_ENCODER_DT          3       ///< Encodeur DT (interruption)
#define PIN_ENCODER_SW          4       ///< Encodeur bouton poussoir

// Périphériques de sortie
#define PIN_BUZZER              25      ///< Buzzer piézoélectrique
#define PIN_WS2812B             6       ///< Bandeau LED WS2812B

// LCD déporté (optionnel, non implémenté v1.0)
#define PIN_DASHBOARD_TX        18      ///< UART2 TX vers Arduino Nano
#define PIN_DASHBOARD_RX        19      ///< UART2 RX depuis Arduino Nano

// ============================================
// CONFIGURATION WS2812B
// ============================================
#define LED_COUNT               8       ///< Nombre total de LEDs
#define LED_BRIGHTNESS          76      ///< Luminosité (0-255), 30% = 76

// Répartition des LEDs
#define LED_POWER_START         0       ///< LEDs 0-3 : Barre puissance
#define LED_POWER_COUNT         4
#define LED_CO                  4       ///< LED 4 : Indicateur CO
#define LED_GPL                 5       ///< LED 5 : Indicateur GPL
#define LED_VOLTAGE_12V         6       ///< LED 6 : État 12V
#define LED_VOLTAGE_5V          7       ///< LED 7 : État 5V

// ============================================
// SEUILS ALERTES - GAZ DANGEREUX
// ============================================
// MQ7 - Monoxyde de carbone (CO)
#define CO_THRESHOLD_INFO       50      ///< Info : détection faible (ppm)
#define CO_THRESHOLD_WARNING    200     ///< Warning : attention nécessaire (ppm)
#define CO_THRESHOLD_DANGER     400     ///< Danger : évacuation immédiate (ppm)

// MQ2 - GPL/Méthane
#define GPL_THRESHOLD_INFO      500     ///< Info : détection faible (ppm)
#define GPL_THRESHOLD_WARNING   1000    ///< Warning : attention nécessaire (ppm)
#define GPL_THRESHOLD_DANGER    3000    ///< Danger : évacuation immédiate (ppm)

// MQ2 - Fumée
#define SMOKE_THRESHOLD_INFO    1000    ///< Info : détection faible (ppm)
#define SMOKE_THRESHOLD_WARNING 1500    ///< Warning : attention nécessaire (ppm)
#define SMOKE_THRESHOLD_DANGER  2000    ///< Danger : évacuation immédiate (ppm)

// ============================================
// SEUILS ALERTES - ÉLECTRIQUES
// ============================================
// Rail 12V (batterie)
#define VOLTAGE_12V_MIN         10.5    ///< Minimum absolu (décharge profonde)
#define VOLTAGE_12V_WARNING     11.5    ///< Warning : batterie faible
#define VOLTAGE_12V_NOMINAL     12.0    ///< Nominal : au repos
#define VOLTAGE_12V_CHARGING    13.8    ///< En charge (alternateur)
#define VOLTAGE_12V_MAX         14.5    ///< Maximum admissible

// Rail 5V (électronique)
#define VOLTAGE_5V_MIN          4.5     ///< Minimum admissible
#define VOLTAGE_5V_NOMINAL      5.0     ///< Nominal
#define VOLTAGE_5V_MAX          5.5     ///< Maximum admissible

// Courants maximaux (alertes sur-courant)
#define CURRENT_12V_MAX         20.0    ///< Courant max rail 12V (A)
#define CURRENT_5V_MAX          3.0     ///< Courant max rail 5V (A)

// ============================================
// SEUILS ALERTES - ENVIRONNEMENT
// ============================================
// Température
#define TEMP_MIN                -10     ///< Température minimale (°C)
#define TEMP_COMFORT_MIN        15      ///< Confort minimum (°C)
#define TEMP_COMFORT_MAX        25      ///< Confort maximum (°C)
#define TEMP_WARNING            35      ///< Warning : chaleur excessive (°C)
#define TEMP_MAX                45      ///< Maximum absolu (°C)

// Humidité
#define HUMIDITY_MIN            20      ///< Humidité minimale (%)
#define HUMIDITY_COMFORT_MIN    30      ///< Confort minimum (%)
#define HUMIDITY_COMFORT_MAX    60      ///< Confort maximum (%)
#define HUMIDITY_WARNING        80      ///< Warning : risque condensation (%)
#define HUMIDITY_MAX            95      ///< Maximum (%)

// Horizontalité (MPU6050)
#define TILT_WARNING            5.0     ///< Warning : inclinaison notable (°)
#define TILT_DANGER             15.0    ///< Danger : inclinaison importante (°)

// ============================================
// INTERVALLES D'ACQUISITION (ms)
// ============================================
#define INTERVAL_BME280         10000   ///< 10s - Température/humidité intérieure
#define INTERVAL_DS18B20        10000   ///< 10s - Température extérieure
#define INTERVAL_MPU6050        500     ///< 500ms - Horizontalité (temps réel)
#define INTERVAL_INA226         2000    ///< 2s - Surveillance tensions
#define INTERVAL_MQ7            2000    ///< 2s - Détection CO
#define INTERVAL_MQ2            2000    ///< 2s - Détection GPL/fumée
#define INTERVAL_DISPLAY        100     ///< 100ms - Rafraîchissement LCD
#define INTERVAL_LEDS           50      ///< 50ms - Rafraîchissement LEDs

// ============================================
// TIMING SYSTÈME
// ============================================
#define PREHEAT_MQ7_TIME        180000  ///< 3 min - Pré-chauffe MQ7 (ms)
#define PREHEAT_MQ2_TIME        60000   ///< 1 min - Pré-chauffe MQ2 (ms)
#define ENCODER_TIMEOUT         300000  ///< 5 min - Retour écran accueil (ms)
#define ALERT_BLINK_INTERVAL    500     ///< 500ms - Clignotement LED alerte
#define BUZZER_BEEP_DURATION    100     ///< 100ms - Durée bip court

// ============================================
// CONFIGURATION INA226
// ============================================
#define SHUNT_12V_RESISTANCE    0.002   ///< 2 mΩ - Résistance shunt 12V
#define SHUNT_5V_RESISTANCE     0.010   ///< 10 mΩ - Résistance shunt 5V
#define SHUNT_12V_MAX_CURRENT   40.0    ///< 40A - Courant max mesurable 12V
#define SHUNT_5V_MAX_CURRENT    8.0     ///< 8A - Courant max mesurable 5V

// ============================================
// CONFIGURATION ÉCRAN LCD
// ============================================
#define LCD_COLS                20      ///< 20 colonnes
#define LCD_ROWS                4       ///< 4 lignes
#define LCD_BACKLIGHT_TIMEOUT   600000  ///< 10 min - Extinction auto (0=désactivé)

// ============================================
// CONFIGURATION MPU6050
// ============================================
#define MPU6050_CALIBRATION_SAMPLES  100  ///< Échantillons pour calibration

// ============================================
// FONCTIONNALITÉS OPTIONNELLES
// ============================================
#define USE_DASHBOARD_LCD       false   ///< LCD déporté (non implémenté v1.0)
#define USE_SERIAL_DEBUG        true    ///< Sortie debug sur Serial
#define SERIAL_BAUD_RATE        115200  ///< Vitesse Serial

// ============================================
// MACROS UTILITAIRES
// ============================================
// Macros pour debug conditionnel
#if USE_SERIAL_DEBUG
  #define DEBUG_PRINT(x)        Serial.print(x)
  #define DEBUG_PRINTLN(x)      Serial.println(x)
  // Fonction helper pour printf-like sur Arduino
  inline void DEBUG_PRINTF(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.print(buffer);
  }
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// Macros de conversion
#define PPM_TO_PERCENT(ppm, max)  ((ppm * 100.0) / max)
#define PERCENT_TO_PPM(pct, max)  ((pct * max) / 100.0)

// Macros de limitation
#define CONSTRAIN_FLOAT(x, min, max)  ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

// ============================================
// CARACTÈRES PERSONNALISÉS LCD
// ============================================
// Symbole degré (°)
const uint8_t CHAR_DEGREE[8] = {
  0b00110,
  0b01001,
  0b01001,
  0b00110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

// Symbole alerte (!)
const uint8_t CHAR_ALERT[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000,
  0b00100,
  0b00000
};

// Symbole batterie
const uint8_t CHAR_BATTERY[8] = {
  0b01110,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b00000
};

// ============================================
// NOTES IMPORTANTES
// ============================================
/*
 * PRIORITÉ SÉCURITÉ :
 * - Les alertes CO et GPL sont prioritaires sur tout le reste
 * - Le système d'alerte bloque la navigation en cas de danger
 * - Le buzzer est continu pour les alertes CRITICAL
 * 
 * INTERVALLES :
 * - Les capteurs de gaz (MQ7/MQ2) nécessitent un pré-chauffage
 * - Les intervalles sont optimisés pour la réactivité et l'économie d'énergie
 * 
 * PERSONNALISATION :
 * - Tous les seuils peuvent être ajustés selon vos besoins
 * - Les intervalles peuvent être modifiés pour plus/moins de réactivité
 */

#endif // CONFIG_H
