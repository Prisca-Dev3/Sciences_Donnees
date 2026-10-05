import pandas as pd
from sqlalchemy import create_engine

#  --- 1. PARAMÈTRES DE CONNEXION (À ajuster selon votre MySQL)
engine_op = create_engine('mysql+mysqlconnector://root:bisounous@localhost/vente_DB')
engine_dw = create_engine('mysql+mysqlconnector://root:bisounous@localhost/vente_DW')

# Charger la source (vente_op)
df = pd.read_sql("SELECT * FROM vente_op", engine_op)
df['DateVente'] = pd.to_datetime(df['DateVente'])

print(" Démarrage du processus ETL Python...")

# --- 2. ALIMENTATION DES DIMENSIONS (HIÉRARCHIES GÉO ET PRODUIT) ---

#  Lieu -> Ville -> Region
pd.DataFrame(df['Region'].unique(), columns=['nom_region']).to_sql('Region', engine_dw, if_exists='append', index=False)
db_reg = pd.read_sql("SELECT * FROM Region", engine_dw)

villes = df[['Ville', 'Region']].drop_duplicates().merge(db_reg, left_on='Region', right_on='nom_region')
villes[['Ville', 'id_region']].rename(columns={'Ville':'nom_ville'}).to_sql('Ville', engine_dw, if_exists='append', index=False)
db_ville = pd.read_sql("SELECT * FROM Ville", engine_dw)

lieux = df[['PointVente', 'ModeCommande', 'Ville']].drop_duplicates().merge(db_ville, left_on='Ville', right_on='nom_ville')
lieux[['PointVente', 'id_ville', 'ModeCommande']].rename(columns={'PointVente':'nom_point_vente', 'ModeCommande':'mode_commande'}).to_sql('Lieu', engine_dw, if_exists='append', index=False)

#  Produit -> Categorie
pd.DataFrame(df['Categorie'].unique(), columns=['nom_categorie']).to_sql('Categorie', engine_dw, if_exists='append', index=False)
db_cat = pd.read_sql("SELECT * FROM Categorie", engine_dw)

prods = df[['Produit', 'Categorie']].drop_duplicates().merge(db_cat, left_on='Categorie', right_on='nom_categorie')
prods[['Produit', 'id_categorie']].rename(columns={'Produit':'nom_produit'}).to_sql('Produit', engine_dw, if_exists='append', index=False)

# --- 3. DIMENSIONS SIMPLES (CLIENTS ET PROMOTIONS) ---

# Table Client
pd.DataFrame(df[['NomClient', 'TypeClient']].drop_duplicates()).rename(columns={'NomClient':'nom_client', 'TypeClient':'type_client'}).to_sql('Client', engine_dw, if_exists='append', index=False)

# Table Promotion (avec conversion stricte pour le futur merge)
promos = df[['NomPromo', 'TauxPromo']].drop_duplicates().copy()
promos['TauxPromo'] = pd.to_numeric(promos['TauxPromo'], errors='coerce').astype(float)
promos.rename(columns={'NomPromo':'nom_promo', 'TauxPromo':'taux_promo'}).to_sql('Promotion', engine_dw, if_exists='append', index=False)

# --- 4. HIÉRARCHIE TEMPS (TEMPS -> MOIS -> TRIMESTRE -> ANNEE) ---

# Année
annees = pd.DataFrame(df['DateVente'].dt.year.unique(), columns=['annee'])
annees['id_annee'] = annees['annee']
annees.to_sql('Annee', engine_dw, if_exists='append', index=False)

# Trimestre
trim = df[['DateVente']].copy()
trim['id_annee'] = trim['DateVente'].dt.year
trim['trimestre'] = trim['DateVente'].dt.quarter
trim['id_trimestre'] = (trim['id_annee'] * 10) + trim['trimestre']
trim[['id_trimestre', 'id_annee', 'trimestre']].drop_duplicates().to_sql('Trimestre', engine_dw, if_exists='append', index=False)

