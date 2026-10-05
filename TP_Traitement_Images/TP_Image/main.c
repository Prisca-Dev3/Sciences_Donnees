/* ============================================================
   main.c  -  INTERFACE UTILISATEUR INTERACTIVE
   Cours 02 (Base) - 03 (Convolution) - 04 (Fourier)
   Cours 05 (Contours) - 06 (Segmentation) - 07 (Images binaires)
   ============================================================ */

#include "image.h"

/* ============================================================
   ARBORESCENCE DE SORTIE (7 dossiers)
   ============================================================ */
#define DIR_RESULTATS "resultats/"
#define DIR_ENTREES   "resultats/00_images_entree/"
#define DIR_C02       "resultats/cours02_traitements_base/"
#define DIR_C03       "resultats/cours03_convolution/"
#define DIR_C04       "resultats/cours04_fourier/"
#define DIR_C05       "resultats/cours05_contours/"
#define DIR_C06       "resultats/cours06_segmentation/"
#define DIR_C07       "resultats/cours07_images_binaires/"

#define SRC_DIR       "images_source/"
#define FICHIER_TANK  SRC_DIR "tank.pgm"
#define FICHIER_SAIL  SRC_DIR "sailboat.ppm"

#define PI_TEXTURE 3.14159265358979323846

/* ============================================================
   PETITS UTILITAIRES D'ENTREE / SORTIE TERMINAL
   ============================================================ */

static void titre(const char *s)
{
    size_t len = strlen(s), i;
    printf("\n+");
    for (i = 0; i < len + 2; i++) printf("-");
    printf("+\n| %s |\n+", s);
    for (i = 0; i < len + 2; i++) printf("-");
    printf("+\n");
}

static int g_entree_terminee = 0;

static int lire_choix_int(const char *prompt, int min, int max)
{
    char buf[64];
    int val;
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) { g_entree_terminee = 1; return min; }
        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max)
            return val;
        printf("  [!] Choix invalide, entrez un nombre entre %d et %d.\n",
               min, max);
    }
}

static void copier_fichier_brut(const char *src, const char *dst)
{
    FILE *fi = fopen(src, "rb");
    FILE *fo;
    char buf[8192];
    size_t n;
    if (!fi) return;
    fo = fopen(dst, "wb");
    if (!fo) { fclose(fi); return; }
    while ((n = fread(buf, 1, sizeof(buf), fi)) > 0) fwrite(buf, 1, n, fo);
    fclose(fi); fclose(fo);
}

static void creer_dossiers(void)
{
    mkdir(DIR_RESULTATS, 0755);
    mkdir(DIR_ENTREES, 0755);
    mkdir(DIR_C02, 0755);
    mkdir(DIR_C03, 0755);
    mkdir(DIR_C04, 0755);
    mkdir(DIR_C05, 0755);
    mkdir(DIR_C06, 0755);
    mkdir(DIR_C07, 0755);
}

/* ============================================================
   AFFICHAGE STATISTIQUES
   ============================================================ */

static void afficher_stats_image(const ImageGris *img, const char *nom)
{
    int i, n = img->largeur * img->hauteur, vmin = 255, vmax = 0;
    double somme = 0.0;
    for (i = 0; i < n; i++) {
        int v = img->pixels[i];
        if (v < vmin) vmin = v;
        if (v > vmax) vmax = v;
        somme += v;
    }
    printf("    - %-24s min=%3d  max=%3d  moyenne=%7.2f\n",
           nom, vmin, vmax, somme / n);
}

/* ============================================================
   "LOT" DE RESULTATS AVEC SAUVEGARDE AUTOMATIQUE
   ============================================================ */
#define MAX_LOT 32
typedef struct { char nom[80]; ImageGris *img; } ResultatItem;
typedef struct { ResultatItem items[MAX_LOT]; int n; } Lot;

static void lot_init(Lot *l) { l->n = 0; }

static void lot_ajouter(Lot *l, const char *nom, ImageGris *img)
{
    if (!img) { printf("    [ERREUR] echec du calcul : %s\n", nom); return; }
    if (l->n >= MAX_LOT) {
        printf("    [!] lot plein, resultat '%s' ignore\n", nom);
        liberer_image_gris(img);
        return;
    }
    strncpy(l->items[l->n].nom, nom, sizeof(l->items[l->n].nom) - 1);
    l->items[l->n].nom[sizeof(l->items[l->n].nom) - 1] = '\0';
    l->items[l->n].img = img;
    l->n++;
}

static void traiter_lot(Lot *l, const char *dossier, const char *nom_image,
                         const char *titre_groupe)
{
    int i;
    if (l->n == 0) { printf("  (aucun resultat produit)\n"); return; }

    printf("\n  --- %s (%d resultat(s)) ---\n", titre_groupe, l->n);
    for (i = 0; i < l->n; i++)
        afficher_stats_image(l->items[i].img, l->items[i].nom);

   
    char chemin_apercu[320];
    snprintf(chemin_apercu, sizeof(chemin_apercu), "%s%s_%s_apercu.pgm",
              dossier, l->items[0].nom, nom_image);
    ecrire_pgm(l->items[0].img, chemin_apercu);
    printf("\n  [+] Apercu graphique genere -> %s\n", chemin_apercu);

   
    for (i = 0; i < l->n; i++) {
        char chemin[320];
        snprintf(chemin, sizeof(chemin), "%s%s_%s.pgm",
                  dossier, l->items[i].nom, nom_image);
        ecrire_pgm(l->items[i].img, chemin);
        printf("    -> %s\n", chemin);
    }

    for (i = 0; i < l->n; i++) liberer_image_gris(l->items[i].img);
    l->n = 0;
}

/* ============================================================
   GENERATEUR : TEXTURE PERIODIQUE SYNTHETIQUE
   ============================================================ */
static ImageGris *generer_texture_periodique(int largeur, int hauteur,
                                              double freq_x, double freq_y)
{
    ImageGris *img = allouer_image_gris(largeur, hauteur);
    int x, y;
    if (!img) return NULL;
    for (y = 0; y < hauteur; y++) {
        for (x = 0; x < largeur; x++) {
            double v = 128.0
                     + 60.0 * sin(2.0 * PI_TEXTURE * freq_x * x / largeur)
                     + 60.0 * sin(2.0 * PI_TEXTURE * freq_y * y / hauteur);
            PIXEL(img, x, y) = (unsigned char)CLAMP((int)(v + 0.5), 0, 255);
        }
    }
    return img;
}

/* ============================================================
   COMPATIBILITE ENTRE DEUX IMAGES
   ============================================================ */
static ImageGris *rendre_compatible(const ImageGris *a, const ImageGris *b)
{
    if (meme_dimensions(a, b)) return NULL;
    printf("    [INFO] dimensions differentes (%dx%d vs %dx%d)"
           " -> redimensionnement automatique\n",
           a->largeur, a->hauteur, b->largeur, b->hauteur);
    return redimensionner_compatible(b, a->largeur, a->hauteur);
}

