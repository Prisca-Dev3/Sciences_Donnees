-- Nom du Fichier : entrepôt_location_vehicules.sql
-- Description : Implémentation complète d'un entrepôt de données (Modèle Constellation)
--               pour l'analyse de la performance, logistique et risque dans la location de véhicules.
-- KAMDOM NGUETCHESSI MERVEILLE PRISCA - 23V2060

-- ######################################################################
-- # 1. CRÉATION DES TABLES DE DIMENSIONS (DIM)
-- ######################################################################

-- Dim_Temps : Chronologie pour l'analyse 
CREATE TABLE dim_temps (
    id_temps        INTEGER PRIMARY KEY,
    annee           SMALLINT NOT NULL,
    mois            SMALLINT NOT NULL,
    semaine         SMALLINT NOT NULL,
    trimestre       SMALLINT NOT NULL,
    jour_semaine    VARCHAR(10)
);

-- Dim_Agence : Organisation et Géographie (Jeu de Rôles : Départ/Retour)
CREATE TABLE dim_agence (
    id_agence           INTEGER PRIMARY KEY,
    nom_agence          VARCHAR(100) NOT NULL,
    societe_nationale   VARCHAR(50) NOT NULL,
    pays                VARCHAR(50) NOT NULL,
    ville               VARCHAR(50) NOT NULL,
    region              VARCHAR(50)
);

-- Dim_Vehicule : Description du Parc (Clé de substitution "id_vehicule" pour la performance)
CREATE TABLE dim_vehicule (
    id_vehicule             INTEGER PRIMARY KEY,
    immatriculation         VARCHAR(20) UNIQUE NOT NULL, -- Clé naturelle (Attribut descriptif)
    categorie               VARCHAR(50) NOT NULL,
    marque                  VARCHAR(50) NOT NULL,
    modele                  VARCHAR(50) NOT NULL,
    annee_mise_en_service   SMALLINT
);

-- Dim_Client : Qui sont les utilisateurs?
CREATE TABLE dim_client (
    id_client           INTEGER PRIMARY KEY,
    prenom              VARCHAR(100),
    nom                 VARCHAR(100),
    segment_client      VARCHAR(50) NOT NULL,
    age                 SMALLINT,
    nationalite         VARCHAR(50)
);

-- Dim_Tarif : Comment la location est facturée ?
CREATE TABLE dim_tarif (
    id_tarif            INTEGER PRIMARY KEY,
    nom_tarif           VARCHAR(50) NOT NULL,
    type_facturation    VARCHAR(20) NOT NULL
);

-- Dim_Promotion : Offres et Partenariats (Axe d'analyse Q4)
CREATE TABLE dim_promotion (
    id_promotion    INTEGER PRIMARY KEY,
    nom_promo       VARCHAR(100) NOT NULL,
    partenaire      VARCHAR(100) NOT NULL,
    type_offre      VARCHAR(50) NOT NULL,
    date_debut      DATE,
    date_fin        DATE
);

-- Dim_Canal : D'où viennent les réservations (Axe d'analyse Q1)
CREATE TABLE dim_canal (
    id_canal    INTEGER PRIMARY KEY,
    nom_canal   VARCHAR(50) UNIQUE NOT NULL,
    description VARCHAR(255)
);


-- ######################################################################
-- # 2. CRÉATION DES TABLES DE FAITS 
-- ######################################################################

-- Location (Faits de Revenu et Utilisation)
-- Utilise le jeu de rôles temporel et géographique
CREATE TABLE location (
    id_location         BIGINT PRIMARY KEY, -- Clé de substitution technique
    
    -- Clés Étrangères (Jeu de Rôles)
    id_temps_depart     INTEGER NOT NULL REFERENCES dim_temps(id_temps),
    id_temps_arrive     INTEGER NOT NULL REFERENCES dim_temps(id_temps), -- Essentiel pour analyse du CA par période de clôture
    id_agence_depart    INTEGER NOT NULL REFERENCES dim_agence(id_agence),
    id_agence_retour    INTEGER NOT NULL REFERENCES dim_agence(id_agence), -- Essentiel pour analyse des flux de flotte (Q2)
    
    -- Autres Clés
    id_vehicule         INTEGER NOT NULL REFERENCES dim_vehicule(id_vehicule),
    id_client           INTEGER NOT NULL REFERENCES dim_client(id_client),
    id_tarif            INTEGER NOT NULL REFERENCES dim_tarif(id_tarif),
    id_promotion        INTEGER NOT NULL REFERENCES dim_promotion(id_promotion),
    id_canal            INTEGER NOT NULL REFERENCES dim_canal(id_canal), 
    
    -- Mesures
    montant_ca_ht               DECIMAL(10, 2) NOT NULL,
    nombre_locations            INTEGER NOT NULL DEFAULT 1,
    kilometres_parcourus        INTEGER NOT NULL,
    duree_location_jours        DECIMAL(5, 2) NOT NULL,

    UNIQUE (id_temps_depart, id_vehicule, id_client)
);


