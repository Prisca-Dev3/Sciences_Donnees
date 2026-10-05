# SVM From Scratch — Classification MNIST

Implémentation complète d'un SVM à noyau RBF avec l'algorithme SMO, appliqué à la classification multi-classes du dataset MNIST (chiffres manuscrits 0–9). Aucune librairie de machine learning n'est utilisée — uniquement NumPy et Pandas.

---

## Structure du projet

```
projet-svm/
├── SVM_one_vs_one.py                    # Code principal (implémentation complète)
├── SVM_From_Scratch_MNIST.ipynb    # Notebook d'explication
├── README.md                       # Ce fichier
└── dataset/
    ├── mnist_train_32.csv          # Données d'entraînement
    └── mnist_test_32.csv           # Données de test
```

---

## Prérequis

- Python **3.8+**
- pip

### Installation des dépendances

```bash
pip install numpy pandas jupyter
```

---

## Lancer le script principal

```bash
python SVM_one_vs_one.py
```

Le script va automatiquement :
1. Chercher les fichiers CSV dans `./dataset/` ou dans le répertoire parent
2. Standardiser les données
3. Entraîner 45 classifieurs binaires (OvO sur 10 classes)
4. Afficher la précision globale sur le jeu de test

**Sortie attendue :**
```
=== SVM Multi-Classes (OvO) From Scratch: MNIST ===
Chargement des donnees...
Standardisation des donnees...
Entrainement sur un sous-ensemble de 3000 images...
Demarrage de l'entrainement OvO: 45 modeles a entrainer.
  -> Entrainement 45/45 : Classe 8 vs Classe 9
Entrainement de tous les modeles termine !
Prediction sur 1000 images test...
>>> Precision test globale (10 classes): ~87.00% <<<
```


---

## Lancer le notebook

```bash
jupyter notebook SVM_From_Scratch_MNIST.ipynb
```

Ou avec JupyterLab :

```bash
jupyter lab SVM_From_Scratch_MNIST.ipynb
```

Le notebook contient l'intégralité du code documenté cellule par cellule, avec les formules mathématiques, des démonstrations interactives et un bilan final. **Toutes les cellules sont auto-contenues** — il n'est pas nécessaire d'avoir les fichiers CSV pour lire les explications et tester les classes individuellement (le chargement MNIST est isolé dans sa propre cellule).

---

## Emplacement des données

Le script cherche les fichiers CSV dans cet ordre :

| Priorité |         Chemin testé         |
|:--------:|:----------------------------:|
| 1 | `dataset/mnist_train_32.csv` |
| 2 |   `../mnist_train_32.csv`    |
| 3 |    `./mnist_train_32.csv`    |

Placez les fichiers dans `dataset/` à la racine du projet, ou ajustez la fonction `charger_csv_mnist()` dans le code.

---

## Hyperparamètres

Les valeurs par défaut sont définies dans le bloc `__main__` de `svm_mnist.py` :

| Paramètre | Valeur par défaut | Description |
|:---------:|:-----------------:|:-----------:|
| `C` |       `1.0`       | Régularisation (compromis marge / erreurs) |
| `gamma` |      `0.001`      | Largeur du noyau RBF |
| `max_passes` |        `3`        | Critère d'arrêt SMO |
| `N_SAMPLES_TRAIN` |      `3000`       | Taille du sous-ensemble d'entraînement |
| `N_SAMPLES_TEST` |      `1000`       | Taille du sous-ensemble de test |

Pour un test rapide de la logique, réduisez `N_SAMPLES_TRAIN` à `500` — l'entraînement prendra alors quelques minutes.

---

## Architecture du code

```
StandardScalerScratch       Normalisation (X - μ) / σ
NoyauRBF                    K(xi, xj) = exp(-γ ||xi - xj||²), vectorisé
SVM_SMO_Optimise            SVM binaire via algorithme SMO
└── _choisir_j()            Heuristique max |Ei - Ej|
└── fit()                   Boucle SMO avec cache des erreurs
└── decision_function()     f(x) = Σ λi·yi·K(xi,x) + b
SVM_MultiClass_OvO          Stratégie One-vs-One (45 modèles pour MNIST)
└── fit()                   Entraîne un SVM par paire de classes
└── predict()               Vote majoritaire
```