### TP INF 372 : Analyse de la Consommation Data

| Nom Complet                         | Matricule |
| :---------------------------------- | :-------- |
| KAMDOM NGUETCHESSI MERVEILLE PRISCA | 23V2060   |
| YOKO AYMERICK JEFFREY               | 23U2983   |
| KENNE YEMTSA LOWEL RAYANN           | 23U2477   |

---

# Projet de Machine Learning : Analyse et Modélisation de la Consommation Internet

Ce dossier rassemble l'ensemble des travaux pratiques (TP) réalisés pour modéliser, analyser et prédire les comportements de consommation de données internet d'un panel d'utilisateurs.

Le projet est divisé en deux grands axes analytiques : la prédiction quantitative (Régression) et l'inférence comportementale (Classification multi-classes).

## Objectifs du Projet

### **1. TP1 : Modélisation Quantitative (Régression)**

L'objectif est de prédire les futures quantités de données (en Mo) qu'un individu est susceptible de consommer.

**Approches utilisées** : Analyse de séries temporelles (Time Series) et régression via l'algorithme des K-Plus Proches Voisins (KNN).

_Enjeu_ : Capturer la tendance et la vélocité de la consommation au fil du temps.

### 2. TP2 : Inférence Comportementale (Classification)

L'objectif est de prédire la période de la journée (Matin, Après-midi, Soir, Nuit) d'une session internet à partir de sa signature volumétrique et technique.

**Contrainte majeure** : Afin d'éviter toute fuite de données (Data Leakage), l'heure exacte de la connexion a été strictement exclue des variables explicatives. Le modèle doit déduire le moment de la journée uniquement via le comportement.

#### Modèles comparés :

- K-Nearest Neighbors (KNN)
- Régression Logistique
- Support Vector Machine (SVM)
- Gaussian Naive Bayes

## Architecture du Projet

Le projet est organisé de manière modulaire, séparant les données brutes des données traitées, et isolant chaque approche algorithmique :

```plaintext
.
├── data/
│   ├── raw/                 # Datasets individuels bruts de chaque utilisateur (CSVs)
│   └── processed/           # Datasets fusionnés et nettoyés prêts pour l'apprentissage
├── TP1(Regression)/
│   └── Modelisation_Regression.ipynb   # Notebook de prédiction des volumes futurs (Regression Lineaire et KNN)
└── TP2(Classification)/
    ├── SVM_-_Regression_Logistique/    #  CONTIENT LE PIPELINE DE PRÉPARATION CENTRAL
    │   ├── 01_Composition_et_Nettoyage.ipynb
    │   ├── 02_Analyse_Exploratoire_EDA.ipynb
    │   ├── 03_Feature_Engineering_et_Pretraitement.ipynb
    │   ├── 04_Classification_Logistique.ipynb
    │   └── 05_Classification_SVM.ipynb
    ├── KNN/
    │   └── TP_connexion_KNN.ipynb      # Classification par plus proches voisins
    └── Naive_Bayes/
        └── TP_connexion_Naive_Bayes.ipynb # Classification bayésienne avec PowerTransformer
```

## Execution du projet

### **1. Prérequis et Installation**:

Assurez-vous d'avoir un environnement Python fonctionnel (version 3.8+ recommandée) avec les bibliothèques standards de Data Science installées. Vous pouvez les installer via pip :

```bash
pip install pandas numpy scikit-learn matplotlib seaborn jupyter
```

### **2. Ordre d'exécution recommandé**:

L'architecture du projet nécessite de générer les données propres avant de lancer les modèles de classification. Voici la marche à suivre :

**TP1: Régression (Indépendante)**

Vous pouvez exécuter le notebook de régression à tout moment, car il possède sa propre logique d'analyse temporelle.

Ouvrez et exécutez : TP1(Regression)/Modelisation_Regression.ipynb

**TP2: Classification**

- _KNN_

Vous pouvez exécuter le notebook de régression à tout moment, car il possède sa propre logique d'analyse temporelle.

Ouvrez et exécutez : TP2(Classification)/KNN/TP_connexion_KNN.ipynb

- _Naive Bayes_

Vous pouvez exécuter le notebook de régression à tout moment, car il possède sa propre logique d'analyse temporelle.

Ouvrez et exécutez : TP2(Classification)/KNN/TP_connexion_KNN.ipynb

- _SVM & Regression Logistique_
  Les fichiers originaux étant séparés dans data/raw/, il faut d'abord les fusionner et les nettoyer. Ce travail préparatoire a été centralisé dans le dossier SVM/Régression Logistique.

       * Allez dans TP2(Classification)/SVM\_-_Regression_Logistique/

       * Exécutez 01_Composition_et_Nettoyage.ipynb (fusionne les CSV bruts).

       * Exécutez 02_Analyse_Exploratoire_EDA.ipynb (analyse statistique et graphique).

       * Exécutez 03_Feature_Engineering_et_Pretraitement.ipynb (génère les fichiers finaux dataset_classification_basic.csv et dataset_classification_enhanced.csv).

       * Entraînement et Évaluation des Modèles de Classification**

          Une fois le dataset de classification généré, vous pouvez explorer les performances de chaque algorithme en exécutant les notebooks restants de manière indépendante :

       * Régression Logistique & SVM : Exécutez les notebooks 04 et 05 dans le même dossier.
