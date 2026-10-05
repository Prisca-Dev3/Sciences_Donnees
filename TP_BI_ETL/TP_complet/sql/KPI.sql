USE vente_DW;

-- KPI 1 : Chiffre d'Affaires, Marge et Taux de Marge Global
SELECT
SUM(Montant_Vente) AS CA_Total,
SUM(Marge) AS Marge_Totale,
(SUM(Marge) / SUM(Montant_Vente)) * 100 AS Taux_Marge_Global_Pourcentage
FROM Fait_Ventes;

-- KPI 2 : Évolution du CA par Année et par Trimestre
SELECT
t.Annee,
t.Trimestre,
SUM(f.Montant_Vente) AS CA,
SUM(f.Marge) AS Marge
FROM Fait_Ventes f
JOIN Dim_Temps t ON f.Date_ID = t.Date_ID
GROUP BY t.Annee, t.Trimestre
ORDER BY t.Annee, t.Trimestre;

-- KPI 3 : Top 3 des Régions les plus performantes
SELECT
r.Libelle_Region,
SUM(f.Montant_Vente) AS CA_Region,
SUM(f.Marge) AS Marge_Region
FROM Fait_Ventes f
JOIN Dim_PointVente pv ON f.ID_PointVente = pv.ID_PointVente
JOIN Dim_Ville v ON pv.ID_Ville = v.ID_Ville
JOIN Dim_Region r ON v.ID_Region = r.ID_Region
GROUP BY r.Libelle_Region
ORDER BY CA_Region DESC
LIMIT 3;

-- KPI 4 : Le Panier Moyen (Montant moyen par vente)
SELECT
AVG(Montant_Vente) AS Panier_Moyen_Global
FROM Fait_Ventes;

-- KPI 5 : Performance par Canal de Vente
SELECT
c.Mode_Commande,
COUNT(DISTINCT f.Num_Ticket_Source) AS Nombre_Ventes,
SUM(f.Montant_Vente) AS CA_Total,
AVG(f.Montant_Vente) AS Panier_Moyen_Canal
FROM Fait_Ventes f
JOIN Dim_Canal c ON f.ID_Canal = c.ID_Canal
GROUP BY c.Mode_Commande
ORDER BY CA_Total DESC;

-- KPI 6 : Top 5 des Produits les plus vendus
SELECT
p.Nom_Produit,
cat.Libelle_Categorie,
SUM(f.Quantite) AS Volume_Vendu,
SUM(f.Montant_Vente) AS CA_Produit
FROM Fait_Ventes f
JOIN Dim_Produit p ON f.ID_Produit = p.ID_Produit
JOIN Dim_Categorie cat ON p.ID_Categorie = cat.ID_Categorie
GROUP BY p.Nom_Produit, cat.Libelle_Categorie
ORDER BY Volume_Vendu DESC
LIMIT 5;

-- KPI 7 : Comparaison Ventes "En Promo" vs "Sans Promo"
SELECT
CASE
WHEN f.Taux_Promo_Applique > 0 THEN 'En Promotion'
ELSE 'Prix Standard'
END AS Statut_Vente,
COUNT(DISTINCT f.Num_Ticket_Source) AS Nombre_Ventes,
SUM(f.Montant_Vente) AS CA_Genere,
SUM(f.Marge) AS Marge_Generee
FROM Fait_Ventes f
GROUP BY Statut_Vente;

-- KPI 8 : Rentabilité par Taux de Promotion
SELECT
CONCAT(f.Taux_Promo_Applique, '%') AS Promo_Appliquee,
SUM(f.Quantite) AS Volume_Vendu,
SUM(f.Marge) AS Marge_Totale
FROM Fait_Ventes f
WHERE f.Taux_Promo_Applique > 0
GROUP BY f.Taux_Promo_Applique
ORDER BY f.Taux_Promo_Applique ASC;