-- Incident (Faits de Coûts et Risques)
CREATE TABLE incident (
    id_incident                 BIGINT PRIMARY KEY, -- Clé de substitution technique
    
    -- Clés Étrangères (Événement ponctuel)
    id_temps                    INTEGER NOT NULL REFERENCES dim_temps(id_temps), -- Date de l'incident
    id_vehicule                 INTEGER NOT NULL REFERENCES dim_vehicule(id_vehicule),
    id_client                   INTEGER NOT NULL REFERENCES dim_client(id_client),
    id_agence                   INTEGER NOT NULL REFERENCES dim_agence(id_agence), -- Agence de déclaration
    
    -- Mesures
    nombre_incidents            INTEGER NOT NULL DEFAULT 1,
    type_incident               VARCHAR(50) NOT NULL, 
    cout_reparation             DECIMAL(10, 2) NOT NULL,
    impact_disponibilite_jours  DECIMAL(5, 2) NOT NULL,

    UNIQUE (id_temps, id_vehicule, id_client)
);


-- ######################################################################
-- # 3. EXEMPLES DE REQUÊTES DÉCISIONNELLES (Q1 à Q4)
-- ######################################################################

-- Q1 : Performance Commerciale (CA généré par Catégorie, Société, Canal au T4)
SELECT
    A.societe_nationale, V.categorie, C.nom_canal,
    SUM(L.montant_ca_ht) AS ca_total_ht,
    COUNT(L.id_location) AS nombre_locations
FROM location L
JOIN dim_temps T ON L.id_temps_arrive = T.id_temps 
JOIN dim_agence A ON L.id_agence_depart = A.id_agence
JOIN dim_vehicule V ON L.id_vehicule = V.id_vehicule
JOIN dim_canal C ON L.id_canal = C.id_canal
WHERE T.annee = 2025 AND T.trimestre = 4
GROUP BY 1, 2, 3 ORDER BY ca_total_ht DESC;

-- Q2 : Optimisation du Parc (Déséquilibre de Flotte : locations one-way)
SELECT
    AD.pays AS pays_depart, AR.pays AS pays_retour, V.categorie,
    COUNT(L.id_location) AS nb_locations_one_way
FROM location L
JOIN dim_agence AD ON L.id_agence_depart = AD.id_agence
JOIN dim_agence AR ON L.id_agence_retour = AR.id_agence
JOIN dim_vehicule V ON L.id_vehicule = V.id_vehicule
WHERE L.id_agence_depart <> L.id_agence_retour 
GROUP BY 1, 2, 3 ORDER BY nb_locations_one_way DESC;

-- Q3 : Gestion des Risques/Coûts (Coût moyen des incidents par catégorie et segment client)
SELECT
    V.categorie, CL.segment_client,
    COUNT(I.id_incident) AS nombre_incidents,
    AVG(I.cout_reparation) AS cout_moyen_incident
FROM incident I
JOIN dim_vehicule V ON I.id_vehicule = V.id_vehicule
JOIN dim_client CL ON I.id_client = CL.id_client
GROUP BY 1, 2 ORDER BY cout_moyen_incident DESC;

-- Q4 : Efficacité des Promotions et Partenariats (CA et durée moyenne des offres groupées)
SELECT
    P.partenaire, P.type_offre,
    SUM(L.montant_ca_ht) AS ca_partenariat,
    AVG(L.duree_location_jours) AS duree_moyenne_location
FROM location L
JOIN dim_promotion P ON L.id_promotion = P.id_promotion
GROUP BY 1, 2 ORDER BY ca_partenariat DESC;
