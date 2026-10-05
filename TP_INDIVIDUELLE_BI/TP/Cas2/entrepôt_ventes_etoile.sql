-- Nom du Fichier : entrepôt_ventes_etoile.sql
-- Description : Implémentation du Modèle en Étoile pour le Cas 2 (Les Débrouillards).
-- KAMDOM NGUETCHESSI MERVEILLE PRISCA - 23V2060

-- ######################################################################
-- # 1. CRÉATION DES TABLES DE DIMENSIONS
-- ######################################################################

CREATE TABLE Dim_Temps (
    id_temps        INTEGER PRIMARY KEY,
    annee           SMALLINT NOT NULL,
    trimestre       SMALLINT NOT NULL,
    mois            SMALLINT NOT NULL,
    jour            SMALLINT NOT NULL
);

CREATE TABLE Dim_Produit (
    id_produit      INTEGER PRIMARY KEY,
    nom_produit     VARCHAR(100) NOT NULL,
    categorie       VARCHAR(50) NOT NULL 
);

CREATE TABLE Dim_Lieu (
    id_lieu         INTEGER PRIMARY KEY,
    nom_point_vente VARCHAR(100) NOT NULL,
    canal_de_vente  VARCHAR(50) NOT NULL,
    ville           VARCHAR(50) NOT NULL, 
    region          VARCHAR(50) NOT NULL  
);

CREATE TABLE Dim_Client (
    id_client       INTEGER PRIMARY KEY,
    nom_client      VARCHAR(100) NOT NULL,
    type_client     VARCHAR(50) NOT NULL
);

CREATE TABLE Dim_Promotion (
    id_promotion    INTEGER PRIMARY KEY,
    nom_promo       VARCHAR(100) NOT NULL,
    remise_pct      DECIMAL(5, 2)
);


-- ######################################################################
-- # 2. CRÉATION DE LA TABLE DE FAITS 
-- ######################################################################

CREATE TABLE Vente (
    -- Clé Primaire : Clé composite (Transaction)
    id_temps            INTEGER NOT NULL REFERENCES Dim_Temps(id_temps),
    id_produit          INTEGER NOT NULL REFERENCES Dim_Produit(id_produit),
    id_lieu             INTEGER NOT NULL REFERENCES Dim_Lieu(id_lieu),
    id_client           INTEGER NOT NULL REFERENCES Dim_Client(id_client),
    id_promotion        INTEGER NOT NULL REFERENCES Dim_Promotion(id_promotion),
    
    -- Mesures
    montant_ca_net      DECIMAL(10, 2) NOT NULL,
    quantite_vendue     INTEGER NOT NULL,
    nombre_transactions INTEGER NOT NULL DEFAULT 1,
    
    PRIMARY KEY (id_temps, id_produit, id_lieu, id_client, id_promotion)
);


-- ######################################################################
-- # 3. EXEMPLES DE REQUÊTES DÉCISIONNELLES 
-- ######################################################################

-- Q.a : CA Net par Région, Mois et Produit
SELECT
    L.region, T.mois, P.nom_produit,
    SUM(FV.montant_ca_net) AS ca_net_total
FROM Fact_Vente FV
JOIN Dim_Temps T ON FV.id_temps = T.id_temps
JOIN Dim_Produit P ON FV.id_produit = P.id_produit
JOIN Dim_Lieu L ON FV.id_lieu = L.id_lieu -- Jointure directe à Région
GROUP BY 1, 2, 3
ORDER BY ca_net_total DESC;

-- Q.b : Volume Total Vendu par Catégorie
SELECT
    P.categorie,
    SUM(FV.quantite_vendue) AS volume_total_vendu
FROM Fact_Vente FV
JOIN Dim_Produit P ON FV.id_produit = P.id_produit -- Catégorie est directement accessible
GROUP BY 1
ORDER BY volume_total_vendu DESC;