# Mois
mois = df[['DateVente']].copy()
mois['id_trimestre'] = (mois['DateVente'].dt.year * 10) + mois['DateVente'].dt.quarter
mois['mois'] = mois['DateVente'].dt.month
mois['id_mois'] = (mois['DateVente'].dt.year * 100) + mois['mois']
mois[['id_mois', 'id_trimestre', 'mois']].drop_duplicates().to_sql('Mois', engine_dw, if_exists='append', index=False)

# Temps (Date finale)
temps = df[['DateVente']].copy()
temps['id_mois'] = (temps['DateVente'].dt.year * 100) + temps['DateVente'].dt.month
temps['jour'] = temps['DateVente'].dt.day
temps['id_temps'] = temps['DateVente'].dt.strftime('%Y%m%d').astype(int)
temps[['id_temps', 'id_mois', 'jour', 'DateVente']].rename(columns={'DateVente':'date_complete'}).drop_duplicates().to_sql('Temps', engine_dw, if_exists='append', index=False)

# --- 5. TABLE DE FAITS (CALCUL DES KPIs) ---

print(" Calcul de la table de faits et des KPIs...")

# Récupération de TOUS les IDs générés pour le mappage
db_p = pd.read_sql("SELECT id_produit, nom_produit FROM Produit", engine_dw)
db_l = pd.read_sql("SELECT id_lieu, nom_point_vente, mode_commande FROM Lieu", engine_dw)
db_c = pd.read_sql("SELECT id_client, nom_client, type_client FROM Client", engine_dw)
db_pr = pd.read_sql("SELECT id_promotion, nom_promo, taux_promo FROM Promotion", engine_dw)

# Préparation des types pour la jointure (Le CAST SQL)
db_pr['taux_promo'] = db_pr['taux_promo'].astype(float)
df['TauxPromo_num'] = pd.to_numeric(df['TauxPromo'], errors='coerce').astype(float)

# Jointures (Merges successifs)
df_f = df.merge(db_p, left_on='Produit', right_on='nom_produit')
df_f = df_f.merge(db_l, left_on=['PointVente', 'ModeCommande'], right_on=['nom_point_vente', 'mode_commande'])
df_f = df_f.merge(db_c, left_on=['NomClient', 'TypeClient'], right_on=['nom_client', 'type_client'])
df_f = df_f.merge(db_pr, left_on=['NomPromo', 'TauxPromo_num'], right_on=['nom_promo', 'taux_promo'])

# Calculs finaux des mesures
df_f['id_temps'] = df_f['DateVente'].dt.strftime('%Y%m%d').astype(int)
df_f['cout_achat_total'] = df_f['CoutAchat'] * df_f['Quantite']
df_f['montant_ca_net'] = (df_f['PrixUnitaire'] * df_f['Quantite']) * (1 - df_f['taux_promo'].astype(float)/100)
df_f['marge_brute'] = df_f['montant_ca_net'] - df_f['cout_achat_total']
df_f['nombre_transactions'] = 1

# Sélection et renommage final pour MySQL
final_cols = {
    'id_temps': 'id_temps',
    'id_produit': 'id_produit',
    'id_lieu': 'id_lieu',
    'id_client': 'id_client',
    'id_promotion': 'id_promotion',
    'PrixUnitaire': 'prix_unitaire',
    'Quantite': 'quantite_vendue',
    'cout_achat_total': 'cout_achat_total',
    'montant_ca_net': 'montant_ca_net',
    'marge_brute': 'marge_brute',
    'nombre_transactions': 'nombre_transactions'
}

df_final = df_f[list(final_cols.keys())].rename(columns=final_cols)

# Envoi vers vente_DW par paquets de 1000
df_final.to_sql('Vente', engine_dw, if_exists='append', index=False, chunksize=1000)

print(" Terminé ! Toutes les tables du flocon sont remplies avec succès.")