/* ============================================================
   GROUPES D'OPERATIONS - COURS 02 : TRAITEMENTS DE BASE
   ============================================================ */

static void groupe_c02_statistiques(ImageGris *img, const char *nom,
                                     const char *dossier)
{
    int hist[256], profil[4096], nb_pts, i, x, y;
    int max_hist = 0, max_profil = 0;
    char chemin[320];
    
    titre("Statistiques & Rendu Graphique");
    
    /* 1. Uniquement les calculs de base requis */
    calculer_histogramme(img, hist);
    profil_intensite(img, img->hauteur / 2, profil, &nb_pts);
    
    printf("  Image : %s (%dx%d)\n", nom, img->largeur, img->hauteur);
    printf("  Luminance moyenne       : %.2f\n", calculer_luminance(img));
    printf("  Contraste (ecart-type)  : %.2f\n", calculer_contraste_ecarttype(img));
    printf("  Contraste (min/max)     : %.4f\n", calculer_contraste_minmax(img));
    printf("  Ligne de profil analysee: %d (contenant %d points)\n", img->hauteur / 2, nb_pts);
    
    for (i = 0; i < 256; i++) { if (hist[i] > max_hist) max_hist = hist[i]; }
    for (i = 0; i < nb_pts; i++) { if (profil[i] > max_profil) max_profil = profil[i]; }
    if (max_hist == 0) max_hist = 1;
    if (max_profil == 0) max_profil = 1;

    /* 2. Generation automatique de l'histogramme graphique PGM */
    int H_img = 200, W_img = 256;
    ImageGris *img_hist = allouer_image_gris(W_img, H_img);
    if (img_hist) {
        memset(img_hist->pixels, 230, W_img * H_img);
        for (x = 0; x < W_img; x++) {
            int hauteur_barre = (hist[x] * (H_img - 10)) / max_hist;
            for (y = H_img - 1; y >= H_img - hauteur_barre; y--) {
                PIXEL(img_hist, x, y) = 0;
            }
        }
        snprintf(chemin, sizeof(chemin), "%sc02_graph_histogramme_%s.pgm", dossier, nom);
        ecrire_pgm(img_hist, chemin);
        printf("  [+] Graphique Histogramme sauvegarde -> %s\n", chemin);
        liberer_image_gris(img_hist);
    }

    /* 3. Generation automatique du profil d'intensite graphique PGM */
    if (nb_pts > 0) {
        int H_prof = 256;
        ImageGris *img_prof = allouer_image_gris(nb_pts, H_prof);
        if (img_prof) {
            memset(img_prof->pixels, 255, nb_pts * H_prof);
            for (x = 0; x < nb_pts; x++) {
                int val_y = profil[x];
                if (val_y < 0) val_y = 0;
                if (val_y > 255) val_y = 255;
                int y_pixel = (H_prof - 1) - val_y;
                PIXEL(img_prof, x, y_pixel) = 0;
                for (y = H_prof - 1; y > y_pixel; y--) {
                    PIXEL(img_prof, x, y) = 200;
                }
            }
            snprintf(chemin, sizeof(chemin), "%sc02_graph_profil_%s.pgm", dossier, nom);
            ecrire_pgm(img_prof, chemin);
            printf("  [+] Graphique Profil intensite sauvegarde -> %s\n", chemin);
            liberer_image_gris(img_prof);
        }
    }
    
    /* L'aperçu de l'image source est cree sous forme de fichier PGM */
    snprintf(chemin, sizeof(chemin), "%ssource_%s_apercu.pgm", dossier, nom);
    ecrire_pgm(img, chemin);
    printf("  [+] Apercu de l'image source sauvegarde -> %s\n", chemin);
}

static void groupe_c02_contraste(ImageGris *img, const char *nom,
                                  const char *dossier)
{
    Lot lot; lot_init(&lot);
    lot_ajouter(&lot, "c02_01_lineaire", transformation_lineaire(img));
    lot_ajouter(&lot, "c02_02_saturation", transformation_lineaire_saturation(img, 50, 200));
    lot_ajouter(&lot, "c02_03_morceaux", transformation_lineaire_morceaux(img, 60, 180, 30));
    lot_ajouter(&lot, "c02_04a_gamma0.5", correction_gamma(img, 0.5));
    lot_ajouter(&lot, "c02_04b_gamma1.5", correction_gamma(img, 1.5));
    lot_ajouter(&lot, "c02_04c_gamma2.2", correction_gamma(img, 2.2));
    lot_ajouter(&lot, "c02_05_inversion", inversion(img));
    lot_ajouter(&lot, "c02_06_seuillage128", seuillage(img, 128));
    lot_ajouter(&lot, "c02_07_egalisation", egalisation_histogramme(img));
    lot_ajouter(&lot, "c02_08_egalisation_locale", egalisation_locale(img, 7));
    lot_ajouter(&lot, "c02_09_mult1.5", multiplication_image(img, 1.5));
    lot_ajouter(&lot, "c02_10_NON", non_image(img));
    traiter_lot(&lot, dossier, nom, "Transformations de contraste");
}

static void groupe_c02_interpolation(ImageGris *img, const char *nom,
                                      const char *dossier)
{
    Lot lot; lot_init(&lot);
    lot_ajouter(&lot, "c02_11a_ppv_x2", zoom_plus_proche_voisin(img, 2.0, 2.0));
    lot_ajouter(&lot, "c02_11b_bilineaire_x2", zoom_bilineaire(img, 2.0, 2.0));
    lot_ajouter(&lot, "c02_11c_bicubique_x2", zoom_bicubique(img, 2.0, 2.0));
    lot_ajouter(&lot, "c02_11d_bilineaire_x0.5", zoom_bilineaire(img, 0.5, 0.5));
    traiter_lot(&lot, dossier, nom, "Interpolation / Zoom");
}

