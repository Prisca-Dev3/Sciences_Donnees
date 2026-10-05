# Documentation du Projet : Analyse & Reporting "Les Débrouillards"

Ce document détaille les étapes nécessaires à l'installation, la configuration et l'exécution du projet d'analyse décisionnelle pour l'entreprise "Les Débrouillards".

## 1. Structure du Projet

Le projet est organisé en plusieurs dossiers pour une meilleure lisibilité et maintenabilité :

```plaintext
TP-groupe/
├── config/               # Fichiers de configuration
│   ├── __init__.py
│   └── db_config.py     # Configuration de la connexion à la base de données
├── dashboards/          # Tableaux de bord Streamlit
│   ├── __init__.py
│   ├── dashboard_commercial.py
│   ├── dashboard_dg.py
│   └── dashboard_marketing.py
├── etl/                 # Scripts ETL (Extract, Transform, Load)
│   ├── __init__.py
│   ├── ETL1.py         # Chargement des données brutes
│   └── ETL3.py         # Alimentation du Data Warehouse
├── sql/                 # Scripts SQL
│   ├── ETL2.sql
│   ├── KPI.sql          # Requêtes pour les KPIs
│   ├── create_vente_db.sql
│   └── create_vente_dw.sql
├── data/                # Fichiers de données
│   └── donnees_ventes.csv
├── docs/                # Documentation
│   └── TP Groupe_Cas_Les-Debrouillards_Reporting.pdf
├── README.md  
|__Rapport_INF371
├── requirements.txt     # Dépendances Python
└── venv/               # Environnement virtuel (créé lors de l'installation)
```

## 2. Prérequis Techniques

Avant de débuter, veuillez vous assurer que les éléments suivants sont installés sur votre environnement :

- **Python 3.12** ou version ultérieure.
- **MySQL Server** (local ou distant).
- **Pip** (gestionnaire de paquets Python).

## 3. Installation de l'Environnement

Il est recommandé d'utiliser un environnement virtuel pour isoler les dépendances du projet.

1. **Création et activation de l'environnement virtuel :**

   ```bash
   python3 -m venv venv
   source venv/bin/activate  # Sur Linux/macOS
   # ou
   .\venv\Scripts\activate  # Sur Windows
   ```

### Option Alternative : Anaconda / Conda

Si vous utilisez Anaconda ou Miniconda, vous pouvez créer l'environnement avec les commandes suivantes :

```bash
conda create --name les_debrouillards python=3.12
conda activate les_debrouillards
```

2. **Installation des dépendances :**

   Veuillez installer les bibliothèques requises listées dans le fichier `requirements.txt`.

   ```bash
   pip install -r requirements.txt
   ```

## 4. Configuration de la Base de Données

Les identifiants de connexion à la base de données sont centralisés dans le fichier `config/db_config.py`.

1. Ouvrez le fichier `config/db_config.py`.
2. Modifiez les variables suivantes avec vos propres informations de connexion MySQL :
   - `DB_USER` : Votre nom d'utilisateur (ex: `root` ou `kyler`).
   - `DB_PASSWORD` : Votre mot de passe.
   - `DB_HOST` : L'adresse de l'hôte (généralement `localhost`).

## 5. Initialisation et Alimentation des Données (ETL)

Le processus ETL (Extract, Transform, Load) permet de créer et de peupler l'entrepôt de données (Data Warehouse).

Exécutez les scripts suivants **dans l'ordre indiqué** :

1. **Création de la base de données source (`vente_db`) :**
   _(Si nécessaire, exécutez le script SQL via votre client MySQL)_

   ```bash
   mysql -u [votre_user] -p < sql/create_vente_db.sql
   ```

2. **Chargement des données brutes :**

   ```bash
   python etl/ETL1.py
   ```

3. **Création de l'entrepôt de données (`vente_DW`) :**
   _(Si nécessaire, exécutez le script SQL via votre client MySQL)_

   ```bash
   mysql -u [votre_user] -p < sql/create_vente_dw.sql
   ```

4. **Exécution de l'ETL complet (Alimentation du Data Warehouse) :**

   ```bash
   python etl/ETL3.py
   ```

## 6. Exécution des Tableaux de Bord

Trois tableaux de bord interactifs sont disponibles, chacun répondant à des questions métiers spécifiques.

Pour les lancer, utilisez la commande `streamlit run` suivie du chemin du fichier.

### 6.1. Dashboard Direction Générale

Ce tableau de bord offre une vision macro-économique de l'entreprise (CA, Marge, Top Régions).

```bash
streamlit run dashboards/dashboard_dg.py
```

### 6.2. Dashboard Commercial

Ce tableau de bord est destiné au pilotage des ventes (Performance des canaux, Produits leaders, Analyse par ville).

```bash
streamlit run dashboards/dashboard_commercial.py
```

### 6.3. Dashboard Marketing

Ce tableau de bord analyse l'efficacité des campagnes promotionnelles (Impact sur le CA et la Marge).

```bash
streamlit run dashboards/dashboard_marketing.py
```

## 7. Analyse par requêtes SQL (KPIs)

Pour ceux qui souhaitent extraire les KPIs directement via SQL dans le Data Warehouse (`vente_DW`), un fichier regroupant les principales requêtes est à disposition.

Exécutez le script dans votre client MySQL (après avoir réalisé l'étape 5) :

```bash
mysql -u [votre_user] -p < sql/KPI.sql
```

---

**Note :** Assurez-vous que le serveur MySQL est en cours d'exécution avant de lancer les scripts.
