import pandas as pd
from sqlalchemy import create_engine, text
import sys
import os

# Add project root to sys.path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from config.db_config import CONN_STR_VENTE_DB, CONN_STR_VENTE_DW

engine_src = create_engine(CONN_STR_VENTE_DB)
engine_dw = create_engine(CONN_STR_VENTE_DW)

def vider_entrepots():
    print("Nettoyage de l'entrepôt en cours...")
    with engine_dw.connect() as conn:
        conn.execute(text("SET FOREIGN_KEY_CHECKS = 0;"))
        tables = [
            "Fait_Ventes", 
            "Dim_PointVente", "Dim_Ville", "Dim_Region",
            "Dim_Produit", "Dim_Categorie",
            "Dim_Client", "Dim_TypeClient",
            "Dim_Temps", "Dim_Canal", "Dim_Promotion"
        ]
        
        for t in tables:
            conn.execute(text(f"TRUNCATE TABLE {t};"))
        conn.execute(text("SET FOREIGN_KEY_CHECKS = 1;"))
        conn.commit()
    print("Entrepôt vidé avec succès.")

def charger_dw():
    """ 5.b : Processus ETL complet """
    print("Début du chargement ETL Python...")

    df_op = pd.read_sql("SELECT * FROM vente_op", engine_src)
    df_op['DateVente'] = pd.to_datetime(df_op['DateVente']) 
       
    # BRANCHE GEOGRAPHIE (Region -> Ville -> PointVente)
    # A. Region
    regions = df_op[['Region']].drop_duplicates().rename(columns={'Region': 'Libelle_Region'})
    regions.to_sql('Dim_Region', engine_dw, if_exists='append', index=False)
    # Récupération des IDs générés (Lookup)
    ref_region = pd.read_sql("SELECT * FROM Dim_Region", engine_dw)
    # On ajoute l'ID_Region dans notre DataFrame principal
    df_op = df_op.merge(ref_region, left_on='Region', right_on='Libelle_Region', how='left')

    # B. Ville (On a besoin de l'ID_Region inséré juste avant)
    villes = df_op[['Ville', 'ID_Region']].drop_duplicates().rename(columns={'Ville': 'Nom_Ville'})
    villes.to_sql('Dim_Ville', engine_dw, if_exists='append', index=False)
    # Lookup Ville
    ref_ville = pd.read_sql("SELECT ID_Ville, Nom_Ville FROM Dim_Ville", engine_dw)
    df_op = df_op.merge(ref_ville, left_on='Ville', right_on='Nom_Ville', how='left')

    # C. Point de Vente (On a besoin de l'ID_Ville)
    pvs = df_op[['PointVente', 'ID_Ville']].drop_duplicates().rename(columns={'PointVente': 'Nom_PointVente'})
    pvs.to_sql('Dim_PointVente', engine_dw, if_exists='append', index=False)
    # Lookup PointVente
    ref_pv = pd.read_sql("SELECT ID_PointVente, Nom_PointVente FROM Dim_PointVente", engine_dw)
    df_op = df_op.merge(ref_pv, left_on='PointVente', right_on='Nom_PointVente', how='left')

    # BRANCHE PRODUIT (Categorie -> Produit)
    
    # A. Categorie
    cats = df_op[['Categorie']].drop_duplicates().rename(columns={'Categorie': 'Libelle_Categorie'})
    cats.to_sql('Dim_Categorie', engine_dw, if_exists='append', index=False)
    # Lookup Categorie
    ref_cat = pd.read_sql("SELECT * FROM Dim_Categorie", engine_dw)
    df_op = df_op.merge(ref_cat, left_on='Categorie', right_on='Libelle_Categorie', how='left')

    # B. Produit
    prods = df_op[['Produit', 'ID_Categorie']].drop_duplicates().rename(columns={'Produit': 'Nom_Produit'})
    prods.to_sql('Dim_Produit', engine_dw, if_exists='append', index=False)
    # Lookup Produit
    ref_prod = pd.read_sql("SELECT ID_Produit, Nom_Produit FROM Dim_Produit", engine_dw)
    df_op = df_op.merge(ref_prod, left_on='Produit', right_on='Nom_Produit', how='left')

    # --- BRANCHE CLIENT (Type -> Client) ---
    
    # A. Type Client
    types = df_op[['TypeClient']].drop_duplicates().rename(columns={'TypeClient': 'Libelle_Type'})
    types.to_sql('Dim_TypeClient', engine_dw, if_exists='append', index=False)
    # Lookup Type
    ref_type = pd.read_sql("SELECT * FROM Dim_TypeClient", engine_dw)
    df_op = df_op.merge(ref_type, left_on='TypeClient', right_on='Libelle_Type', how='left')

    # B. Client
    clients = df_op[['NomClient', 'ID_TypeClient']].drop_duplicates().rename(columns={'NomClient': 'Nom_Client'})
    clients.to_sql('Dim_Client', engine_dw, if_exists='append', index=False)
    # Lookup Client
    ref_client = pd.read_sql("SELECT ID_Client, Nom_Client FROM Dim_Client", engine_dw)
    df_op = df_op.merge(ref_client, left_on='NomClient', right_on='Nom_Client', how='left')

    # --- AUTRES DIMENSIONS ---
    
    # Canal
    canaux = df_op[['ModeCommande']].drop_duplicates().rename(columns={'ModeCommande': 'Mode_Commande'})
    canaux.to_sql('Dim_Canal', engine_dw, if_exists='append', index=False)
    ref_canal = pd.read_sql("SELECT * FROM Dim_Canal", engine_dw)
    df_op = df_op.merge(ref_canal, left_on='ModeCommande', right_on='Mode_Commande', how='left')

    # Promotion
    promotions = df_op[['NomPromo', 'Promotion']].drop_duplicates().rename(columns={'NomPromo': 'Nom_Promotion', 'Promotion': 'Est_Promotion_Active'})
    promotions.to_sql('Dim_Promotion', engine_dw, if_exists='append', index=False)
    ref_promotion = pd.read_sql("SELECT * FROM Dim_Promotion", engine_dw)
    df_op = df_op.merge(ref_promotion, left_on='NomPromo', right_on='Nom_Promotion', how='left')

    # Temps
    temps = pd.DataFrame()
    temps['Date_ID'] = df_op['DateVente'].drop_duplicates()
    temps['Annee'] = temps['Date_ID'].dt.year
    temps['Mois'] = temps['Date_ID'].dt.month
    temps['Mois_Nom'] = temps['Date_ID'].dt.month_name()
    temps['Trimestre'] = temps['Date_ID'].dt.quarter
    temps.to_sql('Dim_Temps', engine_dw, if_exists='append', index=False)

    # ---------------------------------------------------------
    # 3. CHARGEMENT DE LA TABLE DE FAITS
    # ---------------------------------------------------------
    print("chargement de la table de fait ")
    df_fait = pd.DataFrame()
    df_fait['Num_Ticket_Source'] = df_op['ID_Vente']
    df_fait['Date_ID'] = df_op['DateVente']
    
    # Clés étrangères (qu'on a récupérées via les merges successifs)
    df_fait['ID_Produit'] = df_op['ID_Produit']
    df_fait['ID_PointVente'] = df_op['ID_PointVente']
    df_fait['ID_Client'] = df_op['ID_Client']
    df_fait['ID_Canal'] = df_op['ID_Canal']
    df_fait['ID_Promotion'] = df_op['ID_Promotion']
    
    # Mesures brutes
    df_fait['Quantite'] = df_op['Quantite']
    df_fait['Prix_Unitaire'] = df_op['PrixUnitaire']
    df_fait['Cout_Achat'] = df_op['CoutAchat']
    df_fait['Taux_Promo_Applique'] = df_op['TauxPromo']
       
    # Calcul des mesures dérivées (KPIs)
    df_fait['Montant_Vente'] = df_fait['Quantite'] * df_fait['Prix_Unitaire']
    df_fait['Marge'] = (df_fait['Prix_Unitaire'] - df_fait['Cout_Achat']) * df_fait['Quantite']
   
    # Insertion finale
    df_fait.to_sql('Fait_Ventes', engine_dw, if_exists='append', index=False)
    print("Succès : Datawarehouse alimenté via Python !")


if __name__ == "__main__":
    vider_entrepots()
    charger_dw()
