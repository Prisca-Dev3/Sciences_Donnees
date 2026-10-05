#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

/* ============================================================
   STRUCTURES
   ============================================================ */

typedef struct {
    int largeur;
    int hauteur;
    int max_val;
    unsigned char *pixels;          /* pixels[y * largeur + x] */
} ImageGris;

typedef struct {
    int largeur;
    int hauteur;
    int max_val;
    unsigned char *r;
    unsigned char *g;
    unsigned char *b;
} ImageCouleur;

/* ============================================================
   MACROS
   ============================================================ */

#define PIXEL(img, x, y)   ((img)->pixels[(y)*(img)->largeur+(x)])
#define PIXEL_R(img, x, y) ((img)->r[(y)*(img)->largeur+(x)])
#define PIXEL_G(img, x, y) ((img)->g[(y)*(img)->largeur+(x)])
#define PIXEL_B(img, x, y) ((img)->b[(y)*(img)->largeur+(x)])

#define CLAMP(v,lo,hi) ((v)<(lo)?(lo):((v)>(hi)?(hi):(v)))
#define MIN2(a,b)      ((a)<(b)?(a):(b))
#define MAX2(a,b)      ((a)>(b)?(a):(b))

/* ============================================================
   CHARGEMENT UNIVERSEL
   Detecte automatiquement PGM ou PPM, retourne toujours
   une ImageGris (conversion automatique si couleur)
   et optionnellement l'ImageCouleur originale.
   ============================================================ */
typedef enum { FORMAT_INCONNU, FORMAT_PGM, FORMAT_PPM } FormatImage;

FormatImage   detecter_format(const char *fichier);
ImageGris    *charger_image(const char *fichier);          /* PGM ou PPM->gris */
ImageCouleur *charger_image_couleur(const char *fichier);  /* PPM uniquement   */

/* ============================================================
   IO PGM / PPM
   ============================================================ */
ImageGris    *lire_pgm(const char *fichier);
int           ecrire_pgm(const ImageGris *img, const char *fichier);
ImageCouleur *lire_ppm(const char *fichier);
int           ecrire_ppm(const ImageCouleur *img, const char *fichier);

/* ============================================================
   ALLOCATION / COPIE / LIBERATION
   ============================================================ */
ImageGris    *allouer_image_gris(int largeur, int hauteur);
ImageCouleur *allouer_image_couleur(int largeur, int hauteur);
void          liberer_image_gris(ImageGris *img);
void          liberer_image_couleur(ImageCouleur *img);
ImageGris    *copier_image_gris(const ImageGris *src);
ImageCouleur *copier_image_couleur(const ImageCouleur *src);

/* ============================================================
   CONVERSION
   ============================================================ */
ImageGris    *couleur_vers_gris(const ImageCouleur *src);
ImageCouleur *gris_vers_couleur(const ImageGris *src);     /* R=G=B=gris */

/* ============================================================
   OPERATIONS COULEUR CANAL PAR CANAL
   Permettent d'appliquer n'importe quelle fonction gris
   sur chaque canal R, G, B d'une image couleur.
   ============================================================ */
/* Extrait un canal (0=R, 1=G, 2=B) comme ImageGris */
ImageGris    *extraire_canal(const ImageCouleur *src, int canal);
/* Recompose une ImageCouleur depuis 3 canaux gris */
ImageCouleur *assembler_canaux(const ImageGris *r,
                                const ImageGris *g,
                                const ImageGris *b);
/* Applique une fonction f(ImageGris)->ImageGris sur chaque canal */
typedef ImageGris *(*FonctionGris)(const ImageGris *);
ImageCouleur *appliquer_sur_canaux(const ImageCouleur *src,
                                    FonctionGris f);

/* ============================================================
   COMPATIBILITE ENTRE IMAGES
   Verifie ou redimensionne pour rendre deux images compatibles
   ============================================================ */
int           meme_dimensions(const ImageGris *a, const ImageGris *b);
ImageGris    *redimensionner_compatible(const ImageGris *src,
                                         int cible_w, int cible_h);

/* ============================================================
   COURS 02 - TRAITEMENTS DE BASE
   ============================================================ */
void       calculer_histogramme(const ImageGris *img, int hist[256]);
void       afficher_histogramme(const int hist[256]);
double     calculer_luminance(const ImageGris *img);
double     calculer_contraste_ecarttype(const ImageGris *img);
double     calculer_contraste_minmax(const ImageGris *img);

