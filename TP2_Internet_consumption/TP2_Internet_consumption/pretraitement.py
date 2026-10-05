import pandas as pd

df = pd.read_csv('data/processed/internet_consumption_complete.csv')
df['datetime'] = pd.to_datetime(df['datetime'])
df['heure'] = df['datetime'].dt.hour
cols = ['heure'] + [col for col in df.columns if col not in ['datetime', 'heure']]
df = df[cols]

def encoder_statut(df):
    mapping = {
        'Connecté': 1,
        'Déconnecté': 0
    }
    df['statut_connexion'] = df['statut_connexion'].map(mapping)
    
    return df

def remove_session_id_duree(df):
    df = df.drop(columns=['session_id'])
    df = df.drop(columns=['duree_minutes'])
    df = df.drop(columns=['jour_semaine'])
    return df

def encoder_periodes(df):
    df_encoded = pd.get_dummies(df, columns=['periode_journee'], dtype=int)
    
    return df_encoded

df = remove_session_id_duree(df)
df = encoder_statut(df)
df = encoder_periodes(df)

df.to_csv('dataset_modifie.csv', index=False)