static void groupe_c02_couleur(const char *fichier, const char *nom,
                                const char *dossier)
{
    ImageCouleur *img_c, *r_egal, *r_gauss, *r_inv, *r_lin;
    ImageGris *cr, *cg, *cb, *fr, *fg, *fb;

    titre("Traitement couleur canal par canal (R/G/B)");
    img_c = charger_image_couleur(fichier);
    if (!img_c) { printf("  [ERREUR] chargement couleur impossible\n"); return; }

    r_egal = appliquer_sur_canaux(img_c, egalisation_histogramme);

    cr = extraire_canal(img_c, 0);
    cg = extraire_canal(img_c, 1);
    cb = extraire_canal(img_c, 2);
    fr = filtre_gaussien(cr, 5, 1.4);
    fg = filtre_gaussien(cg, 5, 1.4);
    fb = filtre_gaussien(cb, 5, 1.4);
    r_gauss = assembler_canaux(fr, fg, fb);
    liberer_image_gris(cr); liberer_image_gris(cg); liberer_image_gris(cb);
    liberer_image_gris(fr); liberer_image_gris(fg); liberer_image_gris(fb);

    r_inv = appliquer_sur_canaux(img_c, inversion);
    r_lin = appliquer_sur_canaux(img_c, transformation_lineaire);

    printf("  [+] Sauvegarde automatique de l'ensemble des resultats couleur...\n");
    char chemin[320];
    snprintf(chemin, sizeof(chemin), "%sc02_coul_egalisation_%s.ppm", dossier, nom);
    ecrire_ppm(r_egal, chemin);  printf("    -> %s\n", chemin);
    snprintf(chemin, sizeof(chemin), "%sc02_coul_gaussien_%s.ppm", dossier, nom);
    ecrire_ppm(r_gauss, chemin); printf("    -> %s\n", chemin);
    snprintf(chemin, sizeof(chemin), "%sc02_coul_inversion_%s.ppm", dossier, nom);
    ecrire_ppm(r_inv, chemin);   printf("    -> %s\n", chemin);
    snprintf(chemin, sizeof(chemin), "%sc02_coul_lineaire_%s.ppm", dossier, nom);
    ecrire_ppm(r_lin, chemin);   printf("    -> %s\n", chemin);

    liberer_image_couleur(img_c);
    liberer_image_couleur(r_egal); liberer_image_couleur(r_gauss);
    liberer_image_couleur(r_inv);  liberer_image_couleur(r_lin);
}

static void groupe_c02_arithmetique(ImageGris *a, ImageGris *b,
                                     const char *na, const char *nb,
                                     const char *dossier)
{
    Lot lot; lot_init(&lot);
    char combo[80];
    ImageGris *b2 = rendre_compatible(a, b);
    ImageGris *bb = b2 ? b2 : b;
    snprintf(combo, sizeof(combo), "%s_x_%s", na, nb);
    titre("Operations arithmetiques entre paires d'images");
    lot_ajouter(&lot, "c02_add", addition_images(a, bb));
    lot_ajouter(&lot, "c02_sub_ij", soustraction_images(a, bb));
    lot_ajouter(&lot, "c02_sub_ji", soustraction_images(bb, a));
    lot_ajouter(&lot, "c02_mult", multiplication_images(a, bb));
    lot_ajouter(&lot, "c02_fusion50", fusion_images(a, bb, 0.5));
    liberer_image_gris(b2);
    traiter_lot(&lot, dossier, combo, "Operations arithmetiques");
}

static void groupe_c02_logique(ImageGris *a, ImageGris *b,
                                const char *na, const char *nb,
                                const char *dossier)
{
    Lot lot; lot_init(&lot);
    char combo[80];
    ImageGris *b2 = rendre_compatible(a, b);
    ImageGris *bb = b2 ? b2 : b;
    snprintf(combo, sizeof(combo), "%s_x_%s", na, nb);
    titre("Operations logiques entre paires d'images");
    lot_ajouter(&lot, "c02_ET", et_images(a, bb));
    lot_ajouter(&lot, "c02_OU", ou_images(a, bb));
    lot_ajouter(&lot, "c02_XOR", xor_images(a, bb));
    lot_ajouter(&lot, "c02_NON_a", non_image(a));
    lot_ajouter(&lot, "c02_NON_ET", non_et_images(a, bb));
    liberer_image_gris(b2);
    traiter_lot(&lot, dossier, combo, "Operations logiques");
}

/* ============================================================
   GROUPES D'OPERATIONS - COURS 03 : CONVOLUTION
   ============================================================ */

static void groupe_c03_moyenneur(ImageGris *img, const char *nom,
                                  const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtre moyenneur (passe-bas)");
    lot_ajouter(&lot, "c03_moy3", filtre_moyenneur(img, 3));
    lot_ajouter(&lot, "c03_moy5", filtre_moyenneur(img, 5));
    lot_ajouter(&lot, "c03_moy7", filtre_moyenneur(img, 7));
    lot_ajouter(&lot, "c03_moy15", filtre_moyenneur(img, 15));
    traiter_lot(&lot, dossier, nom, "Filtre moyenneur");
}

static void groupe_c03_gaussien(ImageGris *img, const char *nom,
                                 const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtre gaussien (passe-bas)");
    lot_ajouter(&lot, "c03_gauss_5x5_s1.0", filtre_gaussien(img, 5, 1.0));
    lot_ajouter(&lot, "c03_gauss_5x5_s1.4", filtre_gaussien(img, 5, 1.4));
    lot_ajouter(&lot, "c03_gauss_11x11_s2.0", filtre_gaussien(img, 11, 2.0));
    traiter_lot(&lot, dossier, nom, "Filtre gaussien");
}

static void groupe_c03_median(ImageGris *img, const char *nom,
                               const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtre median (non lineaire)");
    lot_ajouter(&lot, "c03_median3", filtre_median(img, 3));
    lot_ajouter(&lot, "c03_median5", filtre_median(img, 5));
    lot_ajouter(&lot, "c03_median7", filtre_median(img, 7));
    traiter_lot(&lot, dossier, nom, "Filtre median");
}

static void groupe_c03_minmax(ImageGris *img, const char *nom,
                               const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtres min / max (morphologiques)");
    lot_ajouter(&lot, "c03_min3", filtre_min(img, 3));
    lot_ajouter(&lot, "c03_max3", filtre_max(img, 3));
    traiter_lot(&lot, dossier, nom, "Filtres min / max");
}

static void groupe_c03_noyau_cours(ImageGris *img, const char *nom,
                                    const char *dossier)
{
    double noyau[25] = { 1, 2, 3, 2, 1,
                          2, 6, 8, 6, 2,
                          3, 8,10, 8, 3,
                          2, 6, 8, 6, 2,
                          1, 2, 3, 2, 1 };
    Lot lot; lot_init(&lot);
    titre("Noyau gaussien exact du cours (5x5)");
    lot_ajouter(&lot, "c03_noyau_cours_5x5", convolution_2d(img, noyau, 5));
    traiter_lot(&lot, dossier, nom, "Noyau du cours");
}

static void groupe_c03_bruit(ImageGris *img, const char *nom,
                              const char *dossier)
{
    Lot lot; lot_init(&lot);
    ImageGris *bruite = copier_image_gris(img);
    int np = img->largeur * img->hauteur, i;
    titre("Bruit poivre & sel + debruitage comparatif");
    srand(42);
    for (i = 0; i < np / 10; i++) {
        int pos = rand() % np;
        bruite->pixels[pos] = (rand() % 2) ? 0 : 255;
    }
    lot_ajouter(&lot, "c03_bruite_poivre_sel", copier_image_gris(bruite));
    lot_ajouter(&lot, "c03_debruite_median3", filtre_median(bruite, 3));
    lot_ajouter(&lot, "c03_debruite_moyenneur3", filtre_moyenneur(bruite, 3));
    lot_ajouter(&lot, "c03_debruite_gaussien3", filtre_gaussien(bruite, 3, 1.0));
    liberer_image_gris(bruite);
    traiter_lot(&lot, dossier, nom, "Bruit + debruitage");
}

