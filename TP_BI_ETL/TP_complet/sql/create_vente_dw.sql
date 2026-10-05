CREATE DATABASE vente_DW;
USE vente_DW;

-- =============================================
-- 1. Branche PRODUIT (Structure Flocon conservée)
-- =============================================
CREATE TABLE Dim_Categorie (
    ID_Categorie INT AUTO_INCREMENT PRIMARY KEY,
    Libelle_Categorie VARCHAR(50) UNIQUE
);

CREATE TABLE Dim_Produit (
    ID_Produit INT AUTO_INCREMENT PRIMARY KEY,
    Nom_Produit VARCHAR(100),
    ID_Categorie INT,
    FOREIGN KEY (ID_Categorie) REFERENCES Dim_Categorie(ID_Categorie)
);

-- =============================================
-- 2. Branche GEOGRAPHIE (Structure Flocon conservée)
-- =============================================
CREATE TABLE Dim_Region (
    ID_Region INT AUTO_INCREMENT PRIMARY KEY,
    Libelle_Region VARCHAR(50) UNIQUE
);

CREATE TABLE Dim_Ville (
    ID_Ville INT AUTO_INCREMENT PRIMARY KEY,
    Nom_Ville VARCHAR(50),
    ID_Region INT,
    FOREIGN KEY (ID_Region) REFERENCES Dim_Region(ID_Region)
);

CREATE TABLE Dim_PointVente (
    ID_PointVente INT AUTO_INCREMENT PRIMARY KEY,
    Nom_PointVente VARCHAR(100),
    ID_Ville INT,
    FOREIGN KEY (ID_Ville) REFERENCES Dim_Ville(ID_Ville)
);

-- =============================================
-- 3. Branche CLIENT
-- =============================================
CREATE TABLE Dim_TypeClient (
    ID_TypeClient INT AUTO_INCREMENT PRIMARY KEY,
    Libelle_Type VARCHAR(50) UNIQUE
);

CREATE TABLE Dim_Client (
    ID_Client INT AUTO_INCREMENT PRIMARY KEY,
    Nom_Client VARCHAR(100),
    ID_TypeClient INT,
    FOREIGN KEY (ID_TypeClient) REFERENCES Dim_TypeClient(ID_TypeClient)
);

-- =============================================
-- 4. Autres Dimensions
-- =============================================
CREATE TABLE Dim_Temps (
    Date_ID DATE PRIMARY KEY,
    Annee INT,
    Mois INT,
    Mois_Nom VARCHAR(20),
    Trimestre INT
);

CREATE TABLE Dim_Canal (
    ID_Canal INT AUTO_INCREMENT PRIMARY KEY,
    Mode_Commande VARCHAR(50) UNIQUE
);

-- [NOUVEAU] Ajout pour répondre au KPI (f)
-- Permet d'analyser l'efficacité par "NomPromo" (ex: St Valentin vs Rentrée)
CREATE TABLE Dim_Promotion (
    ID_Promotion INT AUTO_INCREMENT PRIMARY KEY,
    Nom_Promotion VARCHAR(100), -- Ex: 'St Valentin', 'Janvier Free'
    Est_Promotion_Active VARCHAR(3) -- Ex: 'Oui', 'Non' (selon la source)
);

-- =============================================
-- 5. TABLE DE FAITS (Modifiée pour KPI i et f)
-- =============================================
CREATE TABLE Fait_Ventes (
    ID_Ligne_Vente INT AUTO_INCREMENT PRIMARY KEY, -- Clé technique unique par ligne
    
    -- Dimension Dégénérée pour le KPI (i) : Panier Moyen
    Num_Ticket_Source VARCHAR(50), -- Correspond à l'ID Vente du fichier source (pour regrouper les produits d'un même achat)
    
    -- Clés étrangères vers les dimensions
    Date_ID DATE,
    ID_Produit INT,
    ID_PointVente INT,
    ID_Client INT,
    ID_Canal INT,
    ID_Promotion INT, -- [NOUVEAU] Lien vers la dimension promotion
    
    -- Mesures
    Quantite INT,
    Prix_Unitaire DECIMAL(10,2),
    Cout_Achat DECIMAL(10,2),
    Montant_Vente DECIMAL(10,2), -- (Prix * Qte) - Remise
    Marge DECIMAL(10,2),         -- Montant_Vente - (Cout_Achat * Qte)
    Taux_Promo_Applique DECIMAL(5,2), -- On garde le taux ici car il peut varier pour une même promo
    
    -- Contraintes
    FOREIGN KEY (Date_ID) REFERENCES Dim_Temps(Date_ID),
    FOREIGN KEY (ID_Produit) REFERENCES Dim_Produit(ID_Produit),
    FOREIGN KEY (ID_PointVente) REFERENCES Dim_PointVente(ID_PointVente),
    FOREIGN KEY (ID_Client) REFERENCES Dim_Client(ID_Client),
    FOREIGN KEY (ID_Canal) REFERENCES Dim_Canal(ID_Canal),
    FOREIGN KEY (ID_Promotion) REFERENCES Dim_Promotion(ID_Promotion)
);