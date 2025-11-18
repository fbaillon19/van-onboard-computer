# 🚐 VOBC - Van Onboard Computer

**V**an **O**nboard **B**oard **C**omputer - Système de monitoring et de gestion pour fourgon aménagé.

![Version](https://img.shields.io/badge/version-0.1.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Arduino](https://img.shields.io/badge/Arduino-MEGA%202560-00979D.svg)

## 📋 Présentation

Le VOBC est un ordinateur de bord complet pour fourgon aménagé permettant de :
- 🌡️ Monitorer température intérieure/extérieure et humidité
- ⚡ Gérer l'énergie (batterie, panneaux solaires, consommation)
- ⚠️ Détecter les dangers (CO, fuite de gaz)
- 📊 Mesurer l'horizontalité pour stationnement
- 💧 Surveiller les niveaux d'eau
- 🔔 Alertes sonores et visuelles configurables
- 📈 Historique des données sur carte SD

## 🎯 Fonctionnalités

### Phase 1 - INDISPENSABLE ✅
- [x] Température intérieure + humidité (BME280)
- [x] Température extérieure (DS18B20)
- [x] Détection CO (MQ-7)
- [x] Détection gaz GPL (MQ-2)
- [x] Monitoring batterie (INA226)
- [x] Horizontalité 3 axes (MPU6050)
- [x] Détection ouverture portes (Reed Switch)
- [x] Alertes visuelles (LED RGB) et sonores (Buzzer)
- [x] Affichage LCD 20x4 I2C
- [x] Navigation par encodeur rotatif

### Phase 2 - IMPORTANTE 🔶
- [ ] Niveaux réservoirs eau (HC-SR04)
- [ ] Ventilation automatique (Module relais)
- [ ] Détecteur de mouvement (PIR)
- [ ] Monitoring panneaux solaires (INA226 #2)
- [ ] Éclairage automatique (LDR)

### Phase 3 - MOYENNE 🔹
- [ ] Horodatage (RTC DS3231)
- [ ] Historique données (Module SD)
- [ ] Orientation/boussole (HMC5883L)
- [ ] Prévision solaire algorithmique
- [ ] Interface Web/Bluetooth (optionnel)

## 🔧 Matériel

### Contrôleur
- Arduino MEGA 2560 R3

### Capteurs Phase 1
- BME280 (I2C) - Température + Humidité + Pression
- INA226 (I2C) - Courant/Tension batterie
- MPU6050 (I2C) - Gyroscope + Accéléromètre
- MQ-7 (Analogique) - Détecteur CO
- MQ-2 (Analogique) - Détecteur GPL/Butane
- DS18B20 (One-Wire) - Température extérieure étanche
- Reed Switch - Détection portes

### Interface
- LCD 20x4 avec module I2C
- Encodeur rotatif KY-040
- LED RGB + Buzzer piézo

### Alimentation
- Convertisseur DC-DC 12V → 5V (LM2596/MP1584) 3A min

Voir [HARDWARE.md](docs/HARDWARE.md) pour détails complets.

## 📁 Structure du projet

```
.
├── README.md                    # Ce fichier
├── LICENSE                      # Licence MIT
├── CHANGELOG.md                 # Historique des versions
├── docs/                        # Documentation
│   ├── HARDWARE.md             # Schémas et câblage
│   ├── SOFTWARE.md             # Architecture logicielle
│   ├── CONFIGURATION.md        # Configuration système
│   ├── INSTALL.md              # Guide d'installation
│   └── images/                 # Schémas et photos
├── firmware/                    # Code Arduino
│   └── van_onboard_computer/
│       ├── van_onboard_computer.ino  # Fichier principal
│       ├── config.h                   # Configuration
│       └── *.cpp/*.h                  # Modules
├── hardware/                    # Fichiers matériels
│   ├── schematics/             # Schémas électriques
│   ├── pcb/                    # PCB (phase finale)
│   └── datasheets/             # Datasheets composants
├── testing/                     # Tests unitaires
│   ├── test_codes/             # Codes de test capteurs
│   └── testing_README.md       # Guide de test
└── tools/                       # Outils et scripts
    └── scripts/                # Scripts utilitaires
```

## 🚀 Installation rapide

### Prérequis
- Arduino IDE 2.x
- Librairies (voir [INSTALL.md](docs/INSTALL.md))

### Étapes
```bash
# Cloner le repo
git clone https://github.com/[votre-username]/van-onboard-computer.git
cd van-onboard-computer

# Installer les librairies nécessaires
# (voir docs/INSTALL.md)

# Ouvrir le firmware
# Arduino IDE: Ouvrir firmware/van_onboard_computer/van_onboard_computer.ino
# PlatformIO: Ouvrir le dossier dans VS Code

# Configurer dans config.h
# Compiler et uploader sur Arduino MEGA
```

## 📚 Documentation

- [Guide matériel](docs/HARDWARE.md) - Schémas de câblage complets
- [Architecture logicielle](docs/SOFTWARE.md) - Structure du code
- [Configuration](docs/CONFIGURATION.md) - Paramétrage système
- [Installation](docs/INSTALL.md) - Guide d'installation pas à pas
- [Tests](testing/testing_README.md) - Procédures de test

## 🧪 Tests

Chaque capteur dispose de son code de test unitaire dans `testing/test_codes/`. Voir [testing_README.md](testing/testing_README.md) pour la procédure complète.

## 🤝 Contribution

Les contributions sont bienvenues ! 

1. Fork le projet
2. Créer une branche (`git checkout -b feature/amelioration`)
3. Commit (`git commit -am 'Ajout fonctionnalité'`)
4. Push (`git push origin feature/amelioration`)
5. Créer une Pull Request

## 📝 Changelog

Voir [CHANGELOG.md](CHANGELOG.md) pour l'historique des versions.

## 📄 Licence

Ce projet est sous licence MIT - voir [LICENSE](LICENSE) pour plus de détails.

## 👤 Auteur

**Frédéric BAILLON**
- GitHub: [@fbaillon19](https://github.com/fbaillon19)

## 🙏 Remerciements

- Communauté Arduino
- Fabricants de capteurs (Bosch, Texas Instruments, etc.)
- Tous les contributeurs du projet

## 📸 Galerie

_Photos à venir lors de l'installation..._

---

⭐ Si ce projet vous aide, n'hésitez pas à lui donner une étoile !
