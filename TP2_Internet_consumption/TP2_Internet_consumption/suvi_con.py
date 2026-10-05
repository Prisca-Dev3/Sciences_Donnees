"""
suivi_connexion.py
------------------
Suit la consommation de connexion internet en temps réel :
  - Durée (en minutes)
  - Volume de données consommées (en Mo/minute) pour download ET upload
  - Statut de connexion
  - Période de la journée

Les données sont conservées même si le programme est arrêté et relancé.
Nécessite : psutil  →  pip install psutil
"""

import csv
import time
import socket
import os
from datetime import datetime

try:
    import psutil
except ImportError:
    print("[ERREUR] Le module 'psutil' est requis.")
    print("         Installez-le avec : pip install psutil")
    exit(1)

# ─── CONFIGURATION ────────────────────────────────────────────────────────────
CSV_FILE          = "consommation_connexion.csv"
INTERVAL_SECONDES = 60          # 60s = 1 mesure par minute
HOTE_TEST         = "8.8.8.8"  # DNS Google pour tester la connexion
PORT_TEST         = 53
TIMEOUT           = 3

# ─── PÉRIODES DE LA JOURNÉE ───────────────────────────────────────────────────
def get_periode(heure: int) -> str:
    if 5 <= heure < 9:
        return "Matin tôt (05h-09h)"
    elif 9 <= heure < 12:
        return "Matinée (09h-12h)"
    elif 12 <= heure < 14:
        return "Midi (12h-14h)"
    elif 14 <= heure < 18:
        return "Après-midi (14h-18h)"
    elif 18 <= heure < 21:
        return "Soirée (18h-21h)"
    elif 21 <= heure < 24:
        return "Nuit (21h-00h)"
    else:
        return "Nuit profonde (00h-05h)"

# ─── TEST DE CONNEXION ────────────────────────────────────────────────────────
def est_connecte() -> bool:
    try:
        socket.setdefaulttimeout(TIMEOUT)
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOTE_TEST, PORT_TEST))
        s.close()
        return True
    except (socket.error, OSError):
        return False

# ─── LECTURE DU TRAFIC RÉSEAU (octets cumulés depuis le boot) ─────────────────
def lire_trafic_reseau():
    """Retourne (octets_reçus, octets_envoyés) cumulés depuis le démarrage."""
    stats = psutil.net_io_counters()
    return stats.bytes_recv, stats.bytes_sent

def octets_en_mo(octets: int) -> float:
    """Convertit des octets en mégaoctets, arrondi à 4 décimales."""
    return round(octets / (1024 * 1024), 4)

# ─── INITIALISATION DU FICHIER CSV ───────────────────────────────────────────
EN_TETES = [
    "date",
    "heure",
    "jour_semaine",
    "periode_journee",
    "statut_connexion",
    "duree_minutes",
    "download_Mo",
    "upload_Mo",
    "total_Mo",
    "session_id",
]

def initialiser_csv(chemin: str):
    if not os.path.exists(chemin):
        with open(chemin, mode="w", newline="", encoding="utf-8") as f:
            csv.writer(f).writerow(EN_TETES)
        print(f"[INFO] Nouveau fichier CSV créé : {chemin}")
    else:
        print(f"[INFO] Fichier existant détecté ({chemin}) — données ajoutées à la suite.")

