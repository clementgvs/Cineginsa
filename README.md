<p align="center">
  <img src="assets/banner.png" alt="Cineginsa - auto-régie pour le club ciné" width="100%">
</p>

<p align="center">
  <img alt="Qt 6" src="https://img.shields.io/badge/Qt-6.5%2B-FF7A1A?logo=qt&logoColor=white&labelColor=0C1328">
  <img alt="C++17" src="https://img.shields.io/badge/C%2B%2B-17-FF7A1A?logo=cplusplus&logoColor=white&labelColor=0C1328">
  <img alt="CMake" src="https://img.shields.io/badge/build-CMake-FF7A1A?logo=cmake&logoColor=white&labelColor=0C1328">
  <img alt="Plateformes" src="https://img.shields.io/badge/plateformes-Windows%20%7C%20macOS%20%7C%20Linux-22D3C5?labelColor=0C1328">
  <img alt="Licence MIT" src="https://img.shields.io/badge/licence-MIT-22D3C5?labelColor=0C1328">
  <img alt="Statut" src="https://img.shields.io/badge/statut-beta-FFF3E0?labelColor=0C1328">
</p>

# Cineginsa

Une **auto-régie légère** pour les séances de club ciné. Elle remplace OBS et ses scripts Lua : on saisit les infos de la séance, on pilote au clavier, et l'appli envoie le bon contenu (snacks et prix, infos du film, compte à rebours, puis le film lui-même) sur l'écran du projecteur, pendant que la régie reste sur l'écran du PC.

## Fonctionnement

Deux fenêtres :

| Fenêtre | Écran | Rôle |
|---|---|---|
| **Régie** (Qt Widgets) | PC | Saisie de la séance, boutons de scènes, choix du film |
| **Affichage** (QML) | Projecteur | Scènes animées et lecture de la vidéo |

## Prérequis

- **Qt 6.5 ou +** (modules *Widgets*, *Quick*, *Multimedia*). FFmpeg gère les codecs.

### Linux (apt)

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev \
  qt6-multimedia-dev qml6-module-qtquick qml6-module-qtmultimedia
```

## Compiler et lancer

```bash
git clone https://github.com/clementgvs/Cineginsa.git
cd Cineginsa
cmake -S . -B build
cmake --build build
./build/Cineginsa
```

## Structure

```
Cineginsa/
├── CMakeLists.txt
├── main.cpp
├── mainwindow.cpp / .h / .ui   ← fenêtre de régie
├── display.qml                 ← écran projecteur
├── assets/
│   ├── banner.png
│   └── ... 
├── LICENSE
└── THIRD_PARTY_NOTICES.md
```

## Licence

- **Code** : [MIT](LICENSE).
- **Qt** : utilisé sous licence **LGPL v3**, en liaison dynamique. Voir [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), à joindre à chaque exécutable distribué avec le texte de la LGPL.
- **Visuels du club** (logo, affiches, images de snacks) : non couverts par la licence MIT, tous droits réservés. Ils ne sont pas inclus dans le dépôt.
