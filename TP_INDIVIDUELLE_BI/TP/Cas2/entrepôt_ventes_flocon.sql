-- Nom du Fichier : entrepôt_ventes_flocon.sql
-- Description : Implémentation de l'entrepôt de données (Modèle Flocon de Neige)
--               pour le Cas 2 (Les Débrouillards). 
-- KAMDOM NGUETCHESSI MERVEILLE PRISCA - 23V2060

-- ######################################################################
-- # 1. CRÉATION DES TABLES DE DIMENSIONS (FLOCON)
-- ######################################################################

-- Dimensions Client et Promotion (non décomposées)
CREATE TABLE Client (
    id_client       INTEGER PRIMARY KEY,
    nom_client      VARCHAR(100) NOT NULL,
    type_client     VARCHAR(50) NOT NULL -- Ex: 'Individuel', 'Revendeur'
);

CREATE TABLE Promotion (
    id_promotion    INTEGER PRIMARY KEY,
    nom_promo       VARCHAR(100) NOT NULL,
    remise_pct      DECIMAL(5, 2)
);

-- 1.1. Décomposition Temporelle (Hiérarchie : Temps: Jour -> Mois -> Trimestre -> Année )

CREATE TABLE Annee (
    id_annee        INTEGER PRIMARY KEY,
    annee           SMALLINT NOT NULL
);

CREATE TABLE Trimestre (
    id_trimestre    INTEGER PRIMARY KEY,
    id_annee        INTEGER NOT NULL REFERENCES Annee(id_annee),
    trimestre       SMALLINT NOT NULL
);

CREATE TABLE Mois (
    id_mois         INTEGER PRIMARY KEY,
    id_trimestre    INTEGER NOT NULL REFERENCES Trimestre(id_trimestre),
    mois            SMALLINT NOT NULL
);

CREATE TABLE Temps (
    id_temps        INTEGER PRIMARY KEY, -- Granularité la plus fine
    id_mois         INTEGER NOT NULL REFERENCES Mois(id_mois),
    jour            SMALLINT NOT NULL
);

-- 1.2. Décomposition Produit (Hiérarchie : Produit -> Catégorie)

CREATE TABLE Categorie (
    id_categorie    INTEGER PRIMARY KEY,
    nom_categorie   VARCHAR(50) UNIQUE NOT NULL
);

CREATE TABLE Produit (
    id_produit      INTEGER PRIMARY KEY,
    id_categorie    INTEGER NOT NULL REFERENCES Categorie(id_categorie),
    nom_produit     VARCHAR(100) NOT NULL
);

-- 1.3. Décomposition Géographique (Hiérarchie : Lieu -> Ville -> Région)

CREATE TABLE Region (
    id_region       INTEGER PRIMARY KEY,
    nom_region      VARCHAR(50) UNIQUE NOT NULL
);

CREATE TABLE Ville (
    id_ville        INTEGER PRIMARY KEY,
    id_region       INTEGER NOT NULL REFERENCES Region(id_region),
    nom_ville       VARCHAR(50) NOT NULL
);

CREATE TABLE Lieu (
    id_lieu         INTEGER PRIMARY KEY,
    id_ville        INTEGER NOT NULL REFERENCES Ville(id_ville),
    nom_point_vente VARCHAR(100) NOT NULL,
    canal_de_vente  VARCHAR(50) NOT NULL 
);


-- ######################################################################
-- # 2. CRÉATION DE LA TABLE DE FAITS (Vente)
-- ######################################################################

CREATE TABLE Vente (
    -- Clé Primaire : Clé composite (Transaction)
    id_vente            BIGINT PRIMARY KEY, -- Clé technique si besoin de garantir l'unicité
    id_temps            INTEGER NOT NULL REFERENCES Temps(id_temps),
    id_produit          INTEGER NOT NULL REFERENCES Produit(id_produit),
    id_lieu             INTEGER NOT NULL REFERENCES Lieu(id_lieu),
    id_client           INTEGER NOT NULL REFERENCES Client(id_client),
    id_promotion        INTEGER NOT NULL REFERENCES Promotion(id_promotion),
    
    -- Mesures
    montant_ca_net      DECIMAL(10, 2) NOT NULL,
    quantite_vendue     INTEGER NOT NULL,
    nombre_transactions INTEGER NOT NULL DEFAULT 1,

    -- Indexation
    UNIQUE (id_temps, id_produit, id_lieu, id_client)
);


-- ######################################################################
-- # 3. EXEMPLES DE REQUÊTES DÉCISIONNELLES (Adaptées au Flocon)
-- ######################################################################

-- Q.a : CA Net par Région, Mois et Produit
-- Nécessite 6 JOINTURES (Vente -> Temps -> Mois ; Vente -> Lieu -> Ville -> Région)
SELECT
    R.nom_region, M.mois, P.nom_produit,
    SUM(V.montant_ca_net) AS ca_net_total
FROM Vente V
JOIN Temps T ON V.id_temps = T.id_temps
JOIN Mois M ON T.id_mois = M.id_mois -- Niveau d'agrégation requis
JOIN Produit P ON V.id_produit = P.id_produit
JOIN Lieu L ON V.id_lieu = L.id_lieu
JOIN Ville VL ON L.id_ville = VL.id_ville
JOIN Region R ON VL.id_region = R.id_region -- Niveau d'agrégation requis
GROUP BY 1, 2, 3
ORDER BY ca_net_total DESC;

-- Q.b : Volume Total Vendu par Catégorie
-- Nécessite 2 JOINTURES (Vente -> Produit -> Catégorie)
SELECT
    C.nom_categorie,
    SUM(V.quantite_vendue) AS volume_total_vendu
FROM Vente V
JOIN Produit P ON V.id_produit = P.id_produit
JOIN Categorie C ON P.id_categorie = C.id_categorie -- Niveau d'agrégation requis
GROUP BY 1
ORDER BY volume_total_vendu DESC;

-- Q.f : Promotions qui rapportent le plus (CA Net Attribué)
-- Nécessite 1 JOINTURE (Vente -> Promotion)
SELECT
    PR.nom_promo,
    SUM(V.montant_ca_net) AS ca_net_attribue
FROM Vente V
JOIN Promotion PR ON V.id_promotion = PR.id_promotion
GROUP BY 1
ORDER BY ca_net_attribue DESC;

-- Q.i : Panier Moyen selon le Canal de Vente
-- Nécessite 1 JOINTURES (Vente -> Lieu)
SELECT
    L.canal_de_vente,
    SUM(V.montant_ca_net) / SUM(V.nombre_transactions) AS panier_moyen_net
FROM Vente V
JOIN Lieu L ON V.id_lieu = L.id_lieu -- Contient l'attribut Canal_de_Vente
GROUP BY 1
ORDER BY panier_moyen_net DESC;
