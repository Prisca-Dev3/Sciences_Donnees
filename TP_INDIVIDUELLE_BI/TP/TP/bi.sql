-- ######################################################################
-- # TRAVAIL À FAIRE - POINT 1 : CRÉATION DE LA BASE TAMPON
-- ######################################################################
DROP DATABASE IF EXISTS vente_DB;
CREATE DATABASE vente_DB;
USE vente_DB;

CREATE TABLE vente_op (
    ID_Vente INT,
    DateVente DATE,
    NomClient VARCHAR(255),
    TypeClient VARCHAR(100),
    Produit VARCHAR(255),
    Categorie VARCHAR(100),
    PrixUnitaire DECIMAL(15,2),
    Quantite INT,
    CoutAchat DECIMAL(15,2),
    Promotion VARCHAR(10),
    NomPromo VARCHAR(100),
    TauxPromo VARCHAR(50), 
    PointVente VARCHAR(255),
    Ville VARCHAR(100),
    Region VARCHAR(100),
    ModeCommande VARCHAR(100)
);



-- ######################################################################
-- # TRAVAIL À FAIRE - POINT 2 : CRÉATION DU DW (VERSION FLOCON AJUSTÉE)
-- ######################################################################

DROP DATABASE IF EXISTS vente_DW;
CREATE DATABASE vente_DW;
USE vente_DW;

-- 1. HIÉRARCHIE TEMPS (Flocon : Temps -> Mois -> Trimestre -> Année)
CREATE TABLE Annee (
    id_annee INT PRIMARY KEY,
    annee INT NOT NULL
);

CREATE TABLE Trimestre (
    id_trimestre INT PRIMARY KEY,
    id_annee INT,
    trimestre INT NOT NULL,
    FOREIGN KEY (id_annee) REFERENCES Annee(id_annee)
);

CREATE TABLE Mois (
    id_mois INT PRIMARY KEY,
    id_trimestre INT,
    mois INT NOT NULL,
    FOREIGN KEY (id_trimestre) REFERENCES Trimestre(id_trimestre)
);

CREATE TABLE Temps (
    id_temps INT PRIMARY KEY, -- Format AAAAMMDD
    id_mois INT,
    jour INT NOT NULL,
    date_complete DATE NOT NULL,
    FOREIGN KEY (id_mois) REFERENCES Mois(id_mois)
);

-- 2. HIÉRARCHIE GÉOGRAPHIQUE ET CANAL (Flocon : Lieu -> Ville -> Region )
CREATE TABLE Region (
    id_region INT AUTO_INCREMENT PRIMARY KEY,
    nom_region VARCHAR(100) UNIQUE NOT NULL
);

CREATE TABLE Ville (
    id_ville INT AUTO_INCREMENT PRIMARY KEY,
    id_region INT,
    nom_ville VARCHAR(100) NOT NULL,
    FOREIGN KEY (id_region) REFERENCES Region(id_region)
);

CREATE TABLE Lieu (
    id_lieu INT AUTO_INCREMENT PRIMARY KEY,
    id_ville INT,
    nom_point_vente VARCHAR(100) NOT NULL,
    mode_commande VARCHAR(100), 
    FOREIGN KEY (id_ville) REFERENCES Ville(id_ville)
);

-- 3. HIÉRARCHIE PRODUIT (Flocon : Produit -> Categorie)
CREATE TABLE Categorie (
    id_categorie INT AUTO_INCREMENT PRIMARY KEY,
    nom_categorie VARCHAR(100) UNIQUE NOT NULL
);

CREATE TABLE Produit (
    id_produit INT AUTO_INCREMENT PRIMARY KEY,
    id_categorie INT,
    nom_produit VARCHAR(255) NOT NULL,
    FOREIGN KEY (id_categorie) REFERENCES Categorie(id_categorie)
);

-- 4. DIMENSIONS CLIENT ET PROMOTION
CREATE TABLE Client (
    id_client INT AUTO_INCREMENT PRIMARY KEY,
    nom_client VARCHAR(255) NOT NULL,
    type_client VARCHAR(100)
);

CREATE TABLE Promotion (
    id_promotion INT AUTO_INCREMENT PRIMARY KEY,
    nom_promo VARCHAR(100) NOT NULL,
    taux_promo DECIMAL(5,2) DEFAULT 0
);