ImageGris *transformation_lineaire(const ImageGris *img);
ImageGris *transformation_lineaire_saturation(const ImageGris *img,
                                               int smin, int smax);
ImageGris *transformation_lineaire_morceaux(const ImageGris *img,
                                             int smin, int smax, int s);
ImageGris *correction_gamma(const ImageGris *img, double gamma);
ImageGris *inversion(const ImageGris *img);
ImageGris *seuillage(const ImageGris *img, int seuil);

ImageGris *egalisation_histogramme(const ImageGris *img);
ImageGris *egalisation_locale(const ImageGris *img, int taille_fenetre);

/* Arithmetique : deux images gris (memes dimensions obligatoires) */
ImageGris *addition_images(const ImageGris *a, const ImageGris *b);
ImageGris *soustraction_images(const ImageGris *a, const ImageGris *b);
ImageGris *multiplication_images(const ImageGris *a, const ImageGris *b);
ImageGris *multiplication_image(const ImageGris *img, double ratio);
ImageGris *fusion_images(const ImageGris *a, const ImageGris *b,
                          double alpha);

/* Logique */
ImageGris *et_images(const ImageGris *a, const ImageGris *b);
ImageGris *ou_images(const ImageGris *a, const ImageGris *b);
ImageGris *xor_images(const ImageGris *a, const ImageGris *b);
ImageGris *non_image(const ImageGris *img);
ImageGris *non_et_images(const ImageGris *a, const ImageGris *b);

/* Interpolation */
ImageGris *zoom_plus_proche_voisin(const ImageGris *img,
                                    double fx, double fy);
ImageGris *zoom_bilineaire(const ImageGris *img, double fx, double fy);
ImageGris *zoom_bicubique(const ImageGris *img, double fx, double fy);

void profil_intensite(const ImageGris *img, int ligne,
                       int *profil, int *nb_points);
void profil_intensite_colonne(const ImageGris *img, int colonne,
                               int *profil, int *nb_points);

/* ============================================================
   COURS 03 - CONVOLUTION
   ============================================================ */
ImageGris *convolution_2d(const ImageGris *img,
                           const double *noyau, int taille);
ImageGris *filtre_moyenneur(const ImageGris *img, int taille);
ImageGris *filtre_gaussien(const ImageGris *img, int taille, double sigma);
ImageGris *filtre_median(const ImageGris *img, int taille);
ImageGris *filtre_min(const ImageGris *img, int taille);
ImageGris *filtre_max(const ImageGris *img, int taille);

/* ============================================================
   COURS 05 - DETECTION DE CONTOURS
   ============================================================ */

/* Derivee discrete simple */
ImageGris *derivee_discrete_x(const ImageGris *img);
ImageGris *derivee_discrete_y(const ImageGris *img);
ImageGris *derivee_discrete_norme(const ImageGris *img);

/* Filtre de Roberts */
ImageGris *roberts_gx(const ImageGris *img);
ImageGris *roberts_gy(const ImageGris *img);
ImageGris *roberts_norme(const ImageGris *img);
ImageGris *roberts_norme_euclidienne(const ImageGris *img);
ImageGris *roberts_direction(const ImageGris *img);
ImageGris *roberts_seuillage(const ImageGris *img, int seuil);

/* Filtre de Prewitt */
ImageGris *prewitt_gx(const ImageGris *img);
ImageGris *prewitt_gy(const ImageGris *img);
ImageGris *prewitt_norme(const ImageGris *img);
ImageGris *prewitt_direction(const ImageGris *img);
ImageGris *prewitt_seuillage(const ImageGris *img, int seuil);

/* Filtre de Sobel */
ImageGris *sobel_gx(const ImageGris *img);
ImageGris *sobel_gy(const ImageGris *img);
ImageGris *sobel_norme(const ImageGris *img);
ImageGris *sobel_norme_l1(const ImageGris *img);
ImageGris *sobel_direction(const ImageGris *img);
ImageGris *sobel_seuillage(const ImageGris *img, int seuil);

/* Filtre isotropique */
ImageGris *isotropique_gx(const ImageGris *img);
ImageGris *isotropique_gy(const ImageGris *img);
ImageGris *isotropique_norme(const ImageGris *img);

/* Laplacien */
ImageGris *laplacien_4(const ImageGris *img);
ImageGris *laplacien_8(const ImageGris *img);
ImageGris *laplacien_absolu(const ImageGris *img);
ImageGris *zero_crossing(const ImageGris *img);
ImageGris *laplacien_of_gaussian(const ImageGris *img, double sigma);
ImageGris *zero_crossing_log(const ImageGris *img, double sigma);
ImageGris *rehaussement_laplacien(const ImageGris *img);

