/**
 * @file test_buzzer.ino
 * @brief Programme de test pour le buzzer piézoélectrique
 * @author Frédéric BAILLON
 * @version 1.0.0
 * @date 2024-11-21
 * 
 * @details
 * Ce programme teste les fonctionnalités du buzzer :
 * - Validation de la connexion matérielle
 * - Test de différentes fréquences
 * - Test des patterns sonores (bip, double-bip, alarme)
 * - Test de la gamme musicale
 * 
 * Matériel requis :
 * - ESP32
 * - Buzzer piézoélectrique connecté sur GPIO
 * 
 * Configuration :
 * - Définir BUZZER_PIN selon votre câblage
 */

#include "Buzzer.h"

// ============================================
// CONFIGURATION
// ============================================
#define SERIAL_BAUD 115200   ///< Vitesse de communication série

// ============================================
// VARIABLES GLOBALES
// ============================================
Buzzer buzzer(BUZZER_PIN);

// ============================================
// SETUP
// ============================================
void setup() {
    // Initialisation de la communication série
    Serial.begin(SERIAL_BAUD);
    delay(3000);
    
    Serial.println();
    Serial.println(F("╔════════════════════════════════════════╗"));
    Serial.println(F("║   TEST BUZZER - VOBC                   ║"));
    Serial.println(F("╚════════════════════════════════════════╝"));
    Serial.println();
    
    // Configuration du buzzer
    Serial.print(F("Broche buzzer : GPIO "));
    Serial.println(BUZZER_PIN);
    
    // Initialisation du buzzer
    Serial.print(F("Initialisation du buzzer... "));
    if (buzzer.begin()) {
        Serial.println(F("✓ OK"));
    } else {
        Serial.println(F("✗ ÉCHEC"));
        Serial.println(F("Vérifiez le câblage !"));
        while (1) delay(1000);
    }
    
    // Signal sonore de démarrage
    Serial.println(F("\nSignal de démarrage..."));
    buzzer.beep(1000);
    delay(200);
    buzzer.beep(1500);
    
    // Affichage du menu
    printMenu();
}

// ============================================
// LOOP
// ============================================
void loop() {
    // Attente d'une commande utilisateur
    if (Serial.available()) {
        handleCommand(Serial.read());

        while (Serial.available())  Serial.read();  // Vider le buffer série
        
        // Réaffichage du menu
        delay(500);
        printMenu();
    }
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
            testBasique();
            break;
        case '2':
            testFrequences();
            break;
        case '3':
            testBip();
            break;
        case '4':
            testDoubleBip();
            break;
        case '5':
            testAlarme();
            break;
        case '6':
            arreterSon();
            break;
        default:
            Serial.println(F("Commande inconnue"));
            break;
    }
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
    Serial.println(F("║            COMMANDES                   ║"));
    Serial.println(F("╚════════════════════════════════════════╝"));
    Serial.println();
    Serial.println(F("1. Test de base (1000 Hz)"));
    Serial.println(F("2. Test fréquences variables"));
    Serial.println(F("3. Test bip simple"));
    Serial.println(F("4. Test double-bip"));
    Serial.println(F("5. Test alarme"));
    Serial.println(F("6. Arrêter le son"));
    Serial.println(F("----------------------------------------"));
    Serial.println(F("Tapez le numéro du test à exécuter..."));
    Serial.println();
}

// ============================================
// FONCTIONS DE TEST
// ============================================
/**
 * @brief Test de base avec un son à 1000 Hz
 */
void testBasique() {
    Serial.println();
    Serial.println(F("=== Test 1 : Son basique 1000 Hz ==="));
    Serial.println(F("Émission d'un son à 1000 Hz pendant 2 secondes..."));
    
    buzzer.tone(1000, 2000);
    delay(2000);
    buzzer.stop();
    
    Serial.println(F("✓ Test terminé"));
}

/**
 * @brief Test avec différentes fréquences
 */
void testFrequences() {
    Serial.println();
    Serial.println(F("=== Test 2 : Fréquences variables ==="));
    
    uint16_t frequencies[] = {250, 500, 1000, 2000, 4000};
    const char* labels[] = {"250 Hz (grave)", "500 Hz", "1000 Hz", "2000 Hz", "4000 Hz (aigu)"};
    
    for (int i = 0; i < 5; i++) {
        Serial.print(F("Fréquence : "));
        Serial.println(labels[i]);
        
        buzzer.tone(frequencies[i], 800);
        delay(1000);
        buzzer.stop();
        delay(200);
    }
    
    Serial.println(F("✓ Test terminé"));
}

/**
 * @brief Test du bip simple
 */
void testBip() {
    Serial.println();
    Serial.println(F("=== Test 3 : Bip simple ==="));
    Serial.println(F("Émission d'un bip court..."));
    
    buzzer.beep();
    delay(500);
    
    Serial.println(F("✓ Test terminé"));
}

/**
 * @brief Test du double-bip
 */
void testDoubleBip() {
    Serial.println();
    Serial.println(F("=== Test 4 : Double-bip ==="));
    Serial.println(F("Émission d'un double-bip..."));
    
    buzzer.doubleBeep();
    delay(500);
    
    Serial.println(F("✓ Test terminé"));
}

/**
 * @brief Test de l'alarme
 */
void testAlarme() {
    Serial.println();
    Serial.println(F("=== Test 5 : Alarme ==="));
    Serial.println(F("Émission d'une alarme pendant 3 secondes..."));
    
    buzzer.alarm(3000);
    
    Serial.println(F("✓ Test terminé"));
}


// ============================================
// FONCTIONS UTILITAIRES
// ============================================
/**
 * @brief Arrêter tout son
 */
void arreterSon() {
    Serial.println();
    Serial.println(F("=== Arrêt du son ==="));
    buzzer.stop();
    Serial.println(F("✓ Son arrêté"));
}
