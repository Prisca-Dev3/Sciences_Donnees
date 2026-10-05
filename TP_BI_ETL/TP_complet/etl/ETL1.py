import pandas as pd
from sqlalchemy import create_engine, text
import sys
import os

# Add project root to sys.path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from config.db_config import CONN_STR_VENTE_DB

# Utilisation de la configuration centralisée
db_connection = create_engine(CONN_STR_VENTE_DB)

# Path to CSV file
data_path = os.path.join(os.path.dirname(__file__), '../data/donnees_ventes.csv')
df = pd.read_csv(data_path)

df['TauxPromo'] = pd.to_numeric(df['TauxPromo'], errors='coerce').fillna(0)
df['NomPromo'] = df['NomPromo'].fillna("Aucune")

# Vérification des types
print(df.head())
print(df.dtypes)

# Chargement dans la base de données
try:
    with db_connection.connect() as conn:
        # Get the maximum existing ID_Vente from the database
        try:
            result = conn.execute(text("SELECT MAX(ID_Vente) as max_id FROM vente_op"))
            max_id = result.fetchone()[0]
            max_id = max_id if max_id is not None else 0
        except Exception:
            # Table might be empty or doesn't exist yet
            max_id = 0
        
        # Generate new sequential IDs starting from max_id + 1
        df['ID_Vente'] = range(max_id + 1, max_id + 1 + len(df))
        
        df.to_sql('vente_op', con=conn, if_exists='append', index=False)
        conn.commit()  # Commit the transaction explicitly
    print(f"Succès : {len(df)} lignes ont été chargées dans vente_db.vente_op")
except Exception as e:
    print(f"Erreur lors du chargement : {e}")
    # The context manager will automatically rollback on exception
    db_connection.dispose()  # Clean up the connection pool
    raise  # Re-raise to see the full error
