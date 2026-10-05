/* ============================================================
   convolution.c  -  Cours 03 : Convolution et filtrage spatial
   Contenu :
     - Convolution 2D discrete (avec gestion des bords : zero, miroir,
       convolution partielle)
     - Filtre moyenneur (passe-bas)
     - Filtre Gaussien   (passe-bas)
     - Filtre median     (non-lineaire)
     - Filtre min        (erosion)
     - Filtre max        (dilatation)
   ============================================================ */

#include "image.h"

/* ============================================================
   UTILITAIRES INTERNES
   ============================================================ */

/* Comparateur pour qsort (tri croissant d'entiers) */
static int cmp_int(const void *a, const void *b)
{
    return (*(const int *)a) - (*(const int *)b);
}

/*
 * pixel_avec_bord
 * Retourne la valeur du pixel (x,y) avec gestion du bord.
 * mode :  0 = zero padding
 *         1 = miroir (reflexion)
 *         2 = clamping (repeter le pixel de bord)
 */
static int pixel_avec_bord(const ImageGris *img, int x, int y, int mode)
{
    int W = img->largeur;
    int H = img->hauteur;

    if (x >= 0 && x < W && y >= 0 && y < H)
        return PIXEL(img, x, y);

    switch (mode) {
    case 0: /* zero padding */
        return 0;
    case 1: /* miroir f(-x,y) = f(x,y) */
        if (x < 0)  x = -x;
        if (x >= W) x = 2*(W-1) - x;
        if (y < 0)  y = -y;
        if (y >= H) y = 2*(H-1) - y;
        /* Clamp de securite apres miroir */
        x = CLAMP(x, 0, W-1);
        y = CLAMP(y, 0, H-1);
        return PIXEL(img, x, y);
    case 2: /* clamping */
    default:
        x = CLAMP(x, 0, W-1);
        y = CLAMP(y, 0, H-1);
        return PIXEL(img, x, y);
    }
}

/* ============================================================
   CONVOLUTION 2D GENERALE
   ============================================================ */

/*
 * convolution_2d
 * Applique un noyau carre de taille 'taille' (impaire recommandee selon le support de cours)
 * a l'image.
 *
 * I'(i,j) = sum_u sum_v  I(i-u, j-v) * filtre(u,v)
 *
 * Le resultat est divise par la somme des coefficients du noyau
 * pour eviter de modifier la luminance globale (si somme != 0).
 *
 * Gestion des bords : mode miroir (mode=1) par defaut.
 */
ImageGris *convolution_2d(const ImageGris *img,
                           const double *noyau, int taille)
{
    int W      = img->largeur;
    int H      = img->hauteur;
    int demi   = taille / 2;
    int x, y, u, v;
    double somme_noyau = 0.0;
    ImageGris *res;

    res = allouer_image_gris(W, H);
    if (!res) return NULL;

    /* Calculer la somme des coefficients du noyau */
    for (u = 0; u < taille * taille; u++)
        somme_noyau += noyau[u];

    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            double acc = 0.0;
            for (v = 0; v < taille; v++) {
                for (u = 0; u < taille; u++) {
                    /* Convention cours : I(i-u, j-v) * filtre(u,v) */
                    int xi = x - (u - demi);
                    int yi = y - (v - demi);
                    int pix = pixel_avec_bord(img, xi, yi, 1); /* miroir */
                    acc += noyau[v * taille + u] * pix;
                }
            }
            /* Normalisation */
            if (fabs(somme_noyau) > 1e-9)
                acc /= somme_noyau;

            PIXEL(res, x, y) = (unsigned char)CLAMP((int)(acc + 0.5), 0, 255);
        }
    }
    return res;
}

/* ============================================================
   FILTRE MOYENNEUR  (passe-bas)
   ============================================================ */

/*
 * filtre_moyenneur
 * Tous les coefficients sont egaux a 1.
 * Remplace chaque pixel par la moyenne de ses voisins.
 * La normalisation (division par taille^2) est faite dans
 * convolution_2d via somme_noyau.
 */
ImageGris *filtre_moyenneur(const ImageGris *img, int taille)
{
    int i, n = taille * taille;
    double *noyau;
    ImageGris *res;

    if (taille < 1 || taille % 2 == 0) {
        fprintf(stderr, "filtre_moyenneur : taille doit etre impaire >= 1\n");
        return NULL;
    }

    noyau = (double *)malloc(n * sizeof(double));
    if (!noyau) { perror("malloc noyau moyenneur"); return NULL; }
    for (i = 0; i < n; i++) noyau[i] = 1.0;

    res = convolution_2d(img, noyau, taille);
    free(noyau);
    return res;
}

/* ============================================================
   FILTRE GAUSSIEN  (passe-bas)
   ============================================================ */

