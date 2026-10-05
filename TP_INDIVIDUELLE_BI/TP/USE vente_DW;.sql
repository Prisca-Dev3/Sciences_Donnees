USE vente_DW;

-- On remplit les tables indépendantes
INSERT INTO Region (nom_region) SELECT DISTINCT Region FROM vente_DB.vente_op;
INSERT INTO Categorie (nom_categorie) SELECT DISTINCT Categorie FROM vente_DB.vente_op;
INSERT INTO Client (nom_client, type_client) SELECT DISTINCT NomClient, TypeClient FROM vente_DB.vente_op;
INSERT INTO Promotion (nom_promo, taux_promo) SELECT DISTINCT NomPromo, CAST(TauxPromo AS DECIMAL(5,2)) FROM vente_DB.vente_op;
INSERT INTO Annee (id_annee, annee) SELECT DISTINCT YEAR(DateVente), YEAR(DateVente) FROM vente_DB.vente_op;

USE vente_DW;

-- Villes et Produits
INSERT INTO Ville (nom_ville, id_region)
SELECT DISTINCT op.Ville, r.id_region FROM vente_DB.vente_op op 
JOIN Region r ON op.Region = r.nom_region;

INSERT INTO Produit (nom_produit, id_categorie)
SELECT DISTINCT op.Produit, c.id_categorie FROM vente_DB.vente_op op 
JOIN Categorie c ON op.Categorie = c.nom_categorie;

-- Trimestres et Mois
INSERT INTO Trimestre (id_trimestre, id_annee, trimestre)
SELECT DISTINCT (YEAR(DateVente)*10 + QUARTER(DateVente)), YEAR(DateVente), QUARTER(DateVente) FROM vente_DB.vente_op;

INSERT INTO Mois (id_mois, id_trimestre, mois)
SELECT DISTINCT (YEAR(DateVente)*100 + MONTH(DateVente)), (YEAR(DateVente)*10 + QUARTER(DateVente)), MONTH(DateVente) FROM vente_DB.vente_op;

USE vente_DW;

INSERT INTO Lieu (nom_point_vente, id_ville, mode_commande)
SELECT DISTINCT op.PointVente, v.id_ville, op.ModeCommande FROM vente_DB.vente_op op 
JOIN Ville v ON op.Ville = v.nom_ville;

INSERT INTO Temps (id_temps, id_mois, jour, date_complete)
SELECT DISTINCT CAST(REPLACE(DateVente, '-', '') AS UNSIGNED), (YEAR(DateVente)*100 + MONTH(DateVente)), DAY(DateVente), DateVente FROM vente_DB.vente_op;