/**
 * @file config.h
 * @brief Configuration générale du système VOBC
 * @version 0.1.0
 * @date $(date +%Y-%m-%d)
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// INFORMATIONS SYSTÈME
// ============================================
#define VOBC_VERSION "0.1.0"
#define VOBC_BUILD_DATE __DATE__
#define VOBC_BUILD_TIME __TIME__

// ============================================
// CONFIGURATION MATÉRIELLE - PINS
// ============================================

// Bus I2C (pins fixes sur MEGA)
#define I2C_SDA 20
#define I2C_SCL 21

// Capteurs analogiques
#define PIN_MQ7_CO A0        // Détecteur CO (MQ-7)
#define PIN_MQ2_GAS A1       // Détecteur gaz GPL (MQ-2)

// Capteurs digitaux
#define PIN_DS18B20 12       // Température extérieure (One-Wire)
#define PIN_REED_DOOR 2      // Reed switch porte (avec interruption)
#define PIN_PIR_MOTION 7     // PIR détecteur mouvement (Phase 2)

// Capteurs ultrasons (Phase 2)
#define PIN_US1_TRIG 8       // Ultrason 1 - Trigger (eau propre)
#define PIN_US1_ECHO 9       // Ultrason 1 - Echo
#define PIN_US2_TRIG 10      // Ultrason 2 - Trigger (eaux grises)
#define PIN_US2_ECHO 11      // Ultrason 2 - Echo

// Actionneurs (alertes)
#define PIN_BUZZER 3         // Buzzer piézo (PWM)
#define PIN_LED_R 4          // LED RGB - Rouge (PWM)
#define PIN_LED_G 5          // LED RGB - Verte (PWM)
#define PIN_LED_B 6          // LED RGB - Bleue (PWM)

// Encodeur rotatif
#define PIN_ENCODER_CLK 18   // CLK (avec interruption)
#define PIN_ENCODER_DT 19    // DT (avec interruption)
#define PIN_ENCODER_SW 17    // SW - Bouton poussoir

// Module relais (Phase 2)
#define PIN_RELAY_1 22       // Relais 1 - Ventilateur 1
#define PIN_RELAY_2 23       // Relais 2 - Ventilateur 2
#define PIN_RELAY_3 24       // Relais 3 - Éclairage auto
#define PIN_RELAY_4 25       // Relais 4 - Chauffage/Douche

// Bus SPI - Module SD (Phase 3)
#define PIN_SD_CS 53         // Chip Select SD
// MOSI: 51, MISO: 50, SCK: 52 (pins fixes MEGA)

// ============================================
// ADRESSES I2C
// ============================================
#define I2C_ADDR_BME280    0x76  // ou 0x77 selon module
#define I2C_ADDR_INA226_BAT 0x40 // Batterie principale
#define I2C_ADDR_INA226_SOL 0x41 // Panneaux solaires (Phase 2)
#define I2C_ADDR_MPU6050   0x68
#define I2C_ADDR_HMC5883L  0x1E  // Boussole (Phase 3)
#define I2C_ADDR_RTC       0x68  // RTC DS3231 (Phase 3) - conflit avec MPU6050!
#define I2C_ADDR_LCD       0x27  // ou 0x3F selon module

// ============================================
// CONFIGURATION CAPTEURS
// ============================================

// BME280
#define BME280_ENABLED true
#define BME280_SAMPLE_INTERVAL 5000  // ms (5 secondes)

// INA226
#define INA226_ENABLED true
#define INA226_SAMPLE_INTERVAL 1000   // ms (1 seconde)
#define INA226_SHUNT_RESISTOR 0.1     // Ohms (100 mΩ)
#define INA226_MAX_CURRENT 20.0       // Ampères

// MPU6050
#define MPU6050_ENABLED true
#define MPU6050_SAMPLE_INTERVAL 100   // ms (10 Hz)
#define MPU6050_CALIBRATION_SAMPLES 200

// MQ-7 (CO)
#define MQ7_ENABLED true
#define MQ7_SAMPLE_INTERVAL 1000      // ms (1 seconde)
#define MQ7_WARMUP_TIME 180000        // ms (3 minutes minimum)
#define MQ7_THRESHOLD_WARNING 400     // Valeur analogique
#define MQ7_THRESHOLD_CRITICAL 600    // Valeur analogique

// MQ-2 (Gaz GPL)
#define MQ2_ENABLED true
#define MQ2_SAMPLE_INTERVAL 1000      // ms (1 seconde)
#define MQ2_WARMUP_TIME 60000         // ms (1 minute)
#define MQ2_THRESHOLD_WARNING 400     // Valeur analogique
#define MQ2_THRESHOLD_CRITICAL 700    // Valeur analogique

// DS18B20
#define DS18B20_ENABLED true
#define DS18B20_SAMPLE_INTERVAL 5000  // ms (5 secondes)
#define DS18B20_RESOLUTION 12         // bits (9-12)

// Reed Switch
#define REED_ENABLED true
#define REED_DEBOUNCE_TIME 50         // ms

// ============================================
// SEUILS D'ALERTES
// ============================================

// Température
#define TEMP_INT_MIN_WARNING 10.0     // °C
#define TEMP_INT_MIN_CRITICAL 5.0     // °C
#define TEMP_INT_MAX_WARNING 30.0     // °C
#define TEMP_INT_MAX_CRITICAL 35.0    // °C

// Humidité
#define HUMIDITY_MAX_WARNING 70.0     // %
#define HUMIDITY_MAX_CRITICAL 80.0    // %

// Batterie
#define BATTERY_VOLTAGE_MIN_WARNING 12.0   // V
#define BATTERY_VOLTAGE_MIN_CRITICAL 11.5  // V
#define BATTERY_VOLTAGE_MAX_WARNING 14.5   // V (surcharge)

// Horizontalité
#define TILT_MAX_WARNING 3.0          // degrés
#define TILT_MAX_CRITICAL 5.0         // degrés

// ============================================
// CONFIGURATION AFFICHAGE
// ============================================
#define LCD_ENABLED true
#define LCD_COLS 20
#define LCD_ROWS 4
#define LCD_UPDATE_INTERVAL 2000      // ms (2 secondes)
#define LCD_BACKLIGHT_TIMEOUT 300000  // ms (5 minutes)

// ============================================
// CONFIGURATION ALERTES
// ============================================
#define BUZZER_ENABLED true
#define BUZZER_FREQ_WARNING 2000      // Hz
#define BUZZER_FREQ_CRITICAL 3000     // Hz
#define BUZZER_DURATION 200           // ms

#define LED_BRIGHTNESS_NORMAL 128     // 0-255
#define LED_BRIGHTNESS_ALERT 255      // 0-255
#define LED_BLINK_INTERVAL 500        // ms

// ============================================
// CONFIGURATION SYSTÈME
// ============================================
#define SERIAL_BAUD_RATE 115200
#define DEBUG_MODE true               // Afficher logs série
#define WATCHDOG_ENABLED false        // Watchdog timer (à activer en prod)

// Mode nuit (alertes réduites)
#define NIGHT_MODE_ENABLED false
#define NIGHT_MODE_START_HOUR 22      // 22h00
#define NIGHT_MODE_END_HOUR 7         // 07h00

// Logging SD (Phase 3)
#define SD_LOGGING_ENABLED false
#define SD_LOG_INTERVAL 30000         // ms (30 secondes)

// ============================================
// CONSTANTES PHYSIQUES
// ============================================
#define GRAVITY 9.81                  // m/s²
#define DEG_TO_RAD 0.0174533
#define RAD_TO_DEG 57.2957795

#endif // CONFIG_H
