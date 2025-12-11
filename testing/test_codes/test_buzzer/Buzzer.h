/**
 * @file Buzzer.h
 * @brief Classe pour la gestion d'un buzzer piézoélectrique
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-21
 * 
 * @details
 * Cette classe gère un buzzer piézoélectrique actif ou passif via PWM.
 * Elle permet de générer des sons de différentes fréquences et durées.
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

// ============================================
// CONFIGURATION MATÉRIELLE
// ============================================
#define BUZZER_PIN 11        ///< Broche GPIO du buzzer (à adapter)

// ============================================
// TYPES ET STRUCTURES
// ============================================
// Définition des notes musicales (fréquences en Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

// ============================================
// DEFINITION CLASSE Buzzer
// ============================================
/**
 * @class Buzzer
 * @brief Gestion d'un buzzer piézoélectrique
 * 
 * Cette classe encapsule les fonctionnalités d'un buzzer :
 * - Génération de tonalités à fréquences variables
 * - Gestion de la durée des sons
 * - Patterns sonores prédéfinis (bip, double-bip, alarme)
 */
class Buzzer {
private:
    uint8_t pin;           ///< Broche GPIO du buzzer
    bool initialized;      ///< État d'initialisation
    uint32_t startTime;    ///< Temps de début du son
    uint32_t duration;     ///< Durée du son en cours

public:
    /**
     * @brief Constructeur de la classe Buzzer
     * @param pin Numéro de la broche GPIO connectée au buzzer
     */
    Buzzer(uint8_t pin) :
        pin(pin),
        initialized(false),
        startTime(0),
        duration(0)
    {}

    // INITIALISATION
    // ------------------------------------------
    /**
     * @brief Initialise le buzzer
     * @return true si l'initialisation réussit, false sinon
     */
    bool begin() {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
        initialized = true;
        
        return true;
    }

    /**
     * @brief Émet un son à une fréquence donnée
     * @param frequency Fréquence en Hz (20 à 20000 Hz typiquement)
     * @param duration Durée en millisecondes (0 = son continu)
     */
    void tone(uint16_t frequency, uint32_t duration = 0) {
        if (!initialized) return;
    
        ::tone(pin, frequency);
    
        if (duration > 0) {
            startTime = millis();
            duration = duration;
        } else {
            duration = 0;
        }
    }


    /**
     * @brief Arrête le son en cours
     */
    void stop() {
        if (!initialized) return;
    
        ::noTone(pin);
        duration = 0;
    }

    /**
     * @brief Émet un bip court
     * @param frequency Fréquence du bip en Hz (défaut: 1000 Hz)
     */
    void beep(uint16_t frequency = 1000) {
        tone(frequency, 100);
        delay(100);
        stop();
    }

    /**
     * @brief Émet un double-bip
     * @param frequency Fréquence des bips en Hz (défaut: 1000 Hz)
     */
    void doubleBeep(uint16_t frequency = 1000) {
        tone(frequency, 100);
        delay(100);
        stop();
        delay(100);
        tone(frequency, 100);
        delay(100);
        stop();
    }

    /**
     * @brief Émet une séquence d'alarme
     * @param duration Durée totale de l'alarme en ms (défaut: 1000 ms)
     */
    void alarm(uint32_t duration = 1000) {
        uint32_t startTime = millis();
    
        while (millis() - startTime < duration) {
            tone(800, 100);
            delay(100);
            stop();
            tone(1200, 100);
            delay(100);
            stop();
        }
    }

    /**
     * @brief Vérifie si un son est en cours d'émission
     * @return true si un son est actif, false sinon
     */
    bool isPlaying() const {
        if (duration == 0) return false;
        
        return (millis() - startTime) < duration;
    }

};

#endif // BUZZER_H