/* ============================================================
   COURS 04 - TRANSFORMEE DE FOURIER
   ============================================================ */

/* Spectre */
ImageGris *spectre_fourier(const ImageGris *img);
ImageGris *spectre_rehausse(const ImageGris *img);

/* Filtrage frequentiel */
ImageGris *filtre_passe_bas_fft(const ImageGris *img, double rayon_pct);
ImageGris *filtre_passe_haut_fft(const ImageGris *img, double rayon_pct);
ImageGris *spectre_passe_bas(const ImageGris *img, double rayon_pct);
ImageGris *spectre_passe_haut(const ImageGris *img, double rayon_pct);
ImageGris *filtre_notch(const ImageGris *img,
                         int u0, int v0, double rayon_notch);

/* Rehaussement de contraste par FFT */
ImageGris *rehaussement_fft(const ImageGris *img,
                              double rayon_pct, double alpha);

/* Verification FFT inverse */
int verifier_fft_inverse(const ImageGris *img);

/* ============================================================
   COURS 07 - IMAGES BINAIRES
   ============================================================ */

/* Binarisation */
ImageGris *binariser(const ImageGris *img, int seuil);

/* Connexite */
int connexe_4(int x1, int y1, int x2, int y2);
int connexe_8(int x1, int y1, int x2, int y2);

/* Distances discretes */
int distance_D4(int x1, int y1, int x2, int y2);
int distance_D8(int x1, int y1, int x2, int y2);
ImageGris *carte_distance_D4(const ImageGris *bin);
ImageGris *carte_distance_D8(const ImageGris *bin);

/* Codage de Freeman */
int       coder_freeman(const ImageGris *bin, int *code, int max_code,
                        int *start_x, int *start_y);
ImageGris *visualiser_freeman(const ImageGris *bin);

/* Etiquetage de composantes connexes */
ImageGris *etiqueter_composantes(const ImageGris *bin, int connexite,
                                  int *nb_composantes);

/* Morphologie binaire */
ImageGris *erosion(const ImageGris *bin, int taille);
ImageGris *dilatation(const ImageGris *bin, int taille);
ImageGris *ouverture(const ImageGris *bin, int taille);
ImageGris *fermeture(const ImageGris *bin, int taille);
ImageGris *gradient_interne(const ImageGris *bin, int taille);
ImageGris *gradient_externe(const ImageGris *bin, int taille);
ImageGris *gradient_morphologique(const ImageGris *bin, int taille);

/* Fermeture de contours */
ImageGris *seuillage_hysteresis(const ImageGris *gradient,
                                 int seuil_bas, int seuil_haut);

/* Morphologie niveaux de gris */
ImageGris *erosion_gris(const ImageGris *img, int taille);
ImageGris *dilatation_gris(const ImageGris *img, int taille);
ImageGris *ouverture_gris(const ImageGris *img, int taille);
ImageGris *fermeture_gris(const ImageGris *img, int taille);
ImageGris *gradient_morphologique_gris(const ImageGris *img, int taille);

/* ============================================================
   COURS 06 - SEGMENTATION
   ============================================================ */

/* Seuillage */
ImageGris *seuillage_global(const ImageGris *img, int seuil);
ImageGris *seuillage_multiple(const ImageGris *img,
                               const int *seuils, int nb_seuils);

/* Otsu */
int        otsu_seuil(const ImageGris *img);
ImageGris *seuillage_otsu(const ImageGris *img);

/* Seuillage adaptatif */
ImageGris *seuillage_adaptatif(const ImageGris *img,
                                int taille_fenetre, int offset);
ImageGris *seuillage_adaptatif_variance(const ImageGris *img,
                                         int taille_fenetre,
                                         double variance_min);

/* K-moyennes */
ImageGris *kmoyennes(const ImageGris *img, int k, int max_iter);

/* Division-Fusion */
ImageGris *division_fusion(const ImageGris *img, int seuil_var);
ImageGris *division_fusion_bords(const ImageGris *img, int seuil_var);

/* Croissance de regions */
ImageGris *croissance_region(const ImageGris *img,
                              int germe_x, int germe_y,
                              int tolerance);
ImageGris *croissance_multi_germes(const ImageGris *img,
                                    int nb_germes_x, int nb_germes_y,
                                    int tolerance);

#endif /* IMAGE_H */