-- 5. TABLE DE FAITS (CENTRE DU FLOCON)
CREATE TABLE Vente (
    id_vente_pk INT AUTO_INCREMENT PRIMARY KEY,
    id_temps INT,
    id_produit INT,
    id_lieu INT,
    id_client INT,
    id_promotion INT,
    
    -- Mesures 
    prix_unitaire DECIMAL(15,2),
    quantite_vendue INT,
    cout_achat_total DECIMAL(15,2),   
    montant_ca_net DECIMAL(15,2),     -- CA Net après remise (calculé)
    marge_brute DECIMAL(15,2),        -- (CA Net - Cout Achat total)
    nombre_transactions INT DEFAULT 1, 
    
    FOREIGN KEY (id_temps) REFERENCES Temps(id_temps),
    FOREIGN KEY (id_produit) REFERENCES Produit(id_produit),
    FOREIGN KEY (id_lieu) REFERENCES Lieu(id_lieu),
    FOREIGN KEY (id_client) REFERENCES Client(id_client),
    FOREIGN KEY (id_promotion) REFERENCES Promotion(id_promotion)
);



-- ######################################################################
-- # TRAVAIL À FAIRE - POINT 4 : ETL SQL COMPLET (TRANSFERT DE vente_DB VERS vente_DW)
-- ######################################################################

-- On se positionne dans l'entrepôt
USE vente_DW;

-- 1. VIDAGE PRÉALABLE (Optionnel, pour éviter les doublons si nous relancons)
-- ----------------------------------------------------------------------

-- 1.1 On désactive les contraintes
SET FOREIGN_KEY_CHECKS = 0;

-- 1.2. On vide les tables avec DELETE (plus permissif que TRUNCATE)
DELETE FROM Vente; 
DELETE FROM Temps; 
DELETE FROM Mois; 
DELETE FROM Trimestre; 
DELETE FROM Annee;
DELETE FROM Produit; 
DELETE FROM Categorie;
DELETE FROM Lieu; 
DELETE FROM Ville; 
DELETE FROM Region; 
DELETE FROM Client; 
DELETE FROM Promotion;

-- 1.3. On remet les compteurs d'Auto-Incrément à 1 (pour repartir de zéro)
ALTER TABLE Region AUTO_INCREMENT = 1;
ALTER TABLE Ville AUTO_INCREMENT = 1;
ALTER TABLE Lieu AUTO_INCREMENT = 1;
ALTER TABLE Categorie AUTO_INCREMENT = 1;
ALTER TABLE Produit AUTO_INCREMENT = 1;
ALTER TABLE Client AUTO_INCREMENT = 1;
ALTER TABLE Promotion AUTO_INCREMENT = 1;
ALTER TABLE Vente AUTO_INCREMENT = 1;

-- 1.4. On réactive les contraintes
SET FOREIGN_KEY_CHECKS = 1;


-- 2. ALIMENTATION DES DIMENSIONS (HIÉRARCHIES)
-- ----------------------------------------------------------------------

-- 2.1.  Dimension Géographique (Flocon)
INSERT INTO Region (nom_region) 
SELECT DISTINCT Region FROM vente_DB.vente_op;

INSERT INTO Ville (nom_ville, id_region)
SELECT DISTINCT op.Ville, r.id_region 
FROM vente_DB.vente_op op 
JOIN Region r ON op.Region = r.nom_region;

INSERT INTO Lieu (nom_point_vente, id_ville, mode_commande)
SELECT DISTINCT op.PointVente, v.id_ville, op.ModeCommande 
FROM vente_DB.vente_op op 
JOIN Ville v ON op.Ville = v.nom_ville;

-- 2.2. Dimension Produit (Flocon)
INSERT INTO Categorie (nom_categorie) 
SELECT DISTINCT Categorie FROM vente_DB.vente_op;

INSERT INTO Produit (nom_produit, id_categorie)
SELECT DISTINCT op.Produit, c.id_categorie 
FROM vente_DB.vente_op op 
JOIN Categorie c ON op.Categorie = c.nom_categorie;

