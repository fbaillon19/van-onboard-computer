# CHANGELOG - Tests matériels

## [2024-12-10] Session de debug et simplification

### ✅ Corrections appliquées

#### **KY040 (Encodeur rotatif)**
- **v2.1.0** : Correction comptage (1 clic = 1 position au lieu de 2)
- Ajout filtre anti-rebond logiciel (2ms) pour réduire bruit capacitif
- Passage aux interruptions matérielles (méthode ultra-fiable)
- Alternative proposée : intégration librairie Paul Stoffregen

#### **BME280 (Température/Humidité/Pression)**
- **v1.0.1** : FIX critique - Ajout paramètre `&Wire` dans `bme.begin()`
- Ajout méthodes manquantes : `hasCondensationRisk()`, `getSeaLevelPressure()`
- Ajout délais stabilisation (10ms, 50ms)
- Programme diagnostic bas niveau créé (`diagnostic_bme280.ino`)
- **Note** : Composant testé = BMP280 (pas d'humidité), BME280 commandé

#### **MPU6050 (Horizontalité)**
- **v2.0.0** : Simplification majeure - Accéléromètre uniquement
- Suppression gyroscope (cause de dérive thermique 5-12°)
- Lecture I2C directe (plus de dépendance à MPU6050_tockn)
- Filtre passe-bas exponentiel simple (alpha = 0.3)
- Calibration sur plan horizontal (800 échantillons)
- Dérive finale : < 2° (vs 12° avant) ✓
- Code réduit de ~300 lignes, plus maintenable

### 📚 Fichiers modifiés

```
testing/test_codes/
├── test_ky040/
│   └── KY040Encoder.h (v2.1.0)
├── test_bme280/
│   ├── BME280Sensor.h (v1.0.1)
│   └── diagnostic_bme280.ino (nouveau)
└── test_mpu6050/
    ├── MPU6050Sensor.h (v2.0.0 - refonte complète)
    └── test_mpu6050.ino (v2.0.0 - simplifié)
```

### 🎯 Philosophie de conception

**Avant** : Code "apprendre le module", complexe, features inutiles
**Maintenant** : Code pragmatique focalisé sur le BESOIN RÉEL

- MPU6050 : Juste mesurer horizontalité (pas besoin gyroscope)
- Tests : Valider "ça marche pour mon projet", pas "apprendre le module"
- Simplicité : Moins de code = moins de bugs = plus maintenable

### 📊 Statut des tests

| Test | Statut | Notes |
|------|--------|-------|
| test_arduino | ✅ OK | - |
| test_bme280 | ⏸️ Attente matériel | BMP280 détecté, BME280 commandé |
| test_buzzer | ⏸️ Attente PN2222 | Transistor manquant |
| test_ds18b20 | ✅ OK | - |
| test_ina226 | ⏳ À faire | Test sur PCB préféré |
| test_ky040 | ✅ OK | Condensateurs 100nF ajoutés |
| test_lcd | ✅ OK | - |
| test_led_rgb | ⏳ À faire | - |
| test_mpu6050 | ✅ OK | Refonte complète |
| test_mq2 | ⏳ À faire | - |
| test_mq7 | ⏳ À faire | - |

**Progression** : 6/11 tests validés (55%)

### 🔧 Améliorations matérielles

- **KY040** : Ajout de 2 condensateurs 100nF (CLK-GND, DT-GND) pour filtrage bruit
- **MPU6050** : AD0 à GND confirmé (adresse 0x68)

### 📝 Documentation

- Ajout explications détaillées sur les offsets de calibration (MPU6050)
- Diagrammes de décision pour choix des outils (encodeur)
- Comparaisons avant/après pour justifier les simplifications

### 🚀 Prochaines étapes

1. Réception BME280 → test_bme280 complet
2. Réception PN2222 → test_buzzer
3. Tests restants : LED RGB, MQ2, MQ7
4. Test INA226 sur PCB assemblé
5. Tests recommandés : test_relays, test_integration

---

**Résumé** : Session très productive avec 3 corrections majeures. Code plus simple, plus fiable, mieux documenté. Approche pragmatique validée.
