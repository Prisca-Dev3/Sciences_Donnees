import pandas as pd
import mysql.connector
from sqlalchemy import create_engine

    # 1. PARAMÈTRES DE CONNEXION (À ajuster selon votre MySQL)
user = "root"
password = " " # Mettez votre mot de passe ici
host = "localhost"
db_name = "vente_DB"

try:
    # Création de la connexion via SQLAlchemy (requis pour pandas)
    engine = create_engine(f"mysql+mysqlconnector://{user}:{password}@{host}/{db_name}")

    # 2. CHARGEMENT DU FICHIER CSV
    # On précise le nom exact du fichier reçu
    file_path = "ventes_les_debrouillards_Annees2018-2025_10000Ventes_600clients.csv"
    df = pd.read_csv(file_path)

    # 3. NETTOYAGE ET PRÉPARATION DES DONNÉES 
    # Remplacons les espaces vides dans TauxPromo par 0 pour éviter les erreurs SQL. Pour le nom de la promotion: on remplace le vide par une valeur explicite
    df['TauxPromo'] = pd.to_numeric(df['TauxPromo'], errors='coerce').fillna(0)
    df['NomPromo'] = df['NomPromo'].replace([' ', '', 'nan'], 'Sans Promotion').fillna('Sans Promotion')
    
    # S'assurer que la date est bien reconnue
    df['DateVente'] = pd.to_datetime(df['DateVente'])

    # 4. INSERTION DANS LA TABLE TAMPON (vente_op)
    # if_exists='append' car la table est déjà créée par notre script SQL
    df.to_sql('vente_op', con=engine, if_exists='append', index=False, chunksize=1000)

    print(f"Succès ! {len(df)} lignes ont été chargées dans la table tampon vente_op.")

except Exception as e:
    print(f"Erreur lors du chargement : {e}")
