import streamlit as st
import pandas as pd
import plotly.express as px
from sqlalchemy import create_engine
import sys
import os

# Ajout du chemin pour la config
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
from config.db_config import CONN_STR_VENTE_DW

# --- CONFIGURATION COULEURS ---
MAIN_COLOR = '#2E8B57'  # SeaGreen (Identité visuelle principale)
ACCENT_COLOR = '#FF7F50' # Coral (Pour les alertes ou promotions)
NEUTRAL_COLOR = '#95A5A6' # Gris (Pour "Hors Promotion")
COLOR_MAP_PROMO = {'En Promotion': MAIN_COLOR, 'Hors Promotion': NEUTRAL_COLOR}

st.set_page_config(page_title="Dashboard Marketing", layout="wide")
engine = create_engine(CONN_STR_VENTE_DW)

def load_mkt_data():
    # Requête mise à jour avec Dim_Promotion
    query = """
    SELECT 
        f.Montant_Vente, f.Marge, f.Taux_Promo_Applique,
        p.Nom_Promotion,
        CASE WHEN p.Est_Promotion_Active = 'Oui' OR f.Taux_Promo_Applique > 0 THEN 'En Promotion' ELSE 'Hors Promotion' END as Statut_Promo
    FROM Fait_Ventes f
    LEFT JOIN Dim_Promotion p ON f.ID_Promotion = p.ID_Promotion
    """
    return pd.read_sql(query, engine)

df = load_mkt_data()

st.title("Analyse Marketing & Promotions")
st.markdown("---")

# --- KPI 1 : IMPACT GLOBAL (HORS PROMO VS PROMO) ---
col_m1, col_m2 = st.columns(2)

with col_m1:
    st.markdown("_Question : Quel est l'impact global des promotions sur le chiffre d'affaires ?_")
    df_promo_ca = df.groupby('Statut_Promo')['Montant_Vente'].sum().reset_index()
    
    fig_global = px.bar(df_promo_ca, x='Statut_Promo', y='Montant_Vente', 
                        color='Statut_Promo',
                        color_discrete_map=COLOR_MAP_PROMO,
                        text_auto='.2s')
    
    # Optimisation Légende : On l'affiche pour répondre à la demande
    fig_global.update_layout(showlegend=True, margin=dict(t=30, l=0, r=0, b=0))
    st.plotly_chart(fig_global, width='stretch')

with col_m2:
    st.markdown("_Question : Existe-t-il une corrélation entre le taux de promotion appliqué et le montant des ventes ?_")
    # On filtre uniquement sur ce qui est en promo pour éviter le bruit du '0%'
    df_active_promo = df[df['Taux_Promo_Applique'] > 0]
    
    fig_corr = px.scatter(df_active_promo, x='Taux_Promo_Applique', y='Montant_Vente',
                          color_discrete_sequence=[MAIN_COLOR], # Couleur unique cohérente
                          opacity=0.6,
                          trendline="ols")
    fig_corr.update_layout(showlegend=True, margin=dict(t=30, l=0, r=0, b=0))
    st.plotly_chart(fig_corr, width='stretch')

# --- KPI 2 : PERFORMANCE PAR CAMPAGNE (NOUVEAU - Demande KPI f) ---
st.divider()
st.subheader("Top Campagnes Promotionnelles (Rentabilité)")
st.markdown("_Question : Quelles sont les campagnes promotionnelles les plus rentables en termes de marge ?_")

# On exclut les ventes "Standard" ou "Aucune" s'il y en a dans le nom
df_campagne = df[df['Statut_Promo'] == 'En Promotion'].groupby('Nom_Promotion')[['Marge', 'Montant_Vente']].sum().reset_index()
df_campagne = df_campagne.sort_values('Marge', ascending=False)

if not df_campagne.empty:
    fig_campagne = px.bar(df_campagne, x='Nom_Promotion', y='Marge',
                          color='Marge',
                          color_continuous_scale='Greens', # Dégradé de vert cohérent
                          text_auto='.2s')
    
    fig_campagne.update_layout(coloraxis_showscale=True, margin=dict(t=30, l=0, r=0, b=0))
    st.plotly_chart(fig_campagne, width='stretch')
else:
    st.warning("Aucune donnée de campagne spécifique trouvée. Vérifiez l'alimentation de Dim_Promotion.")

# --- BARRE LATERALE ---
st.sidebar.header("Analyse Stratégique")
ca_promo = df[df['Statut_Promo'] == 'En Promotion']['Montant_Vente'].sum()
marge_promo = df[df['Statut_Promo'] == 'En Promotion']['Marge'].sum()

st.sidebar.info(f"""
**Synthèse :**
- CA sous Promotion : **{ca_promo:,.0f} FCFA**
- Marge sous Promotion : **{marge_promo:,.0f} FCFA**

L'analyse ci-dessus par "Nom de Promotion" permet enfin de savoir quelle campagne (St Valentin, Rentrée...) est la plus rentable, répondant au besoin précis de la direction.
""")