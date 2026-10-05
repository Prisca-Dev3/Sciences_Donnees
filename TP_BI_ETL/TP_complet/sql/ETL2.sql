USE vente_DW;

-- ==========================================================
-- ÉTAPE 1 : LES DIMENSIONS PARENTS
-- ==========================================================

-- 1.1 Remplir les Régions (depuis vente_op)
INSERT INTO Dim_Region (Libelle_Region)
SELECT DISTINCT Region 
FROM vente_db.vente_op;

-- 1.2 Remplir les Catégories
INSERT INTO Dim_Categorie (Libelle_Categorie)
SELECT DISTINCT Categorie 
FROM vente_db.vente_op;

-- 1.3 Remplir les Types de Client
INSERT INTO Dim_TypeClient (Libelle_Type)
SELECT DISTINCT TypeClient
FROM vente_db.vente_op;

-- 1.4 Remplir le Canal de vente (Dimension simple)
INSERT INTO Dim_Canal (Mode_Commande)
SELECT DISTINCT ModeCommande
FROM vente_db.vente_op;

-- 1.5 Remplir la Dimension Promotion (Nouvelle dimension)
INSERT INTO Dim_Promotion (Nom_Promotion, Est_Promotion_Active)
SELECT DISTINCT NomPromo, Promotion
FROM vente_db.vente_op;

-- ==========================================================
-- ÉTAPE 2 : LES DIMENSIONS ENFANTS 
-- ==========================================================

-- 2.1 Remplir les Villes (On joint avec Region pour récupérer l'ID_Region généré juste avant)
INSERT INTO Dim_Ville (Nom_Ville, ID_Region)
SELECT DISTINCT op.Ville, r.ID_Region
FROM vente_db.vente_op op
JOIN Dim_Region r ON op.Region = r.Libelle_Region;

-- 2.2 Remplir les Points de Vente (On joint avec Ville)
INSERT INTO Dim_PointVente (Nom_PointVente, ID_Ville)
SELECT DISTINCT op.PointVente, v.ID_Ville
FROM vente_db.vente_op op
JOIN Dim_Ville v ON op.Ville = v.Nom_Ville;

-- 2.3 Remplir les Produits (On joint avec Categorie)
INSERT INTO Dim_Produit (Nom_Produit, ID_Categorie)
SELECT DISTINCT op.Produit, c.ID_Categorie
FROM vente_db.vente_op op
JOIN Dim_Categorie c ON op.Categorie = c.Libelle_Categorie;

-- 2.4 Remplir les Clients (On joint avec TypeClient)
INSERT INTO Dim_Client (Nom_Client, ID_TypeClient)
SELECT DISTINCT op.NomClient, t.ID_TypeClient
FROM vente_db.vente_op op
JOIN Dim_TypeClient t ON op.TypeClient = t.Libelle_Type;

-- ==========================================================
-- ÉTAPE 3 : LA DIMENSION TEMPS
-- ==========================================================
INSERT INTO Dim_Temps (Date_ID, Annee, Mois, Mois_Nom, Trimestre)
SELECT DISTINCT 
    DateVente,
    YEAR(DateVente),
    MONTH(DateVente),
    MONTHNAME(DateVente),
    QUARTER(DateVente)
FROM vente_db.vente_op;

-- ==========================================================
-- ÉTAPE 4 : LA TABLE DE FAITS 
-- ==========================================================

INSERT INTO Fait_Ventes (
    Num_Ticket_Source, Date_ID, ID_Produit, ID_PointVente, ID_Client, ID_Canal, ID_Promotion,
    Quantite, Prix_Unitaire, Cout_Achat, Montant_Vente, Marge, Taux_Promo_Applique
)
SELECT 
    op.ID_Vente,
    op.DateVente,               
    p.ID_Produit,               
    pv.ID_PointVente,           
    c.ID_Client,                
    cn.ID_Canal,
    prm.ID_Promotion,
    op.Quantite,
    op.PrixUnitaire,
    op.CoutAchat,
    (op.Quantite * op.PrixUnitaire),                
    ((op.PrixUnitaire - op.CoutAchat) * op.Quantite), 
    op.TauxPromo
FROM vente_db.vente_op op
    JOIN Dim_Produit p ON op.Produit = p.Nom_Produit
    JOIN Dim_PointVente pv ON op.PointVente = pv.Nom_PointVente
    JOIN Dim_Client c ON op.NomClient = c.Nom_Client
    JOIN Dim_Canal cn ON op.ModeCommande = cn.Mode_Commande
    JOIN Dim_Promotion prm ON op.NomPromo = prm.Nom_Promotion;