# Com_ESP32
# Détecteur Frelon Asiatique ESP32-S3 CAM - comm avec smartphone 
## L'idée est de relever avec un smartphone des pots à mèche simplement équipés d'un ESP32CAM et d'un déclencheur (interrupteur en U)


## Guide d'Installation et d'Utilisation

### 📋 MATÉRIEL REQUIS
- ESP32-S3 N16R8 CAM (avec caméra OV3660)
- Capteur PIR HC-SR501
- Bouton poussoir
- Résistances pull-down (10kΩ)
- Alimentation batterie + panneau solaire

### 🔧 CÂBLAGE
```
ESP32-S3        Composant
--------        ---------
GPIO 13    ←→   PIR (OUT)
GPIO 14    ←→   Bouton
GND        ←→   GND (PIR + Bouton)
3.3V       ←→   VCC PIR
```

### 💻 CONFIGURATION IDE ARDUINO

**1. Installation ESP32:**
- Fichier → Préférences
- URLs supplémentaires: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
- Outils → Type de carte → Gestionnaire de cartes
- Installer "ESP32 by Espressif Systems"

**2. Configuration carte:**
```
Carte: ESP32S3 Dev Module
Upload Speed: 921600
USB CDC On Boot: Enabled  
Flash Size: 16MB (3MB APP / 12.5MB FATFS)
PSRAM: OPI PSRAM
Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)
```

### 📤 TÉLÉVERSEMENT

1. Connecter l'ESP32-S3 via USB
2. Sélectionner le port COM
3. Téléverser le code
4. Ouvrir le moniteur série (115200 baud)

### 🎯 UTILISATION TERRAIN

#### Mode Capture Autonome
1. Le système est en deep sleep permanent
2. Le PIR détecte un mouvement → Photo automatique
3. Photo sauvegardée dans /photos/
4. Retour immédiat en deep sleep
5. Max 20 photos (FIFO: supprime la plus ancienne)

#### Mode Relève Terrain
1. Appuyer sur le bouton
2. L'ESP32 se réveille et active le WiFi
3. Sur votre smartphone:
   - WiFi → Chercher "Frelon-Cam"
   - Mot de passe: frelon2026
   - Ouvrir navigateur → http://192.168.4.1
4. Interface web:
   - Voir galerie photos
   - Télécharger individuellement
   - Supprimer toutes
5. Timeout auto 3 min → Retour deep sleep

### ⚙️ PERSONNALISATION

Modifier dans le code:

**Résolution photo:**
```cpp
#define PHOTO_RESOLUTION FRAMESIZE_QVGA  // QVGA, VGA, SVGA
```

**Qualité JPEG:**
```cpp
#define JPEG_QUALITY 12  // 10=meilleur, 63=pire
```

**Nombre de photos:**
```cpp
#define MAX_PHOTOS 50  // Augmenter selon stockage
```

**GPIO:**
```cpp
#define GPIO_PIR 13
#define GPIO_BUTTON 14
```

**WiFi:**
```cpp
#define AP_SSID "MonNom"
#define AP_PASSWORD "MonMotDePasse"
```

**Timeout relève:**
```cpp
#define WEB_TIMEOUT_MS 300000  // 5 minutes
```

### 🔋 OPTIMISATION BATTERIE

**Consommation:**
- Deep sleep: ~10µA
- Capture photo: ~200mA pendant 2-3s
- Mode relève: ~150mA

**Durée batterie estimée (LiPo 2000mAh):**
- 10 captures/jour: ~2-3 semaines
- Avec panneau solaire 5V 1W: autonomie infinie

### 🐛 DÉPANNAGE

**Erreur caméra:**
- Vérifier connexion ruban caméra
- Redémarrer l'ESP32
- Vérifier PSRAM activé

**Erreur FFat:**
- Premier boot: formatage auto
- Si persistant: reflasher avec partition FATFS

**WiFi ne se connecte pas:**
- Vérifier mot de passe (8 car min)
- Redémarrer smartphone WiFi
- Vérifier timeout pas expiré

**Photos floues:**
- Augmenter JPEG_QUALITY (valeur plus basse)
- Vérifier focus caméra
- Ajouter délai avant capture

### 📊 MONITEUR SÉRIE

Messages normaux:
```
Boot #1 | Photos: 0/20
⚡ PREMIER DÉMARRAGE
✅ Système initialisé
💤 Deep Sleep
```
```
Boot #2 | Photos: 0/20
📸 PIR - Mode Capture
📸 Capture...
✅ 45231 bytes
📊 Total: 1/20 photos
💤 Deep Sleep
```
```
Boot #3 | Photos: 1/20  
📱 BOUTON - Mode Relève
WiFi: Frelon-Cam
IP: http://192.168.4.1
⏱️ Timeout: 3 minutes
```

### 📸 FORMAT FICHIERS

- Nom: `/photos/[timestamp].jpg`
- Exemple: `/photos/12345678.jpg`
- Timestamp = millis() au moment capture

### ✅ CHECKLIST DÉPLOIEMENT

- [ ] Code téléversé
- [ ] PIR testé (LED clignote)
- [ ] Bouton testé (WiFi démarre)
- [ ] Batterie chargée
- [ ] Panneau solaire connecté
- [ ] Boîtier étanche
- [ ] Position optimale PIR vers ruche
- [ ] Test capture photo OK
- [ ] Test relève smartphone OK

### 📞 SUPPORT

Problème? Vérifier:
1. Moniteur série pour messages erreur
2. Partition FFat correcte
3. Câblage GPIO
4. Alimentation stable

Bon piégeage de frelons! 🐝