/* ============================================================
   GROUPES D'OPERATIONS - COURS 05 : DETECTION DE CONTOURS
   ============================================================ */

static void groupe_c05_derivee(ImageGris *img, const char *nom,
                                const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Derivee discrete simple");
    lot_ajouter(&lot, "c05_derivee_x", derivee_discrete_x(img));
    lot_ajouter(&lot, "c05_derivee_y", derivee_discrete_y(img));
    lot_ajouter(&lot, "c05_derivee_norme", derivee_discrete_norme(img));
    traiter_lot(&lot, dossier, nom, "Derivee discrete");
}

static void groupe_c05_roberts(ImageGris *img, const char *nom,
                                const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtre de Roberts");
    lot_ajouter(&lot, "c05_roberts_gx", roberts_gx(img));
    lot_ajouter(&lot, "c05_roberts_gy", roberts_gy(img));
    lot_ajouter(&lot, "c05_roberts_norme", roberts_norme(img));
    lot_ajouter(&lot, "c05_roberts_norme_euclid", roberts_norme_euclidienne(img));
    lot_ajouter(&lot, "c05_roberts_direction", roberts_direction(img));
    lot_ajouter(&lot, "c05_roberts_seuil30", roberts_seuillage(img, 30));
    lot_ajouter(&lot, "c05_roberts_seuil60", roberts_seuillage(img, 60));
    traiter_lot(&lot, dossier, nom, "Roberts");
}

static void groupe_c05_prewitt(ImageGris *img, const char *nom,
                                const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtre de Prewitt");
    lot_ajouter(&lot, "c05_prewitt_gx", prewitt_gx(img));
    lot_ajouter(&lot, "c05_prewitt_gy", prewitt_gy(img));
    lot_ajouter(&lot, "c05_prewitt_norme", prewitt_norme(img));
    lot_ajouter(&lot, "c05_prewitt_direction", prewitt_direction(img));
    lot_ajouter(&lot, "c05_prewitt_seuil30", prewitt_seuillage(img, 30));
    lot_ajouter(&lot, "c05_prewitt_seuil60", prewitt_seuillage(img, 60));
    traiter_lot(&lot, dossier, nom, "Prewitt");
}

static void groupe_c05_sobel(ImageGris *img, const char *nom,
                              const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtre de Sobel");
    lot_ajouter(&lot, "c05_sobel_gx", sobel_gx(img));
    lot_ajouter(&lot, "c05_sobel_gy", sobel_gy(img));
    lot_ajouter(&lot, "c05_sobel_norme", sobel_norme(img));
    lot_ajouter(&lot, "c05_sobel_norme_l1", sobel_norme_l1(img));
    lot_ajouter(&lot, "c05_sobel_direction", sobel_direction(img));
    lot_ajouter(&lot, "c05_sobel_seuil25", sobel_seuillage(img, 25));
    lot_ajouter(&lot, "c05_sobel_seuil60", sobel_seuillage(img, 60));
    traiter_lot(&lot, dossier, nom, "Sobel");
}

static void groupe_c05_isotropique(ImageGris *img, const char *nom,
                                    const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtre isotropique");
    lot_ajouter(&lot, "c05_iso_gx", isotropique_gx(img));
    lot_ajouter(&lot, "c05_iso_gy", isotropique_gy(img));
    lot_ajouter(&lot, "c05_iso_norme", isotropique_norme(img));
    traiter_lot(&lot, dossier, nom, "Isotropique");
}

static void groupe_c05_laplacien(ImageGris *img, const char *nom,
                                  const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Laplacien / LoG / zero-crossing");
    lot_ajouter(&lot, "c05_laplacien4", laplacien_4(img));
    lot_ajouter(&lot, "c05_laplacien8", laplacien_8(img));
    lot_ajouter(&lot, "c05_laplacien_absolu", laplacien_absolu(img));
    lot_ajouter(&lot, "c05_zero_crossing", zero_crossing(img));
    lot_ajouter(&lot, "c05_log_sigma1", laplacien_of_gaussian(img, 1.0));
    lot_ajouter(&lot, "c05_log_sigma2", laplacien_of_gaussian(img, 2.0));
    lot_ajouter(&lot, "c05_zc_log_sigma1", zero_crossing_log(img, 1.0));
    lot_ajouter(&lot, "c05_zc_log_sigma2", zero_crossing_log(img, 2.0));
    lot_ajouter(&lot, "c05_rehaussement_laplacien", rehaussement_laplacien(img));
    traiter_lot(&lot, dossier, nom, "Laplacien");
}

/* ============================================================
   GROUPES D'OPERATIONS - COURS 04 : TRANSFORMEE DE FOURIER
   ============================================================ */

static void groupe_c04_verification(ImageGris *img, const char *nom,
                                     const char *dossier)
{
    (void)dossier;
    titre("Verification FFT inverse");
    printf("  [%s] FFT directe puis inverse : %s\n", nom,
           verifier_fft_inverse(img) ? "OK (image reconstruite)"
                                      : "ERREUR");
}

static void groupe_c04_spectre(ImageGris *img, const char *nom,
                                const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Spectre de Fourier");
    lot_ajouter(&lot, "c04_spectre", spectre_fourier(img));
    lot_ajouter(&lot, "c04_spectre_log_rehausse", spectre_rehausse(img));
    traiter_lot(&lot, dossier, nom, "Spectre");
}

static void groupe_c04_passebas(ImageGris *img, const char *nom,
                                 const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtrage frequentiel passe-bas");
    lot_ajouter(&lot, "c04_passebas_10pct", filtre_passe_bas_fft(img, 10.0));
    lot_ajouter(&lot, "c04_passebas_20pct", filtre_passe_bas_fft(img, 20.0));
    lot_ajouter(&lot, "c04_passebas_30pct", filtre_passe_bas_fft(img, 30.0));
    lot_ajouter(&lot, "c04_spectre_passebas_10pct", spectre_passe_bas(img, 10.0));
    lot_ajouter(&lot, "c04_spectre_passebas_30pct", spectre_passe_bas(img, 30.0));
    traiter_lot(&lot, dossier, nom, "Passe-bas");
}

