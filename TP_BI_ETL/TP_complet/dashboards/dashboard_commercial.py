import streamlit as st
import pandas as pd
import plotly.express as px
from sqlalchemy import create_engine
import sys
import os

# Add project root to sys.path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from config.db_config import CONN_STR_VENTE_DW

st.set_page_config(page_title="Dashboard Commercial", layout="wide")
engine = create_engine(CONN_STR_VENTE_DW)

def load_comm_data():
    query = """
    SELECT 
        f.Montant_Vente, f.Quantite, f.Num_Ticket_Source,
        p.Nom_Produit, 
        c.Mode_Commande,
        v.Nom_Ville
    FROM Fait_Ventes f
    JOIN Dim_Produit p ON f.ID_Produit = p.ID_Produit
    JOIN Dim_Canal c ON f.ID_Canal = c.ID_Canal
    JOIN Dim_PointVente pv ON f.ID_PointVente = pv.ID_PointVente
    JOIN Dim_Ville v ON pv.ID_Ville = v.ID_Ville
    """
    return pd.read_sql(query, engine)

df = load_comm_data()

st.title("Pilotage Commercial")

# --- CALCUL DES KPIs ---
# Panier Moyen = CA Total / Nombre de Ventes (Tickets uniques)
panier_moyen = df['Montant_Vente'].sum() / df['Num_Ticket_Source'].nunique()

col1, col2 = st.columns(2)
with col1:
    st.metric("Panier Moyen", f"{panier_moyen:.2f} FCFA")
with col2:
    st.metric("Volume Total d'Articles", f"{df['Quantite'].sum():,.0f}")

# --- ANALYSE DES CANAUX ET PRODUITS ---
col_a, col_b = st.columns(2)

with col_a:
    st.subheader("Performance par Canal")
    st.markdown("_Question : Quel canal est le plus performant ?_")
    df_canal = df.groupby('Mode_Commande')['Montant_Vente'].sum().reset_index()
    fig_canal = px.pie(df_canal, values='Montant_Vente', names='Mode_Commande', hole=0.4,
                       title="Pie Chart : Performance par Canal")
    st.plotly_chart(fig_canal)

with col_b:
    st.subheader("Produits les plus vendus (Volume)")
    st.markdown("_Question : Quels produits doivent être davantage promus ?_")
    df_top_v = df.groupby('Nom_Produit')['Quantite'].sum().nlargest(10).reset_index()
    fig_top_v = px.bar(df_top_v, y='Nom_Produit', x='Quantite', orientation='h', color='Quantite',
                       color_continuous_scale='Greens',
                       title="Horizontal Bar Chart : Produits les plus vendus")
    st.plotly_chart(fig_top_v)

# --- COMPARAISON DES VILLES ---
st.subheader("Comparaison du CA par Ville")
st.markdown("_Question : Quelles villes ont le plus fort potentiel ?_")
df_ville = df.groupby('Nom_Ville')['Montant_Vente'].sum().reset_index().sort_values('Montant_Vente', ascending=False)
st.plotly_chart(px.scatter(df_ville, x='Nom_Ville', y='Montant_Vente', size='Montant_Vente', color='Nom_Ville',
                           title="Scatter Plot : Comparaison du CA par Ville"))


# Calculs pour la note
top_canal = df_canal.sort_values('Montant_Vente', ascending=False).iloc[0]['Mode_Commande']
top_ville = df_ville.iloc[0]['Nom_Ville']
top_produit_vol = df_top_v.iloc[0]['Nom_Produit']

st.sidebar.header("Analyse & Recommandations")
st.sidebar.info(f"""
**Analyse des résultats :**
- Le canal de vente dominant est **{top_canal}**. 
- La ville présentant le plus fort potentiel actuel est **{top_ville}**. 
- En termes de volume, le produit **{top_produit_vol}** est le plus vendu. 

**Recommandation :** Renforcer la logistique dans la ville de **{top_ville}** et envisager des campagnes spécifiques sur le canal **{top_canal}** pour maximiser le panier moyen. 
""")