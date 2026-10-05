CREATE DATABASE vente_db;
USE vente_db;

CREATE TABLE vente_op (
    ID_Vente INT PRIMARY KEY,
    DateVente DATE,
    NomClient VARCHAR(100),
    TypeClient VARCHAR(50),
    Produit VARCHAR(100),
    Categorie VARCHAR(50),
    PrixUnitaire DECIMAL(10, 2),
    Quantite INT,
    CoutAchat DECIMAL(10, 2),
    Promotion VARCHAR(5), 
    NomPromo VARCHAR(100),
    TauxPromo DECIMAL(5, 2), 
    PointVente VARCHAR(100),
    Ville VARCHAR(50),
    Region VARCHAR(50),
    ModeCommande VARCHAR(50)
);
