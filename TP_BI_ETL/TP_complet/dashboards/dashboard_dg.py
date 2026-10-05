import streamlit as st
import pandas as pd
import plotly.express as px
from sqlalchemy import create_engine
import sys
import os

# Add project root to sys.path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from config.db_config import CONN_STR_VENTE_DW

# --- CONFIGURATION DE LA PAGE ---
st.set_page_config(page_title="Dashboard Direction Générale - Les Débrouillards", layout="wide")

# --- CONNEXION À LA BASE DE DONNÉES ---
@st.cache_resource
def get_connection():
    # Paramètres basés sur votre fichier db_config.py
    return create_engine(CONN_STR_VENTE_DW)

engine = get_connection()

# --- CHARGEMENT DES DONNÉES ---
def load_data():
    query = """
    SELECT 
        f.Montant_Vente, f.Marge, f.Date_ID,
        p.Nom_Produit, 
        r.Libelle_Region,
        t.Annee, t.Mois, t.Mois_Nom
    FROM Fait_Ventes f
    JOIN Dim_Produit p ON f.ID_Produit = p.ID_Produit
    JOIN Dim_PointVente pv ON f.ID_PointVente = pv.ID_PointVente
    JOIN Dim_Ville v ON pv.ID_Ville = v.ID_Ville
    JOIN Dim_Region r ON v.ID_Region = r.ID_Region
    JOIN Dim_Temps t ON f.Date_ID = t.Date_ID
    """
    return pd.read_sql(query, engine)

df = load_data()

# --- TITRE DU DASHBOARD ---
st.title("Rapport Stratégique : Direction Générale")
st.markdown("Ce tableau de bord présente la vision globale et la performance de l'entreprise.")

# --- SECTION 1 : KPIs FLASH  ---
col1, col2, col3 = st.columns(3)

total_ca = df['Montant_Vente'].sum()
total_marge = df['Marge'].sum()
taux_marge = (total_marge / total_ca) * 100 if total_ca != 0 else 0

with col1:
    st.metric("Chiffre d'Affaires Global", f"{total_ca:,.2f} FCFA")
with col2:
    st.metric("Marge Totale", f"{total_marge:,.2f} FCFA")
with col3:
    st.metric("Taux de Marge Moyen", f"{taux_marge:.2f} %")

st.divider()

# --- SECTION 2 : ANALYSE TEMPORELLE ET TOP RÉGIONS ---
col_left, col_right = st.columns(2)

with col_left:
    st.subheader("Évolution mensuelle du CA")
    st.markdown("_Objectif : Voir si l'entreprise est en croissance._")
    # Tri par date pour le graphique
    df_mensuel = df.groupby(['Annee', 'Mois', 'Mois_Nom'])['Montant_Vente'].sum().reset_index()
    df_mensuel = df_mensuel.sort_values(['Annee', 'Mois'])
    
    # Conversion de l'année en string pour des couleurs distinctes
    df_mensuel['Annee'] = df_mensuel['Annee'].astype(str)
    
    fig_evol = px.line(df_mensuel, x='Mois_Nom', y='Montant_Vente', color='Annee',
                       title="Line Chart : Chiffre d'Affaires par Mois", markers=True)
    st.plotly_chart(fig_evol, width='stretch')

with col_right:
    st.subheader("Top Régions par CA")
    st.markdown("_Objectif : Identifier les régions qui performent le mieux._")
    df_region = df.groupby('Libelle_Region')['Montant_Vente'].sum().reset_index().sort_values('Montant_Vente', ascending=False)
    fig_reg = px.bar(df_region, x='Libelle_Region', y='Montant_Vente', 
                     color='Montant_Vente', title="Bar Chart : Performance Géographique",
                     color_continuous_scale='Greens') # Shades of green
    st.plotly_chart(fig_reg, width='stretch')

# --- SECTION 3 : TOP PRODUITS ---
st.divider()
st.subheader("Top Produits Stratégiques")
st.markdown("_Objectif : Identifier les produits stratégiques._")
df_prod = df.groupby('Nom_Produit')[['Montant_Vente', 'Marge']].sum().sort_values('Montant_Vente', ascending=False).head(10)

fig_prod = px.bar(df_prod.reset_index(), x='Nom_Produit', y='Montant_Vente', 
                  text_auto='.2s', color='Marge',
                  title="Bar Chart : Top 10 Produits par CA (couleur par Marge)",
                  color_continuous_scale='Greens') # Shades of green
st.plotly_chart(fig_prod, width='stretch')

# --- SECTION 4 : ANALYSE ÉCRITE ---
st.sidebar.header("Analyse & Recommandations")
st.sidebar.info(f"""
**Analyse des résultats :**
- Le CA total est de {total_ca:,.0f} FCFA.
- La région la plus performante est **{df_region.iloc[0]['Libelle_Region']}**.
- Le produit leader est **{df_prod.index[0]}**.

**Recommandation :** Maintenir les efforts sur les produits à forte marge pour optimiser la rentabilité globale.
""")