static void groupe_c04_passehaut(ImageGris *img, const char *nom,
                                  const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtrage frequentiel passe-haut");
    lot_ajouter(&lot, "c04_passehaut_5pct", filtre_passe_haut_fft(img, 5.0));
    lot_ajouter(&lot, "c04_passehaut_15pct", filtre_passe_haut_fft(img, 15.0));
    lot_ajouter(&lot, "c04_passehaut_30pct", filtre_passe_haut_fft(img, 30.0));
    lot_ajouter(&lot, "c04_spectre_passehaut_15pct", spectre_passe_haut(img, 15.0));
    traiter_lot(&lot, dossier, nom, "Passe-haut");
}

static void groupe_c04_rehaussement(ImageGris *img, const char *nom,
                                     const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Rehaussement de contraste par FFT");
    lot_ajouter(&lot, "c04_rehaus_5pct", rehaussement_fft(img, 5.0, 1.0));
    lot_ajouter(&lot, "c04_rehaus_15pct", rehaussement_fft(img, 15.0, 1.0));
    lot_ajouter(&lot, "c04_rehaus_30pct", rehaussement_fft(img, 30.0, 1.0));
    traiter_lot(&lot, dossier, nom, "Rehaussement FFT");
}

static void groupe_c04_notch(ImageGris *img, const char *nom,
                              const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Filtre Notch (suppression de bruit periodique)");
    lot_ajouter(&lot, "c04_notch_u20_v0_r3", filtre_notch(img, 20, 0, 3.0));
    traiter_lot(&lot, dossier, nom, "Filtre Notch");
}

/* ============================================================
   GROUPES D'OPERATIONS - COURS 07 : IMAGES BINAIRES
   ============================================================ */

static void groupe_c07_binarisation(ImageGris *img, const char *nom,
                                     const char *dossier)
{
    Lot lot; lot_init(&lot);
    int s = otsu_seuil(img);
    titre("Binarisation");
    printf("  Seuil Otsu calcule automatiquement : %d\n", s);
    lot_ajouter(&lot, "c07_binaire_seuil128", binariser(img, 128));
    lot_ajouter(&lot, "c07_binaire_otsu", binariser(img, s));
    traiter_lot(&lot, dossier, nom, "Binarisation");
}

static void groupe_c07_distances(ImageGris *img, const char *nom,
                                  const char *dossier)
{
    Lot lot; lot_init(&lot);
    ImageGris *bin = binariser(img, 128);
    titre("Cartes de distances discretes");
    lot_ajouter(&lot, "c07_distance_D4", carte_distance_D4(bin));
    lot_ajouter(&lot, "c07_distance_D8", carte_distance_D8(bin));
    liberer_image_gris(bin);
    traiter_lot(&lot, dossier, nom, "Cartes de distances");
}

static void groupe_c07_freeman(ImageGris *img, const char *nom,
                                const char *dossier)
{
    Lot lot; lot_init(&lot);
    ImageGris *bin = binariser(img, 128);
    titre("Codage de Freeman");
    lot_ajouter(&lot, "c07_freeman", visualiser_freeman(bin));
    liberer_image_gris(bin);
    traiter_lot(&lot, dossier, nom, "Freeman");
}

static void groupe_c07_etiquetage(ImageGris *img, const char *nom,
                                   const char *dossier)
{
    Lot lot; lot_init(&lot);
    ImageGris *bin = binariser(img, 128);
    int nb4 = 0, nb8 = 0;
    titre("Etiquetage de composantes connexes");
    lot_ajouter(&lot, "c07_etiquetage_4connexe", etiqueter_composantes(bin, 4, &nb4));
    lot_ajouter(&lot, "c07_etiquetage_8connexe", etiqueter_composantes(bin, 8, &nb8));
    printf("  Nombre de composantes : 4-connexite=%d   8-connexite=%d\n", nb4, nb8);
    liberer_image_gris(bin);
    traiter_lot(&lot, dossier, nom, "Etiquetage");
}

static void groupe_c07_morphologie(ImageGris *img, const char *nom,
                                    const char *dossier)
{
    Lot lot; lot_init(&lot);
    ImageGris *bin = binariser(img, 128);
    titre("Morphologie binaire");
    lot_ajouter(&lot, "c07_erosion3", erosion(bin, 3));
    lot_ajouter(&lot, "c07_erosion5", erosion(bin, 5));
    lot_ajouter(&lot, "c07_dilatation3", dilatation(bin, 3));
    lot_ajouter(&lot, "c07_dilatation5", dilatation(bin, 5));
    lot_ajouter(&lot, "c07_ouverture3", ouverture(bin, 3));
    lot_ajouter(&lot, "c07_fermeture3", fermeture(bin, 3));
    lot_ajouter(&lot, "c07_ouverture5", ouverture(bin, 5));
    lot_ajouter(&lot, "c07_fermeture5", fermeture(bin, 5));
    liberer_image_gris(bin);
    traiter_lot(&lot, dossier, nom, "Morphologie binaire");
}

static void groupe_c07_gradients(ImageGris *img, const char *nom,
                                  const char *dossier)
{
    Lot lot; lot_init(&lot);
    ImageGris *bin = binariser(img, 128);
    titre("Gradients morphologiques binaires");
    lot_ajouter(&lot, "c07_grad_interne3", gradient_interne(bin, 3));
    lot_ajouter(&lot, "c07_grad_externe3", gradient_externe(bin, 3));
    lot_ajouter(&lot, "c07_grad_morpho3", gradient_morphologique(bin, 3));
    liberer_image_gris(bin);
    traiter_lot(&lot, dossier, nom, "Gradients binaires");
}

static void groupe_c07_morphologie_gris(ImageGris *img, const char *nom,
                                         const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Morphologie en niveaux de gris");
    lot_ajouter(&lot, "c07_erosion_gris3", erosion_gris(img, 3));
    lot_ajouter(&lot, "c07_erosion_gris5", erosion_gris(img, 5));
    lot_ajouter(&lot, "c07_dilatation_gris3", dilatation_gris(img, 3));
    lot_ajouter(&lot, "c07_dilatation_gris5", dilatation_gris(img, 5));
    lot_ajouter(&lot, "c07_ouverture_gris3", ouverture_gris(img, 3));
    lot_ajouter(&lot, "c07_fermeture_gris3", fermeture_gris(img, 3));
    lot_ajouter(&lot, "c07_grad_morpho_gris3", gradient_morphologique_gris(img, 3));
    traiter_lot(&lot, dossier, nom, "Morphologie niveaux de gris");
}

static void groupe_c07_hysteresis(ImageGris *img, const char *nom,
                                   const char *dossier)
{
    Lot lot; lot_init(&lot);
    ImageGris *gsob = sobel_norme(img);
    titre("Seuillage par hysteresis (sur gradient Sobel)");
    lot_ajouter(&lot, "c07_hysteresis_30_80", seuillage_hysteresis(gsob, 30, 80));
    lot_ajouter(&lot, "c07_hysteresis_20_60", seuillage_hysteresis(gsob, 20, 60));
    liberer_image_gris(gsob);
    traiter_lot(&lot, dossier, nom, "Hysteresis");
}

