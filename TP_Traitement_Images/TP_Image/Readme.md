# INF3722 - Traitement d'Images (C)
# KAMDOM NGUETCHESSI MERVEILLE PRISCA - 23V2060 - LICENCE 3 INFORMATIQUE


## Description du TP
Dans le cadre du traitement d'images , ce TP est conçue dans un cadre académique, rassemblant et implémentant l'ensemble des concepts fondamentaux, algorithmes spatiaux, fréquentiels et morphologiques étudiés au cours de l'année sur les traitements d'images.Il intègre un pipeline complet allant du chargement de fichiers matriciels à la gestion automatisée des flux de sortie, organisée rigoureusement à travers 7 répertoires de résultats distincts.

---

## 1. Architecture Logicielle & Modularité

Le code est structuré selon les principes de modularité et de séparation des préoccupations (*Separation of Concerns*). Chaque module correspond à une unité thématique claire du cours :

* **`main.c`** : Interface utilisateur (IHM) interactive. Gère les menus, l'aiguillage des commandes, la journalisation des métriques console et l'automatisation des sauvegardes.
* **`image.h`** : Structure de données centrale (`ImageGris`, `ImageCouleur`), macros de manipulation de pixels (`PIXEL`, `CLAMP`) et prototypes globaux.
* **`io_image.c`** : Entrées/Sorties. Prise en charge universelle et transparente des formats PGM (P5) et PPM (P6).
* **`traitement_base.c` (Cours 02)** : Histogrammes, égalisations (globales/locales), transformations de contraste (gamma, saturation) et interpolations spatiales (voisin, bilinéaire, bicubique).
* **`convolution.c` (Cours 03)** : Opérateurs de filtrage spatial linéaire et non-linéaire (Moyenneur, Gaussien, Médian, Min, Max) avec gestion paramétrable des effets de bord (Miroir, Zéro-padding, Clamping).
* **`fourier.c` (Cours 04)** : Transformée de Fourier Rapide 2D (FFT/IFFT Radix-2 Cooley-Tukey), spectres logarithmiques recentrés (`fftshift`) et filtres fréquentiels (Passe-haut, Passe-bas, Filtre Coupe-bande / Notch).
* **`contours.c` (Cours 05)** : Opérateurs de dérivation discrets (Roberts, Prewitt, Sobel), Laplaciens (4 et 8-connexes), Laplacien de Gaussienne (LoG) et détection de passages à zéro.
* **`segmentation.c` (Cours 06)** : Seuillage d'Otsu, multi-seuillages, classification par K-moyennes, division-fusion (*Split & Merge*) par Quadtree et croissance de régions.
* **`images_binaires.c` (Cours 07)** : Morphologie mathématique (érosion, dilatation, ouverture, fermeture, gradients), étiquetage de composantes connexes et cartes de distances discrètes.

---

## 2. Spécifications & Optimisations Clés

### A. Pipeline d'Aperçu Réaligné
Conformément aux exigences de production d'images, l'affichage se fait intégralement par un moteur de rendu matriciel. 
* **Génération PGM systématique :** Chaque demande d'aperçu génère instantanément un fichier binaire `_apercu.pgm`.
* **Flux direct :**  La sauvegarde est entièrement automatisée, garantissant que l'image d'aperçu et le résultat final au sein du dossier cible sont strictement identiques.

---

## 3. Analyse des Métriques Topologiques (Focus Cours 07)

L'exécution des outils d'analyse morphologique binaire produit des indicateurs précieux sur la topologie des images cibles :

### Étiquetage des Composantes Connexes
Sur l'image de référence ("tank.pgm"), après exécution via la terminal l'algorithme d'étiquetage en deux passes renvoie :
* **4-Connexité = 918 objets** (Voisinage orthogonal restrictif : Nord, Sud, Est, Ouest). Ces objets representent les morceaux isolés
* **8-Connexité = 473 objets** (Voisinage étendu aux diagonales).