# ─── ENREGISTREMENT D'UNE LIGNE ──────────────────────────────────────────────
def enregistrer_mesure(chemin: str, connecte: bool, session_id: str,
                        dl_mo: float, ul_mo: float):
    maintenant = datetime.now()
    jours_fr   = ["Lundi", "Mardi", "Mercredi", "Jeudi",
                  "Vendredi", "Samedi", "Dimanche"]

    total_mo = round(dl_mo + ul_mo, 4)

    ligne = {
        "date"             : maintenant.strftime("%Y-%m-%d"),
        "heure"            : maintenant.strftime("%H:%M:%S"),
        "jour_semaine"     : jours_fr[maintenant.weekday()],
        "periode_journee"  : get_periode(maintenant.hour),
        "statut_connexion" : "Connecté" if connecte else "Déconnecté",
        "duree_minutes"    : 1,
        "download_Mo"      : dl_mo,
        "upload_Mo"        : ul_mo,
        "total_Mo"         : total_mo,
        "session_id"       : session_id,
    }

    with open(chemin, mode="a", newline="", encoding="utf-8") as f:
        csv.DictWriter(f, fieldnames=EN_TETES).writerow(ligne)

    statut_emoji = "🟢" if connecte else "🔴"
    print(
        f"{statut_emoji} [{ligne['heure']}] {ligne['periode_journee']:<25} | "
        f"Statut : {ligne['statut_connexion']:<11} | "
        f"DL {dl_mo:>8.4f} Mo  UL {ul_mo:>8.4f} Mo  Total {total_mo:>8.4f} Mo"
    )

# ─── BOUCLE PRINCIPALE ───────────────────────────────────────────────────────
def main():
    print("=" * 75)
    print("     SUIVI DE CONSOMMATION DE CONNEXION INTERNET (Mo + minutes)")
    print("=" * 75)
    print(f"  Fichier CSV  : {CSV_FILE}")
    print(f"  Intervalle   : {INTERVAL_SECONDES} secondes (1 mesure / minute)")
    print(f"  Arret propre : Ctrl+C\n")

    session_id = datetime.now().strftime("S%Y%m%d_%H%M%S")
    print(f"  Session      : {session_id}\n")

    initialiser_csv(CSV_FILE)

    # Capture de référence (avant la première minute)
    recv_avant, sent_avant = lire_trafic_reseau()

    print(f"\n{'HEURE':<10} {'PERIODE':<25} {'STATUT':<12} "
          f"{'DL (Mo)':>12} {'UL (Mo)':>12} {'TOTAL':>10}")
    print("-" * 82)

    try:
        while True:
            # Attente de l'intervalle
            time.sleep(INTERVAL_SECONDES)

            # Lecture après l'intervalle
            recv_apres, sent_apres = lire_trafic_reseau()

            # Différence = consommation pendant cette minute
            dl_mo = octets_en_mo(recv_apres - recv_avant)
            ul_mo = octets_en_mo(sent_apres - sent_avant)

            # Mise à jour de la référence pour la prochaine minute
            recv_avant, sent_avant = recv_apres, sent_apres

            connecte = est_connecte()
            enregistrer_mesure(CSV_FILE, connecte, session_id, dl_mo, ul_mo)

    except KeyboardInterrupt:
        print("\n\n[INFO] Programme arrete. Donnees sauvegardees dans :")
        print(f"       {os.path.abspath(CSV_FILE)}")

        # Résumé de la session
        print("\n--- Resume de la session -------------------------------------------")
        total_dl = 0.0
        total_ul = 0.0
        nb_lignes = 0
        try:
            with open(CSV_FILE, newline="", encoding="utf-8") as f:
                reader = csv.DictReader(f)
                for row in reader:
                    if row.get("session_id") == session_id:
                        total_dl += float(row["download_Mo"])
                        total_ul += float(row["upload_Mo"])
                        nb_lignes += 1
        except Exception:
            pass

        if nb_lignes:
            print(f"  Duree totale   : {nb_lignes} minute(s)")
            print(f"  Total download : {total_dl:.4f} Mo  ({total_dl/1024:.4f} Go)")
            print(f"  Total upload   : {total_ul:.4f} Mo  ({total_ul/1024:.4f} Go)")
            print(f"  Total general  : {total_dl+total_ul:.4f} Mo  ({(total_dl+total_ul)/1024:.4f} Go)")
        print("--------------------------------------------------------------------")

if __name__ == "__main__":
    main()