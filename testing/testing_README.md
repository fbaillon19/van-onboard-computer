# Guide de test VOBC

## 📋 Présentation

Ce dossier contient tous les codes de test unitaires pour valider chaque capteur et composant individuellement avant l'intégration dans le système complet.

## 🎯 Objectif

Tester chaque composant séparément permet de :
- Vérifier le bon fonctionnement matériel
- Valider le câblage
- Comprendre le comportement de chaque capteur
- Identifier les problèmes rapidement
- Calibrer les capteurs si nécessaire

## 📁 Structure

```
testing/
├── test_codes/
│   ├── test_arduino/          # Test basique Arduino
│   ├── test_button/           # Test encodeur rotatif
│   ├── test_bme280/           # Test température/humidité
│   ├── test_mpu6050/          # Test gyroscope/accéléromètre
│   ├── test_ina226/           # Test mesure courant/tension
│   ├── test_mq7_mq2/          # Test détecteurs gaz
│   ├── test_ds18b20/          # Test température extérieure
│   ├── test_lcd/              # Test affichage LCD
│   ├── test_reed_switch/      # Test détection porte
│   ├── test_buzzer/           # Test alarme sonore
│   ├── test_led_rgb/          # Test LED RGB
│   └── test_encoder/          # Test encodeur rotatif
└── testing_README.md          # Ce fichier
```

## 🔧 Procédure de test recommandée

### 1. Test Arduino de base
**Fichier:** `test_codes/test_arduino/test_arduino.ino`

Valide que l'Arduino MEGA fonctionne correctement.
- Upload du code
- Vérification LED intégrée
- Test moniteur série

### 2. Test Scanner I2C
**Fichier:** `test_codes/test_i2c_scanner/test_i2c_scanner.ino`

Détecte tous les périphériques I2C connectés.
- Affiche les adresses I2C trouvées
- Permet d'identifier les capteurs

### 3. Tests capteurs individuels

Tester dans cet ordre :

#### a) BME280 (Température/Humidité)
**Fichier:** `test_codes/test_bme280/test_bme280.ino`
- Vérifier température ambiante cohérente
- Vérifier humidité 30-70%
- Tester réactivité (souffler dessus)

#### b) MPU6050 (Horizontalité)
**Fichier:** `test_codes/test_mpu6050/test_mpu6050.ino`
- Calibration obligatoire
- Vérifier angles Roll/Pitch sur surface plane
- Tester inclinaison

#### c) INA226 (Courant/Tension)
**Fichier:** `test_codes/test_ina226/test_ina226.ino`
- Mesure tension batterie
- Test avec charge connue
- Vérifier calcul puissance

#### d) DS18B20 (Température extérieure)
**Fichier:** `test_codes/test_ds18b20/test_ds18b20.ino`
- Vérifier température
- Tester réactivité
- Si étanche: test immersion

#### e) MQ-7 et MQ-2 (Détecteurs gaz)
**Fichier:** `test_codes/test_mq7_mq2/test_mq7_mq2.ino`
- ⚠️ ATTENTION: Préchauffage 24-48h première utilisation
- ⚠️ EXTÉRIEUR ou zone très ventilée
- Noter valeurs de base
- Tester réactivité

#### f) LCD I2C
**Fichier:** `test_codes/test_lcd/test_lcd.ino`
- Test affichage 4 lignes
- Réglage contraste
- Test rétroéclairage
- Test caractères spéciaux

#### g) Reed Switch
**Fichier:** `test_codes/test_reed_switch/test_reed_switch.ino`
- Test détection aimant
- Vérifier debouncing
- Test distance activation

#### h) Buzzer
**Fichier:** `test_codes/test_buzzer/test_buzzer.ino`
- Test différentes fréquences
- Test volume
- Test séquences alarme

#### i) LED RGB
**Fichier:** `test_codes/test_led_rgb/test_led_rgb.ino`
- Test couleurs primaires (R,G,B)
- Test couleurs mixtes (Jaune, Cyan, Magenta, Blanc)
- Test PWM (fade)

#### j) Encodeur rotatif
**Fichier:** `test_codes/test_encoder/test_encoder.ino`
- Test rotation (sens horaire/anti-horaire)
- Test bouton poussoir
- Test interruptions

## ⚠️ Sécurité

### Capteurs de gaz (MQ-7, MQ-2)
- **JAMAIS** tester en intérieur fermé
- Toujours en extérieur ou zone très ventilée
- Préchauffage minimum 24h avant mesures fiables
- Ne pas toucher (150°C en fonctionnement)

### Alimentation
- Vérifier polarités
- Commencer par alimentation USB (5V sécurisé)
- Tester convertisseur 12V→5V avec multimètre AVANT connexion

### Câblage
- Toujours débrancher avant modification
- Vérifier pas de court-circuit
- Respecter tensions (3.3V vs 5V)

## 📊 Tableau de suivi des tests

| Composant | Test OK | Date | Notes |
|-----------|---------|------|-------|
| Arduino MEGA | ☐ | | |
| I2C Scanner | ☐ | | Adresses détectées: |
| BME280 | ☐ | | |
| MPU6050 | ☐ | | Calibré: ☐ |
| INA226 #1 | ☐ | | |
| MQ-7 | ☐ | | Préchauffé: ☐ Valeur base: |
| MQ-2 | ☐ | | Préchauffé: ☐ Valeur base: |
| DS18B20 | ☐ | | |
| LCD I2C | ☐ | | Contraste OK: ☐ |
| Reed Switch | ☐ | | |
| Buzzer | ☐ | | |
| LED RGB | ☐ | | |
| Encodeur | ☐ | | |

## 📝 Rapport de test

Pour chaque test, noter :
- ✅ Succès / ❌ Échec
- Valeurs mesurées (si applicable)
- Problèmes rencontrés
- Solutions apportées

## 🔄 Prochaine étape

Une fois tous les tests unitaires validés :
1. Tester combinaisons de 2-3 capteurs
2. Vérifier pas de conflit I2C
3. Tester système complet
4. Calibration finale
5. Installation dans véhicule

## 💡 Conseils

- Tester un composant à la fois
- Noter toutes les observations
- Prendre photos du câblage fonctionnel
- Conserver les valeurs de calibration
- Documenter les problèmes résolus

---

**Temps estimé:** 4-6 heures pour tous les tests
