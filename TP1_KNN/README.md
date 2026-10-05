# Système de Classification des Catégories de Revenus

## Vue d'Ensemble du Projet

Ce projet a pour objectif de classifier les pays selon les catégories de revenus définies par la Banque Mondiale (Faible revenu, Revenu intermédiaire tranche inférieure, Revenu intermédiaire tranche supérieure, Revenu élevé, et Non classifié) en se basant sur un vaste ensemble d'indicateurs macroéconomiques et sociaux. La méthodologie fondamentale et les étapes de modélisation sont formalisées dans le notebook Jupyter `TP_Serie1 (2)-1_(0).ipynb`.

L'objectif principal de ce projet est d'établir un pipeline prédictif strictement rigoureux et robuste. Par conséquent, la prédiction du groupe de revenus d'une nouvelle observation inédite (inférence) nécessite de respecter scrupuleusement les paramètres de prétraitement établis lors de la phase d'entraînement.

## Réaliser une Inférence sur de Nouvelles Observations

Pour prédire la catégorie de revenus d'un nouveau pays ou d'une nouvelle observation, il est obligatoire d'appliquer les paramètres de transformation exacts, appris de manière dynamique à partir du jeu de données d'entraînement.

Toute tentative de recalculer ces paramètres (tels que les médianes ou les métriques de normalisation) directement sur de nouvelles données introduirait des incohérences statistiques et dévierait de l'espace de représentation appris, causant une fuite de données (Data Leakage).

### Procédure d'Inférence Théorique

1. **Alignement des Caractéristiques (Features)** : Conserver exclusivement les 246 indicateurs exacts définis par le processus d'entraînement.
2. **Imputation des Valeurs Manquantes** : Imputer les valeurs manquantes avec les médianes exactes de la distribution d'entraînement.
3. **Standardisation (Z-Score)** : Transformer la nouvelle observation avec le `StandardScaler` (`scaler.transform()`).
4. **Prédiction du Modèle** : Soumettre le vecteur standardisé à l'objet `KNeighborsClassifier` entraîné.
5. **Décodage de la Cible** : Convertir l'indice entier résultant (0 à 4) vers la catégorie textuelle d'origine.

---

### Guide Pratique : Code d'Inférence dans le Notebook

Une fois que toutes les cellules du notebook `TP_Serie1 (2)-1_(0).ipynb` ont été exécutées avec succès, le modèle final et les paramètres de transformation sont disponibles en mémoire.

Pour inférer la classe d'une nouvelle entrée de manière totalement indépendante, ajoutez une nouvelle cellule de code à la toute fin du notebook et exécutez l'extrait suivant :

```python
import pandas as pd
import numpy as np

# 1. Définir votre nouvelle observation sous forme de dictionnaire.
# Il s'agit des codes bruts des indicateurs de la Banque Mondiale.
nouvelle_observation = {
    'AG.LND.AGRI.K2': 500000,        # Superficie des terres agricoles
    'NY.GDP.MKTP.CD': 15000000000,   # PIB (US$)
    # Vous pouvez omettre des indicateurs, ils seront traités comme manquants
}

# Conversion en DataFrame (représentant 1 pays/observation)
df_nouveau = pd.DataFrame([nouvelle_observation])

# 2. Sécuriser les caractéristiques requises (Alignement strict)
# X_train_filtered est le DataFrame d'entraînement conservé en mémoire (246 colonnes)
colonnes_requises = X_train_filtered.columns

for col in colonnes_requises:
    if col not in df_nouveau.columns:
        # Assigner une valeur NaN de sécurité si la donnée n'est pas fournie
        df_nouveau[col] = np.nan

# Réordonner le DataFrame pour qu'il coïncide parfaitement avec l'entraînement
df_nouveau = df_nouveau[colonnes_requises]

# 3. Imputation par les médianes d'entraînement
# Calcul dynamique des médianes depuis le jeu d'entraînement filtré
medianes_entrainement = X_train_filtered.median(skipna=True)
df_nouveau = df_nouveau.fillna(medianes_entrainement)

# 4. Standardisation (Z-Score)
# 'scaler' est l'objet StandardScaler utilisé précédemment dans le notebook.
# Nous utilisons impérativement transform(), jamais fit_transform()
observation_standardisee = scaler.transform(df_nouveau)
df_nouveau_scaled = pd.DataFrame(observation_standardisee, columns=colonnes_requises)

# 5. Prédiction avec le modèle k-NN optimal
# 'best_knn' est l'entité du modèle formel optimal retenu à la dernière étape
prediction_num = best_knn.predict(df_nouveau_scaled)[0]

# 6. Décodage et Affichage
dictionnaire_revenus = {
    0: 'Faible revenu (Low income)',
    1: 'Revenu intermédiaire, tranche inférieure (Lower middle income)',
    2: 'Revenu intermédiaire, tranche supérieure (Upper middle income)',
    3: 'Revenu élevé (High income)',
    4: 'Non classifié (Unclassified)'
}

categorie_predite = dictionnaire_revenus.get(prediction_num, "Inconnue")
print(f"La catégorie de revenu assignée à cette observation est : {categorie_predite}")
```

### Dépendances pour la Mise en Production

Pour un déploiement réel ou des inférences récurrentes hors du notebook, il est impératif de sérialiser et d'exporter les artefacts suivants à l'issue de la séquence d'entraînement (à l'aide de bibliothèques telles que `joblib` ou `pickle`) :

- La liste ordonnée des 246 caractéristiques conservées.
- Le dictionnaire contenant les 246 médianes d'entraînement.
- L'objet `StandardScaler` ajusté (`scaler`).
- Le modèle `KNeighborsClassifier` entraîné (`best_knn`).

Cette infrastructure technique de sauvegarde évite de devoir exécuter l'intégralité du pipeline ETL et l'entraînement à chaque nouvelle instanciation de l'environnement de prédiction.

## Contexte du Pipeline d'Entraînement

Le notebook documente le pipeline d'une façon conçue catégoriquement pour prévenir toute fuite de données :

- **Ségrégation Stricte des Données** : L'opération `train_test_split` est exécutée avant toute estimation de paramètres (imputation, filtrage des corrélations, standardisation).
- **Filtrage des Corrélations** : Le seuil de détection de la multicolinéarité est fixé à `|r| > 0.8`. Les matrices redondantes sont identifiées et écartées uniquement sur le jeu d'entraînement.
- **Décision de Mise à l'Échelle** : La normalisation standard Z-Score a été privilégiée par rapport à la mise à l'échelle Min-Max afin de maintenir les structures de distribution relatives et d'atténuer les distorsions liées aux valeurs aberrantes extrêmes inhérentes aux données macroéconomiques.

## Auteur et Attribution

Projet : Classification des catégories de revenus (Approche k-NN)
Source des données : World Bank Open Data
Année : 2026

Les données brutes sont issues des archives du domaine public fournies par la Banque Mondiale.