**Interprétation :** Un tel écart (un rapport de près de 2 pour 1) met en évidence une structure d'image hautement fragmentée ou bruitée. Les pixels blancs adjacents uniquement par leurs sommets (diagonales) se retrouvent isolés en 4-connexité, créant une multitude de micro-objets informatiques, tandis qu'ils fusionnent en blocs cohérents sous la règle de la 8-connexité.

### Cartes de Distances Discrètes (Distance Transform)
* **`D4` (Manhattan)** : `Moyenne = 27.29` | Limitation aux déplacements orthogonaux induisant des chemins plus longs pour atteindre le fond noir (valeur moyenne plus élevée).
* **`D8` (Échiquier)** : `Moyenne = 26.44` | Autorisation des sauts diagonaux à coût unitaire, réduisant géométriquement la longueur des trajectoires de recherche (valeur moyenne inférieure).
* **Normalisation (`Max = 255`)** : Afin de rendre les cartes de distances visibles et de pouvoir les exporter au format PGM, les distances brutes subissent une étalonnage dynamique : le pixel le plus enfoui (le plus éloigné de tout bord) reçoit la valeur maximale de luminance ($255$, blanc pur).

---

## 4. Compilation, Dépendances et Exécution

Le TP est rigoureusement conforme à la norme C. La compilation est automatisée via un utilitaire `Makefile` fourni, garantissant l'intégration correcte de toutes les dépendances et l'optimisation des calculs matriciels.

### A. Dépendances Système Requises
Pour pouvoir compiler et exécuter ce projet sous Linux (Ubuntu / Debian), assurez-vous de disposer des outils de développement de base. Si nécessaire, installez-les via votre gestionnaire de paquets :
* **Compilateur :** `gcc` (supportant la norme standard C89/C99).
* **Gestionnaire de build :** `make` (GNU Make).
* **Bibliothèque Mathématique Standard :** `glibc` (la liaison avec la bibliothèque mathématique `-lm` est requise pour l'utilisation de `math.h` dans les modules Fréquentiels, Contours et Segmentation).

Commande d'installation rapide sur Ubuntu :
```bash
sudo apt update && sudo apt install build-essential
```
### B.Arborescence Complète du Projet
.
├── image.h                    # En-tête global et structures de données
├── main.c                     # Point d'entrée et gestion de l'IHM interactive
├── io_image.c                 # Gestion des formats PGM/PPM (E/S)
├── traitement_base.c          # Algorithmes du Cours 02
├── convolution.c              # Algorithmes du Cours 03
├── fourier.c                  # Algorithmes du Cours 04 (FFT 2D)
├── contours.c                 # Algorithmes du Cours 05
├── segmentation.c             # Algorithmes du Cours 06
├── images_binaires.c          # Algorithmes du Cours 07
├── Makefile                   # Script d'automatisation de la compilation
│
├── images_source/             # Répertoire contenant les fichiers d'entrée réels
│   ├── tank.pgm
│   └── sailboat.ppm
│
└── resultats/                 # Répertoire racine des sorties (créé automatiquement après exécution)
    ├── 00_images_entree/
    ├── cours02_traitements_base/
    ├── cours03_convolution/
    ├── cours04_fourier/
    ├── cours05_contours/
    ├── cours06_segmentation/
    └── cours07_images_binaires/

### C. Compilation via Makefile
Pour compiler l'intégralité du TP, ouvrez votre terminal à la racine du projet et exécutez simplement :
```bash
make
```
Note : Pour nettoyer les fichiers objets (.o) et réinitialiser le répertoire avant une nouvelle compilation, utilisez la commande 
```bash 
make clean || make clean clean-resultats 
```
### D. Exécution
Une fois la compilation réussie, lancez le projet à l'aide de la commande suivante :
```bash
./traitement_image
```
Suivez les instructions textuelles du menu interactif pour naviguer à travers les différents modules de cours, exécuter les pipelines de filtrage et générer de manière autonome vos fichiers images d'aperçu et de résultats finaux visible dans le dossier **resultats/**. 

NB : Les images d'entrée sont déjà fournies dans images_source/,aucune manipulation n'est nécessaire avant make && ./traitement_image."