/*
 * filtre_gaussien
 * Noyau gaussien 2D : G(x,y) = exp( -(x^2+y^2) / (2*sigma^2) )
 * Approximation numerique de la gaussienne continue.
 * La normalisation est effectuee dans convolution_2d.
 *
 * Exemple du cours pour taille=5, sigma approx:
 *  1  2  3  2  1
 *  2  6  8  6  2
 *  3  8 10  8  3
 *  2  6  8  6  2
 *  1  2  3  2  1    (/ 98)
 */
ImageGris *filtre_gaussien(const ImageGris *img, int taille, double sigma)
{
    int i, u, v;
    int demi = taille / 2;
    double *noyau;
    ImageGris *res;

    if (taille < 1 || taille % 2 == 0) {
        fprintf(stderr, "filtre_gaussien : taille doit etre impaire >= 1\n");
        return NULL;
    }
    if (sigma <= 0.0) sigma = 1.0;

    noyau = (double *)malloc(taille * taille * sizeof(double));
    if (!noyau) { perror("malloc noyau gaussien"); return NULL; }

    i = 0;
    for (v = -demi; v <= demi; v++) {
        for (u = -demi; u <= demi; u++) {
            noyau[i++] = exp(-(u*u + v*v) / (2.0 * sigma * sigma));
        }
    }
    /* convolution_2d normalise automatiquement par somme_noyau */
    res = convolution_2d(img, noyau, taille);
    free(noyau);
    return res;
}

/* ============================================================
   FILTRE MEDIAN  (non-lineaire)
   ============================================================ */

/*
 * filtre_median
 * Remplace chaque pixel par la valeur MEDIANE de son voisinage NxN.
 * Ce filtre ne peut pas s'implementer comme une convolution.
 * Tres efficace contre le bruit "poivre et sel".
 */
ImageGris *filtre_median(const ImageGris *img, int taille)
{
    int W      = img->largeur;
    int H      = img->hauteur;
    int demi   = taille / 2;
    int n_vois = taille * taille;
    int x, y, u, v, k;
    int *voisins;
    ImageGris *res;

    if (taille < 1 || taille % 2 == 0) {
        fprintf(stderr, "filtre_median : taille doit etre impaire >= 1\n");
        return NULL;
    }

    voisins = (int *)malloc(n_vois * sizeof(int));
    if (!voisins) { perror("malloc voisins median"); return NULL; }

    res = allouer_image_gris(W, H);
    if (!res) { free(voisins); return NULL; }

    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            k = 0;
            for (v = -demi; v <= demi; v++) {
                for (u = -demi; u <= demi; u++) {
                    voisins[k++] = pixel_avec_bord(img, x+u, y+v, 1);
                }
            }
            /* Tri pour trouver la mediane */
            qsort(voisins, n_vois, sizeof(int), cmp_int);
            PIXEL(res, x, y) = (unsigned char)voisins[n_vois / 2];
        }
    }
    free(voisins);
    return res;
}

/* ============================================================
   FILTRE MIN  (erosion morphologique)
   ============================================================ */

/*
 * filtre_min
 * Remplace chaque pixel par la valeur MINIMALE de son voisinage.
 * Utile pour l'erosion et pour illustrer l'effet sur le bruit.
 */
ImageGris *filtre_min(const ImageGris *img, int taille)
{
    int W    = img->largeur;
    int H    = img->hauteur;
    int demi = taille / 2;
    int x, y, u, v;
    ImageGris *res;

    if (taille < 1) return NULL;

    res = allouer_image_gris(W, H);
    if (!res) return NULL;

    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int vmin = 255;
            for (v = -demi; v <= demi; v++) {
                for (u = -demi; u <= demi; u++) {
                    int p = pixel_avec_bord(img, x+u, y+v, 1);
                    if (p < vmin) vmin = p;
                }
            }
            PIXEL(res, x, y) = (unsigned char)vmin;
        }
    }
    return res;
}

/* ============================================================
   FILTRE MAX  (dilatation morphologique)
   ============================================================ */

/*
 * filtre_max
 * Remplace chaque pixel par la valeur MAXIMALE de son voisinage.
 */
ImageGris *filtre_max(const ImageGris *img, int taille)
{
    int W    = img->largeur;
    int H    = img->hauteur;
    int demi = taille / 2;
    int x, y, u, v;
    ImageGris *res;

    if (taille < 1) return NULL;

    res = allouer_image_gris(W, H);
    if (!res) return NULL;

    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int vmax = 0;
            for (v = -demi; v <= demi; v++) {
                for (u = -demi; u <= demi; u++) {
                    int p = pixel_avec_bord(img, x+u, y+v, 1);
                    if (p > vmax) vmax = p;
                }
            }
            PIXEL(res, x, y) = (unsigned char)vmax;
        }
    }
    return res;
}