/* ============================================================
   GROUPES D'OPERATIONS - COURS 06 : SEGMENTATION
   ============================================================ */

static void groupe_c06_seuillage_global(ImageGris *img, const char *nom,
                                         const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Seuillage global");
    lot_ajouter(&lot, "c06_seuil64", seuillage_global(img, 64));
    lot_ajouter(&lot, "c06_seuil128", seuillage_global(img, 128));
    lot_ajouter(&lot, "c06_seuil192", seuillage_global(img, 192));
    traiter_lot(&lot, dossier, nom, "Seuillage global");
}

static void groupe_c06_multiclasses(ImageGris *img, const char *nom,
                                     const char *dossier)
{
    Lot lot; lot_init(&lot);
    const int s3[2] = {85, 170};
    const int s4[3] = {64, 128, 192};
    titre("Seuillage multi-classes");
    lot_ajouter(&lot, "c06_multiclasses_3", seuillage_multiple(img, s3, 2));
    lot_ajouter(&lot, "c06_multiclasses_4", seuillage_multiple(img, s4, 3));
    traiter_lot(&lot, dossier, nom, "Seuillage multi-classes");
}

static void groupe_c06_otsu(ImageGris *img, const char *nom,
                             const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Seuillage d'Otsu");
    printf("  Seuil Otsu calcule : %d\n", otsu_seuil(img));
    lot_ajouter(&lot, "c06_otsu", seuillage_otsu(img));
    traiter_lot(&lot, dossier, nom, "Otsu");
}

static void groupe_c06_adaptatif(ImageGris *img, const char *nom,
                                  const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Seuillage adaptatif");
    lot_ajouter(&lot, "c06_adaptatif_f15_o0", seuillage_adaptatif(img, 15, 0));
    lot_ajouter(&lot, "c06_adaptatif_f15_o10", seuillage_adaptatif(img, 15, 10));
    lot_ajouter(&lot, "c06_adaptatif_f31_o5", seuillage_adaptatif(img, 31, 5));
    lot_ajouter(&lot, "c06_adaptatif_variance_f15", seuillage_adaptatif_variance(img, 15, 100.0));
    lot_ajouter(&lot, "c06_adaptatif_variance_f31", seuillage_adaptatif_variance(img, 31, 100.0));
    traiter_lot(&lot, dossier, nom, "Seuillage adaptatif");
}

static void groupe_c06_kmoyennes(ImageGris *img, const char *nom,
                                  const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Segmentation par K-moyennes");
    lot_ajouter(&lot, "c06_kmoyennes_k2", kmoyennes(img, 2, 50));
    lot_ajouter(&lot, "c06_kmoyennes_k3", kmoyennes(img, 3, 50));
    lot_ajouter(&lot, "c06_kmoyennes_k4", kmoyennes(img, 4, 50));
    lot_ajouter(&lot, "c06_kmoyennes_k5", kmoyennes(img, 5, 50));
    traiter_lot(&lot, dossier, nom, "K-moyennes");
}

static void groupe_c06_division_fusion(ImageGris *img, const char *nom,
                                        const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Division-Fusion (Split & Merge)");
    lot_ajouter(&lot, "c06_divfusion_var10", division_fusion(img, 10));
    lot_ajouter(&lot, "c06_divfusion_var30", division_fusion(img, 30));
    lot_ajouter(&lot, "c06_divfusion_var50", division_fusion(img, 50));
    lot_ajouter(&lot, "c06_divfusion_bords_var10", division_fusion_bords(img, 10));
    lot_ajouter(&lot, "c06_divfusion_bords_var30", division_fusion_bords(img, 30));
    traiter_lot(&lot, dossier, nom, "Division-Fusion");
}

static void groupe_c06_croissance_region(ImageGris *img, const char *nom,
                                          const char *dossier)
{
    Lot lot; lot_init(&lot);
    int cx = img->largeur / 2, cy = img->hauteur / 2;
    titre("Croissance de region (germe au centre)");
    printf("  Germe : (%d, %d)\n", cx, cy);
    lot_ajouter(&lot, "c06_croissance_tol10", croissance_region(img, cx, cy, 10));
    lot_ajouter(&lot, "c06_croissance_tol20", croissance_region(img, cx, cy, 20));
    lot_ajouter(&lot, "c06_croissance_tol40", croissance_region(img, cx, cy, 40));
    traiter_lot(&lot, dossier, nom, "Croissance de region");
}

static void groupe_c06_croissance_multi(ImageGris *img, const char *nom,
                                         const char *dossier)
{
    Lot lot; lot_init(&lot);
    titre("Croissance multi-germes");
    lot_ajouter(&lot, "c06_multigermes_4x4_tol20", croissance_multi_germes(img, 4, 4, 20));
    lot_ajouter(&lot, "c06_multigermes_6x6_tol15", croissance_multi_germes(img, 6, 6, 15));
    lot_ajouter(&lot, "c06_multigermes_8x8_tol25", croissance_multi_germes(img, 8, 8, 25));
    traiter_lot(&lot, dossier, nom, "Croissance multi-germes");
}

/* ============================================================
   MOTEUR DE MENU GENERIQUE
   ============================================================ */
typedef void (*GroupeFn)(ImageGris *, const char *, const char *);
typedef struct { const char *titre; GroupeFn fn; } GroupeMenu;

static void executer_menu_cours(const char *titre_cours,
                                 GroupeMenu *groupes, int nb_groupes,
                                 ImageGris *img, const char *nom_image,
                                 const char *dossier)
{
    int choix, i;
    while (1) {
        titre(titre_cours);
        printf("  Image en cours : %s (%dx%d)\n\n",
               nom_image, img->largeur, img->hauteur);
        for (i = 0; i < nb_groupes; i++)
            printf("   %2d. %s\n", i + 1, groupes[i].titre);
        printf("   %2d. Executer TOUS les groupes de ce cours\n",
               nb_groupes + 1);
        printf("    0. Retour au menu precedent\n");
        choix = lire_choix_int("\n  Votre choix : ", 0, nb_groupes + 1);
        if (g_entree_terminee || choix == 0) return;
        if (choix == nb_groupes + 1) {
            for (i = 0; i < nb_groupes; i++)
                groupes[i].fn(img, nom_image, dossier);
            printf("\n  [OK] Tous les groupes de '%s' ont ete executes.\n",
                   titre_cours);
            continue;
        }
        groupes[choix - 1].fn(img, nom_image, dossier);
    }
}

/* ============================================================
   LANCEURS PAR COURS
   ============================================================ */