-- 2.3. Dimensions Simples
INSERT INTO Client (nom_client, type_client) 
SELECT DISTINCT NomClient, TypeClient FROM vente_DB.vente_op;

INSERT INTO Promotion (nom_promo, taux_promo) 
SELECT DISTINCT NomPromo, CAST(TauxPromo AS DECIMAL(5,2)) FROM vente_DB.vente_op;


-- 3. ALIMENTATION DE LA HIÉRARCHIE TEMPS
-- ----------------------------------------------------------------------
INSERT INTO Annee (id_annee, annee) 
SELECT DISTINCT YEAR(DateVente), YEAR(DateVente) FROM vente_DB.vente_op;

INSERT INTO Trimestre (id_trimestre, id_annee, trimestre)
SELECT DISTINCT (YEAR(DateVente)*10 + QUARTER(DateVente)), YEAR(DateVente), QUARTER(DateVente) FROM vente_DB.vente_op;

INSERT INTO Mois (id_mois, id_trimestre, mois)
SELECT DISTINCT (YEAR(DateVente)*100 + MONTH(DateVente)), (YEAR(DateVente)*10 + QUARTER(DateVente)), MONTH(DateVente) FROM vente_DB.vente_op;

INSERT INTO Temps (id_temps, id_mois, jour, date_complete)
SELECT DISTINCT CAST(REPLACE(DateVente, '-', '') AS UNSIGNED), (YEAR(DateVente)*100 + MONTH(DateVente)), DAY(DateVente), DateVente FROM vente_DB.vente_op;


-- 4. ALIMENTATION DE LA TABLE DE FAITS (CALCUL DES KPIs)
-- ----------------------------------------------------------------------

--  On prépare le chargement rapide
SET FOREIGN_KEY_CHECKS = 0;
SET UNIQUE_CHECKS = 0;
SET AUTOCOMMIT = 0;

--  La grande insertion
INSERT INTO Vente (
    id_temps, id_produit, id_lieu, id_client, id_promotion, 
    prix_unitaire, quantite_vendue, cout_achat_total, 
    montant_ca_net, marge_brute, nombre_transactions
)
SELECT 
    CAST(REPLACE(op.DateVente, '-', '') AS UNSIGNED),
    p.id_produit, l.id_lieu, cl.id_client, pr.id_promotion,
    op.PrixUnitaire, op.Quantite, (op.CoutAchat * op.Quantite),
    (op.PrixUnitaire * op.Quantite * (1 - CAST(op.TauxPromo AS DECIMAL(10,2))/100)),
    ((op.PrixUnitaire * op.Quantite * (1 - CAST(op.TauxPromo AS DECIMAL(10,2))/100)) - (op.CoutAchat * op.Quantite)),
    1 
FROM vente_DB.vente_op op
JOIN Produit p ON op.Produit = p.nom_produit
JOIN Lieu l ON op.PointVente = l.nom_point_vente AND op.ModeCommande = l.mode_commande
JOIN Client cl ON op.NomClient = cl.nom_client AND op.TypeClient = cl.type_client
JOIN Promotion pr ON op.NomPromo = pr.nom_promo AND CAST(op.TauxPromo AS DECIMAL(5,2)) = pr.taux_promo;

-- On valide l'écriture sur le disque
COMMIT;

-- On remet tout en ordre
SET FOREIGN_KEY_CHECKS = 1;
SET UNIQUE_CHECKS = 1;
SET AUTOCOMMIT = 1;


-- ######################################################################
-- # TRAVAIL À FAIRE - POINT 5 : GESTION DES VERROUILLAGES EN CAS D'ERREUR
-- ######################################################################

-- 1. Le "Grand Déblocage" (À faire en premier dans la mesure où la table Vente est bloquée après plusieurs tentatives)

-- Exécutons ces commandes une par une. Elles vont forcer MySQL à oublier les transactions qui ont planté et qui verrouillent la table Vente.
SET FOREIGN_KEY_CHECKS = 0;
COMMIT;
-- Cette commande suivante montre les processus. 
-- Si vous voyez une ligne avec "Query" qui dure depuis longtemps, il faut la stopper (Cliquons sur le boutton kill). Ne pas toucher aux autres processus (System User)!
SHOW FULL PROCESSLIST; 
SET FOREIGN_KEY_CHECKS = 1;