static void lancer_cours03(ImageGris *img, const char *nom)
{
    GroupeMenu g[] = {
        {"Filtre moyenneur",                groupe_c03_moyenneur},
        {"Filtre gaussien",                 groupe_c03_gaussien},
        {"Filtre median",                   groupe_c03_median},
        {"Filtres min / max",               groupe_c03_minmax},
        {"Noyau gaussien exact du cours",   groupe_c03_noyau_cours},
        {"Bruit poivre&sel + debruitage",   groupe_c03_bruit},
    };
    executer_menu_cours("COURS 03 - CONVOLUTION", g,
                         (int)(sizeof(g)/sizeof(g[0])), img, nom, DIR_C03);
}

static void lancer_cours04(ImageGris *img, const char *nom)
{
    GroupeMenu g[] = {
        {"Verification FFT inverse (terminal)", groupe_c04_verification},
        {"Spectre de Fourier",                  groupe_c04_spectre},
        {"Filtrage passe-bas",                  groupe_c04_passebas},
        {"Filtrage passe-haut",                 groupe_c04_passehaut},
        {"Rehaussement de contraste (FFT)",     groupe_c04_rehaussement},
        {"Filtre Notch (bruit periodique)",     groupe_c04_notch},
    };
    executer_menu_cours("COURS 04 - TRANSFORMEE DE FOURIER", g,
                         (int)(sizeof(g)/sizeof(g[0])), img, nom, DIR_C04);
}

static void lancer_cours05(ImageGris *img, const char *nom)
{
    GroupeMenu g[] = {
        {"Derivee discrete simple",  groupe_c05_derivee},
        {"Filtre de Roberts",        groupe_c05_roberts},
        {"Filtre de Prewitt",        groupe_c05_prewitt},
        {"Filtre de Sobel",          groupe_c05_sobel},
        {"Filtre isotropique",       groupe_c05_isotropique},
        {"Laplacien / LoG",          groupe_c05_laplacien},
    };
    executer_menu_cours("COURS 05 - DETECTION DE CONTOURS", g,
                         (int)(sizeof(g)/sizeof(g[0])), img, nom, DIR_C05);
}

static void lancer_cours06(ImageGris *img, const char *nom)
{
    GroupeMenu g[] = {
        {"Seuillage global",         groupe_c06_seuillage_global},
        {"Seuillage multi-classes",  groupe_c06_multiclasses},
        {"Seuillage d'Otsu",         groupe_c06_otsu},
        {"Seuillage adaptatif",      groupe_c06_adaptatif},
        {"K-moyennes",               groupe_c06_kmoyennes},
        {"Division-Fusion",          groupe_c06_division_fusion},
        {"Croissance de region",     groupe_c06_croissance_region},
        {"Croissance multi-germes",  groupe_c06_croissance_multi},
    };
    executer_menu_cours("COURS 06 - SEGMENTATION", g,
                         (int)(sizeof(g)/sizeof(g[0])), img, nom, DIR_C06);
}

static void lancer_cours07(ImageGris *img, const char *nom)
{
    GroupeMenu g[] = {
        {"Binarisation (seuil fixe + Otsu)",   groupe_c07_binarisation},
        {"Cartes de distances D4 / D8",        groupe_c07_distances},
        {"Codage de Freeman",                  groupe_c07_freeman},
        {"Etiquetage composantes connexes",    groupe_c07_etiquetage},
        {"Morphologie binaire",                groupe_c07_morphologie},
        {"Gradients morphologiques binaires",  groupe_c07_gradients},
        {"Morphologie niveaux de gris",        groupe_c07_morphologie_gris},
        {"Seuillage par hysteresis",           groupe_c07_hysteresis},
    };
    executer_menu_cours("COURS 07 - IMAGES BINAIRES", g,
                         (int)(sizeof(g)/sizeof(g[0])), img, nom, DIR_C07);
}

static void lancer_cours02(ImageGris *imgs[], const char *noms[],
                            const char *fichiers[], int nb)
{
    int choix, i;
    int max_choix = 3;
    int idx_couleur = -1, idx_arith = -1, idx_logique = -1;

    for (i = 0; i < nb; i++) {
        if (detecter_format(fichiers[i]) == FORMAT_PPM) {
            idx_couleur = ++max_choix;
            break;
        }
    }
    if (nb == 2) {
        idx_arith   = ++max_choix;
        idx_logique = ++max_choix;
    }

    while (1) {
        titre("COURS 02 - TRAITEMENTS DE BASE");
        for (i = 0; i < nb; i++)
            printf("  Image selectionnee : %s (%dx%d)\n",
                   noms[i], imgs[i]->largeur, imgs[i]->hauteur);
        printf("\n    1. Statistiques & Histogramme graphique\n");
        printf("    2. Transformations de contraste\n");
        printf("    3. Interpolation / Zoom\n");
        if (idx_couleur > 0)
            printf("    %d. Traitement couleur canal par canal (PPM)\n",
                   idx_couleur);
        if (idx_arith > 0)
            printf("    %d. Operations arithmetiques entre les 2 images\n",
                   idx_arith);
        if (idx_logique > 0)
            printf("    %d. Operations logiques entre les 2 images\n",
                   idx_logique);
        printf("    %d. Executer TOUS les groupes applicables\n",
               max_choix + 1);
        printf("    0. Retour au menu precedent\n");

        choix = lire_choix_int("\n  Votre choix : ", 0, max_choix + 1);
        if (g_entree_terminee || choix == 0) return;

        if (choix == max_choix + 1) {
            for (i = 0; i < nb; i++) {
                groupe_c02_statistiques(imgs[i], noms[i], DIR_C02);
                groupe_c02_contraste(imgs[i], noms[i], DIR_C02);
                groupe_c02_interpolation(imgs[i], noms[i], DIR_C02);
                if (idx_couleur > 0 &&
                    detecter_format(fichiers[i]) == FORMAT_PPM)
                    groupe_c02_couleur(fichiers[i], noms[i], DIR_C02);
            }
            if (idx_arith > 0) {
                groupe_c02_arithmetique(imgs[0], imgs[1], noms[0], noms[1], DIR_C02);
                groupe_c02_logique(imgs[0], imgs[1], noms[0], noms[1], DIR_C02);
            }
            printf("\n  [OK] Cours 02 execute pour la selection.\n");
            continue;
        }

        if (choix == 1)
            for (i = 0; i < nb; i++)
                groupe_c02_statistiques(imgs[i], noms[i], DIR_C02);
        else if (choix == 2)
            for (i = 0; i < nb; i++)
                groupe_c02_contraste(imgs[i], noms[i], DIR_C02);
        else if (choix == 3)
            for (i = 0; i < nb; i++)
                groupe_c02_interpolation(imgs[i], noms[i], DIR_C02);
        else if (choix == idx_couleur) {
            for (i = 0; i < nb; i++)
                if (detecter_format(fichiers[i]) == FORMAT_PPM)
                    groupe_c02_couleur(fichiers[i], noms[i], DIR_C02);
        } else if (choix == idx_arith) {
            groupe_c02_arithmetique(imgs[0], imgs[1], noms[0], noms[1], DIR_C02);
        } else if (choix == idx_logique) {
            groupe_c02_logique(imgs[0], imgs[1], noms[0], noms[1], DIR_C02);
        }
    }
}

/* ============================================================
   MENU : IMAGES IMPORTEES (tank.pgm / sailboat.ppm)
   ============================================================ */
static void menu_images_importees(ImageGris *tank, ImageGris *sail)
{
    ImageGris *imgs[2];
    const char *noms[2];
    const char *fichiers[2];
    int nb, choix_img, choix_cours, i;

    while (1) {
        titre("IMAGES IMPORTEES - Selection");
        printf("    1. tank.pgm      (image reelle, PGM)\n");
        printf("    2. sailboat.ppm  (image reelle, PPM)\n");
        printf("    3. Les deux (necessaire pour les operations par paires)\n");
        printf("    0. Retour au menu principal\n");
        choix_img = lire_choix_int("\n  Votre choix : ", 0, 3);
        if (g_entree_terminee || choix_img == 0) return;

        nb = (choix_img == 3) ? 2 : 1;
        if (choix_img == 1) {
            imgs[0] = tank; noms[0] = "tank"; fichiers[0] = FICHIER_TANK;
        } else if (choix_img == 2) {
            imgs[0] = sail; noms[0] = "sailboat"; fichiers[0] = FICHIER_SAIL;
        } else {
            imgs[0] = tank; noms[0] = "tank"; fichiers[0] = FICHIER_TANK;
            imgs[1] = sail; noms[1] = "sailboat"; fichiers[1] = FICHIER_SAIL;
        }

        while (1) {
            titre("Choix du cours a appliquer");
            for (i = 0; i < nb; i++)
                printf("  Image selectionnee : %s\n", noms[i]);
            printf("\n    1. Cours 02 - Traitements de base\n");
            printf("    2. Cours 03 - Convolution\n");
            printf("    3. Cours 04 - Transformee de Fourier\n");
            printf("    4. Cours 05 - Detection de contours\n");
            printf("    5. Cours 06 - Segmentation\n");
            printf("    6. Cours 07 - Images binaires\n");
            printf("    0. Changer d'image\n");
            choix_cours = lire_choix_int("\n  Votre choix : ", 0, 6);
            if (choix_cours == 0) break;
            if (g_entree_terminee) return;

            switch (choix_cours) {
            case 1: lancer_cours02(imgs, noms, fichiers, nb); break;
            case 2: for (i = 0; i < nb; i++) lancer_cours03(imgs[i], noms[i]); break;
            case 3: for (i = 0; i < nb; i++) lancer_cours04(imgs[i], noms[i]); break;
            case 4: for (i = 0; i < nb; i++) lancer_cours05(imgs[i], noms[i]); break;
            case 5: for (i = 0; i < nb; i++) lancer_cours06(imgs[i], noms[i]); break;
            case 6: for (i = 0; i < nb; i++) lancer_cours07(imgs[i], noms[i]); break;
            }
        }
    }
}

/* ============================================================
   MENU : TEXTURE PERIODIQUE SYNTHETIQUE (demo Fourier / Notch)
   ============================================================ */
static void menu_texture_fourier(ImageGris *texture)
{
    titre("TEXTURE PERIODIQUE SYNTHETIQUE - Cours Fourier");
    printf("  Cette image est generee mathematiquement (deux sinusoides\n"
           "  combinees) : elle produit un spectre de Fourier avec des\n"
           "  pics nets, ideal pour valider le filtre coupe-bande (Notch)\n"
           "  et pour observer clairement l'effet des filtres passe-bas /\n"
           "  passe-haut. Elle a ete sauvegardee dans :\n"
           "    " DIR_ENTREES "texture_fourier_synthetique.pgm\n");
    lancer_cours04(texture, "texture_synthetique");
}

/* ============================================================
   BANNIERE / MENU PRINCIPAL
   ============================================================ */
static void banniere(void)
{
    printf("\n");
    printf("========================================================\n");
    printf("        TRAITEMENT D'IMAGES - INTERFACE INTERACTIVE\n");
    printf("   Cours 02-03-04-05-06-07 : Base, Convolution, Fourier,\n");
    printf("        Contours, Segmentation, Images binaires\n");
    printf("========================================================\n");
}

int main(void)
{
    ImageGris *tank, *sail, *texture;
    int choix;

    creer_dossiers();
    banniere();

    tank = charger_image(FICHIER_TANK);
    sail = charger_image(FICHIER_SAIL);
    if (!tank || !sail) {
        fprintf(stderr,
            "\n[ERREUR] Impossible de charger les images d'entree.\n"
            "Merci de placer :\n"
            "  - tank.pgm      dans le dossier '%s'\n"
            "  - sailboat.ppm  dans le dossier '%s'\n",
            SRC_DIR, SRC_DIR);
        if (tank) liberer_image_gris(tank);
        if (sail) liberer_image_gris(sail);
        return 1;
    }

    copier_fichier_brut(FICHIER_TANK, DIR_ENTREES "tank.pgm");
    copier_fichier_brut(FICHIER_SAIL, DIR_ENTREES "sailboat.ppm");

    texture = generer_texture_periodique(256, 256, 8.0, 5.0);
    ecrire_pgm(texture, DIR_ENTREES "texture_fourier_synthetique.pgm");

    printf("\nImages chargees :\n");
    printf("  - tank.pgm       %dx%d\n", tank->largeur, tank->hauteur);
    printf("  - sailboat.ppm   %dx%d (convertie en gris pour les calculs)\n",
           sail->largeur, sail->hauteur);
    printf("  - texture Fourier %dx%d (generee automatiquement)\n",
           texture->largeur, texture->hauteur);
    printf("\nDossiers de sortie prepares sous '%s' :\n", DIR_RESULTATS);
    printf("  00_images_entree/  cours02_traitements_base/  "
           "cours03_convolution/\n");
    printf("  cours04_fourier/   cours05_contours/           "
           "cours06_segmentation/\n");
    printf("  cours07_images_binaires/\n");

    while (1) {
        titre("MENU PRINCIPAL");
        printf("    1. Travailler sur une image importee (PGM/PPM reelle)\n");
        printf("    2. Travailler sur la texture periodique synthetique"
               " (demo Fourier)\n");
        printf("    3. Quitter\n");
        choix = lire_choix_int("\n  Votre choix : ", 1, 3);
        if (g_entree_terminee) break;
        if (choix == 1) menu_images_importees(tank, sail);
        else if (choix == 2) menu_texture_fourier(texture);
        else break;
    }

    liberer_image_gris(tank);
    liberer_image_gris(sail);
    liberer_image_gris(texture);

    printf("\nTermine. Resultats ranges dans : %s\n", DIR_RESULTATS);
    printf("Au revoir !\n");
    return 0;
}