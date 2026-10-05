/* ============================================================
   fourier.c  -  Cours 04 : Transformee de Fourier

   Contenu :
     - FFT 1D complexe (radix-2 Cooley-Tukey, in-place)
     - FFT 2D ligne par ligne puis colonne par colonne
     - FFT inverse 2D
     - Spectre de Fourier  |F(u,v)|
     - Spectre rehausse    log(1 + |F(u,v)|), normalise [0,255]
     - Inversion des quadrants (fftshift / centre au milieu)
     - Filtrage passe-bas  (masque circulaire dans le domaine freq.)
     - Filtrage passe-haut (masque circulaire)
     - Filtrage coupe-bande (notch) : annule des frequences cibles
     - Rehaussement de contraste : image + alpha * passe-haut
   ============================================================ */

#include "image.h"

/* Assurer la disponibilite de M_PI */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============================================================
   TYPES COMPLEXES INTERNES
   ============================================================ */

typedef struct { double re; double im; } Complexe;

static Complexe cadd(Complexe a, Complexe b)
{ Complexe r = {a.re+b.re, a.im+b.im}; return r; }

static Complexe csub(Complexe a, Complexe b)
{ Complexe r = {a.re-b.re, a.im-b.im}; return r; }

static Complexe cmul(Complexe a, Complexe b)
{
    Complexe r = {a.re*b.re - a.im*b.im,
                  a.re*b.im + a.im*b.re};
    return r;
}

static double cabs_val(Complexe a)
{ return sqrt(a.re*a.re + a.im*a.im); }

/* ============================================================
   UTILITAIRES FFT
   ============================================================ */

/*
 * prochaine_puissance_de_2
 * Retourne la puissance de 2 >= n.
 */
static int prochaine_puissance_2(int n)
{
    int p = 1;
    while (p < n) p <<= 1;
    return p;
}

/*
 * bit_reversal_permutation
 * Reordonne le tableau data (taille n, puissance de 2)
 * selon le renversement des bits de l'indice.
 */
static void bit_reversal(Complexe *data, int n)
{
    int i, j = 0, k;
    for (i = 1; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { Complexe tmp = data[i]; data[i] = data[j]; data[j] = tmp; }
    }
    (void)k;
}

/*
 * fft1d_inplace
 * FFT 1D de Cooley-Tukey, radix-2, in-place.
 * direction : +1 = directe, -1 = inverse.
 * n DOIT etre une puissance de 2.
 */
static void fft1d_inplace(Complexe *data, int n, int direction)
{
    int len, i;
    bit_reversal(data, n);

    for (len = 2; len <= n; len <<= 1) {
        double angle = direction * 2.0 * M_PI / len;
        Complexe wlen = { cos(angle), sin(angle) };
        for (i = 0; i < n; i += len) {
            Complexe w = {1.0, 0.0};
            int j;
            for (j = 0; j < len/2; j++) {
                Complexe u = data[i+j];
                Complexe t = cmul(w, data[i+j+len/2]);
                data[i+j]        = cadd(u, t);
                data[i+j+len/2]  = csub(u, t);
                w = cmul(w, wlen);
            }
        }
    }

    /* Normalisation pour la FFT inverse */
    if (direction == -1) {
        for (i = 0; i < n; i++) {
            data[i].re /= n;
            data[i].im /= n;
        }
    }
}

/* ============================================================
   ALLOCATION DU TABLEAU COMPLEXE 2D
   ============================================================ */

/*
 * Alloue un tableau 2D de Complexes, taille (H x W).
 * Retourne NULL en cas d'erreur.
 */
static Complexe *allouer_tableau_complexe(int W, int H)
{
    return (Complexe *)calloc(W * H, sizeof(Complexe));
}

/* ============================================================
   FFT 2D
   Convention : dimensions completees a la prochaine puissance
   de 2 par zero-padding.
   ============================================================ */

/*
 * image_vers_complexe
 * Copie les pixels dans un tableau complexe de taille (fH x fW),
 * le reste est laisse a zero (zero-padding).
 */
static Complexe *image_vers_complexe(const ImageGris *img,
                                      int fW, int fH)
{
    Complexe *data = allouer_tableau_complexe(fW, fH);
    int x, y;
    if (!data) return NULL;
    for (y = 0; y < img->hauteur; y++) {
        for (x = 0; x < img->largeur; x++) {
            data[y*fW + x].re = PIXEL(img, x, y);
            data[y*fW + x].im = 0.0;
        }
    }
    return data;
}

/*
 * fft2d
 * FFT 2D directe (direction=+1) ou inverse (direction=-1).
 * Opere en place sur data (fH x fW), dimensions en puissances de 2.
 */
static void fft2d_inplace(Complexe *data, int fW, int fH, int direction)
{
    int x, y;
    Complexe *ligne;

    /* Alloue un buffer temporaire pour les lignes/colonnes */
    int taille_max = (fW > fH) ? fW : fH;
    Complexe *buf = (Complexe *)malloc(taille_max * sizeof(Complexe));
    if (!buf) return;

    /* --- FFT sur chaque ligne --- */
    for (y = 0; y < fH; y++) {
        ligne = data + y * fW;
        fft1d_inplace(ligne, fW, direction);
    }

    /* --- FFT sur chaque colonne --- */
    for (x = 0; x < fW; x++) {
        int i;
        for (i = 0; i < fH; i++) buf[i] = data[i*fW + x];
        fft1d_inplace(buf, fH, direction);
        for (i = 0; i < fH; i++) data[i*fW + x] = buf[i];
    }

    free(buf);
}

/* ============================================================
   FFTSHIFT : inversion des quadrants
   Centre les basses frequences au milieu de l'image.
   ============================================================ */

static void fftshift(Complexe *data, int fW, int fH)
{
    int x, y;
    int hw = fW / 2, hh = fH / 2;
    /* Echange bloc haut-gauche <-> bas-droit et haut-droit <-> bas-gauche */
    for (y = 0; y < hh; y++) {
        for (x = 0; x < hw; x++) {
            Complexe tmp;
            /* haut-gauche <-> bas-droit */
            tmp = data[y*fW + x];
            data[y*fW + x] = data[(y+hh)*fW + (x+hw)];
            data[(y+hh)*fW + (x+hw)] = tmp;
            /* haut-droit <-> bas-gauche */
            tmp = data[y*fW + (x+hw)];
            data[y*fW + (x+hw)] = data[(y+hh)*fW + x];
            data[(y+hh)*fW + x] = tmp;
        }
    }
}

/* ============================================================
   INTERFACE PUBLIQUE : calcule FFT et renvoie les tableaux
   ============================================================ */

/*
 * calculer_fft2d
 * Calcule la FFT 2D d'une image et retourne le tableau complexe.
 * *fW_out, *fH_out : dimensions utilisees (puissances de 2).
 * L'appelant doit liberer le tableau retourne avec free().
 * Si centered=1, applique fftshift (centre a 0).
 */
Complexe *calculer_fft2d(const ImageGris *img,
                          int *fW_out, int *fH_out, int centered)
{
    int fW = prochaine_puissance_2(img->largeur);
    int fH = prochaine_puissance_2(img->hauteur);
    Complexe *data = image_vers_complexe(img, fW, fH);
    if (!data) return NULL;

    fft2d_inplace(data, fW, fH, +1);
    if (centered) fftshift(data, fW, fH);

    *fW_out = fW;
    *fH_out = fH;
    return data;
}

/* ============================================================
   SPECTRE DE FOURIER  |F(u,v)|
   Normalise dans [0,255] par rapport au maximum.
   ============================================================ */

ImageGris *spectre_fourier(const ImageGris *img)
{
    int fW, fH, x, y;
    double max_val = 0.0;
    Complexe *data = calculer_fft2d(img, &fW, &fH, 1); /* centre */
    ImageGris *res;

    if (!data) return NULL;

    /* Calcul du maximum */
    for (y = 0; y < fH; y++)
        for (x = 0; x < fW; x++) {
            double m = cabs_val(data[y*fW + x]);
            if (m > max_val) max_val = m;
        }

    /* Image en sortie aux dimensions de l'originale (crop) */
    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) { free(data); return NULL; }

    if (max_val > 0) {
        for (y = 0; y < img->hauteur; y++)
            for (x = 0; x < img->largeur; x++) {
                double m = cabs_val(data[y*fW + x]);
                int v = (int)(m / max_val * 255.0 + 0.5);
                PIXEL(res, x, y) = (unsigned char)CLAMP(v, 0, 255);
            }
    }
    free(data);
    return res;
}

/* ============================================================
   SPECTRE REHAUSSE  log(1 + |F(u,v)|)
   Permet de visualiser les dynamiques faibles.
   ============================================================ */

ImageGris *spectre_rehausse(const ImageGris *img)
{
    int fW, fH, x, y;
    double max_val = 0.0;
    Complexe *data = calculer_fft2d(img, &fW, &fH, 1);
    ImageGris *res;

    if (!data) return NULL;

    for (y = 0; y < fH; y++)
        for (x = 0; x < fW; x++) {
            double m = log(1.0 + cabs_val(data[y*fW + x]));
            if (m > max_val) max_val = m;
        }

    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) { free(data); return NULL; }

    if (max_val > 0) {
        for (y = 0; y < img->hauteur; y++)
            for (x = 0; x < img->largeur; x++) {
                double m = log(1.0 + cabs_val(data[y*fW + x]));
                int v = (int)(m / max_val * 255.0 + 0.5);
                PIXEL(res, x, y) = (unsigned char)CLAMP(v, 0, 255);
            }
    }
    free(data);
    return res;
}

/* ============================================================
   FILTRAGE PASSE-BAS dans le domaine frequentiel
   Masque circulaire de rayon r (en % de la demi-diagonale).
   On conserve les frequences dans le cercle (basses freq.).
   ============================================================ */

ImageGris *filtre_passe_bas_fft(const ImageGris *img, double rayon_pct)
{
    int fW, fH, x, y;
    double cx, cy, rayon, dist;
    Complexe *data = calculer_fft2d(img, &fW, &fH, 1);
    ImageGris *res;

    if (!data) return NULL;

    cx = fW / 2.0;
    cy = fH / 2.0;
    rayon = rayon_pct / 100.0 * sqrt(cx*cx + cy*cy);

    /* Application du masque circulaire */
    for (y = 0; y < fH; y++) {
        for (x = 0; x < fW; x++) {
            double dx = x - cx, dy = y - cy;
            dist = sqrt(dx*dx + dy*dy);
            if (dist > rayon) {
                data[y*fW + x].re = 0.0;
                data[y*fW + x].im = 0.0;
            }
        }
    }

    /* FFT inverse : recentre d'abord, puis IFFT */
    fftshift(data, fW, fH);   /* defait le shift avant IFFT */
    fft2d_inplace(data, fW, fH, -1);

    /* Extraction de la partie reelle, crop aux dimensions originales */
    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) { free(data); return NULL; }

    for (y = 0; y < img->hauteur; y++)
        for (x = 0; x < img->largeur; x++) {
            int v = (int)(data[y*fW + x].re + 0.5);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v, 0, 255);
        }

    free(data);
    return res;
}

/* ============================================================
   FILTRAGE PASSE-HAUT dans le domaine frequentiel
   On annule les frequences a l'interieur du cercle (basses freq.).
   ============================================================ */

ImageGris *filtre_passe_haut_fft(const ImageGris *img, double rayon_pct)
{
    int fW, fH, x, y;
    double cx, cy, rayon, dist;
    Complexe *data = calculer_fft2d(img, &fW, &fH, 1);
    ImageGris *res;

    if (!data) return NULL;

    cx = fW / 2.0;
    cy = fH / 2.0;
    rayon = rayon_pct / 100.0 * sqrt(cx*cx + cy*cy);

    for (y = 0; y < fH; y++) {
        for (x = 0; x < fW; x++) {
            double dx = x - cx, dy = y - cy;
            dist = sqrt(dx*dx + dy*dy);
            if (dist < rayon) {
                data[y*fW + x].re = 0.0;
                data[y*fW + x].im = 0.0;
            }
        }
    }

    fftshift(data, fW, fH);
    fft2d_inplace(data, fW, fH, -1);

    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) { free(data); return NULL; }

    for (y = 0; y < img->hauteur; y++)
        for (x = 0; x < img->largeur; x++) {
            int v = (int)(data[y*fW + x].re + 0.5);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v, 0, 255);
        }

    free(data);
    return res;
}

/* ============================================================
   SPECTRE DU FILTRE PASSE-BAS (visualisation du masque)
   Retourne l'image du spectre APRES application du masque.
   ============================================================ */

ImageGris *spectre_passe_bas(const ImageGris *img, double rayon_pct)
{
    int fW, fH, x, y;
    double cx, cy, rayon, dist, max_val = 0.0;
    Complexe *data = calculer_fft2d(img, &fW, &fH, 1);
    ImageGris *res;

    if (!data) return NULL;

    cx = fW / 2.0;
    cy = fH / 2.0;
    rayon = rayon_pct / 100.0 * sqrt(cx*cx + cy*cy);

    for (y = 0; y < fH; y++) {
        for (x = 0; x < fW; x++) {
            double dx = x - cx, dy = y - cy;
            dist = sqrt(dx*dx + dy*dy);
            if (dist > rayon) {
                data[y*fW + x].re = 0.0;
                data[y*fW + x].im = 0.0;
            }
        }
    }

    for (y = 0; y < fH; y++)
        for (x = 0; x < fW; x++) {
            double m = log(1.0 + cabs_val(data[y*fW + x]));
            if (m > max_val) max_val = m;
        }

    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) { free(data); return NULL; }

    if (max_val > 0) {
        for (y = 0; y < img->hauteur; y++)
            for (x = 0; x < img->largeur; x++) {
                double m = log(1.0 + cabs_val(data[y*fW + x]));
                int v = (int)(m / max_val * 255.0 + 0.5);
                PIXEL(res, x, y) = (unsigned char)CLAMP(v, 0, 255);
            }
    }
    free(data);
    return res;
}

/* ============================================================
   SPECTRE DU FILTRE PASSE-HAUT (visualisation du masque)
   ============================================================ */

ImageGris *spectre_passe_haut(const ImageGris *img, double rayon_pct)
{
    int fW, fH, x, y;
    double cx, cy, rayon, dist, max_val = 0.0;
    Complexe *data = calculer_fft2d(img, &fW, &fH, 1);
    ImageGris *res;

    if (!data) return NULL;

    cx = fW / 2.0;
    cy = fH / 2.0;
    rayon = rayon_pct / 100.0 * sqrt(cx*cx + cy*cy);

    for (y = 0; y < fH; y++) {
        for (x = 0; x < fW; x++) {
            double dx = x - cx, dy = y - cy;
            dist = sqrt(dx*dx + dy*dy);
            if (dist < rayon) {
                data[y*fW + x].re = 0.0;
                data[y*fW + x].im = 0.0;
            }
        }
    }

    for (y = 0; y < fH; y++)
        for (x = 0; x < fW; x++) {
            double m = log(1.0 + cabs_val(data[y*fW + x]));
            if (m > max_val) max_val = m;
        }

    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) { free(data); return NULL; }

    if (max_val > 0) {
        for (y = 0; y < img->hauteur; y++)
            for (x = 0; x < img->largeur; x++) {
                double m = log(1.0 + cabs_val(data[y*fW + x]));
                int v = (int)(m / max_val * 255.0 + 0.5);
                PIXEL(res, x, y) = (unsigned char)CLAMP(v, 0, 255);
            }
    }
    free(data);
    return res;
}

/* ============================================================
   FILTRAGE COUPE-BANDE (NOTCH)
   Annule des frequences autour d'un point (u0, v0) du spectre
   centre, dans un rayon donne.
   Ce qui est utile pour supprimer un bruit periodique visible dans le spectre.
   ============================================================ */

ImageGris *filtre_notch(const ImageGris *img,
                         int u0, int v0, double rayon_notch)
{
    int fW, fH, x, y;
    double cx, cy;
    Complexe *data = calculer_fft2d(img, &fW, &fH, 1);
    ImageGris *res;

    if (!data) return NULL;

    cx = fW / 2.0;
    cy = fH / 2.0;

    /* Annule deux points symetriques (spectre hermitien) */
    for (y = 0; y < fH; y++) {
        for (x = 0; x < fW; x++) {
            double dx1 = x - (cx + u0), dy1 = y - (cy + v0);
            double dx2 = x - (cx - u0), dy2 = y - (cy - v0);
            if (sqrt(dx1*dx1+dy1*dy1) < rayon_notch ||
                sqrt(dx2*dx2+dy2*dy2) < rayon_notch) {
                data[y*fW + x].re = 0.0;
                data[y*fW + x].im = 0.0;
            }
        }
    }

    fftshift(data, fW, fH);
    fft2d_inplace(data, fW, fH, -1);

    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) { free(data); return NULL; }

    for (y = 0; y < img->hauteur; y++)
        for (x = 0; x < img->largeur; x++) {
            int v = (int)(data[y*fW + x].re + 0.5);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v, 0, 255);
        }

    free(data);
    return res;
}

/* ============================================================
   REHAUSSEMENT DE CONTRASTE PAR PASSE-HAUT
   Image rehaussee = image originale + alpha * passe_haut
   (slide cours 29 : image + image_filtree_passe_haut)
   ============================================================ */

ImageGris *rehaussement_fft(const ImageGris *img,
                              double rayon_pct, double alpha)
{
    int fW, fH, x, y;
    double cx, cy, rayon, dist;
    /* On calcule le passe-haut en FFT */
    Complexe *data = calculer_fft2d(img, &fW, &fH, 1);
    ImageGris *res;
    double *ph_buf;   /* buffer passe-haut */

    if (!data) return NULL;

    cx = fW / 2.0;
    cy = fH / 2.0;
    rayon = rayon_pct / 100.0 * sqrt(cx*cx + cy*cy);

    for (y = 0; y < fH; y++) {
        for (x = 0; x < fW; x++) {
            double dx = x - cx, dy = y - cy;
            dist = sqrt(dx*dx + dy*dy);
            if (dist < rayon) {
                data[y*fW + x].re = 0.0;
                data[y*fW + x].im = 0.0;
            }
        }
    }

    fftshift(data, fW, fH);
    fft2d_inplace(data, fW, fH, -1);

    ph_buf = (double *)malloc(img->largeur * img->hauteur * sizeof(double));
    if (!ph_buf) { free(data); return NULL; }

    for (y = 0; y < img->hauteur; y++)
        for (x = 0; x < img->largeur; x++)
            ph_buf[y*img->largeur+x] = data[y*fW+x].re;

    free(data);

    /* Combinaison : originale + alpha * passe_haut */
    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) { free(ph_buf); return NULL; }

    for (y = 0; y < img->hauteur; y++)
        for (x = 0; x < img->largeur; x++) {
            double v = PIXEL(img, x, y) + alpha * ph_buf[y*img->largeur+x];
            PIXEL(res, x, y) = (unsigned char)CLAMP((int)(v+0.5), 0, 255);
        }

    free(ph_buf);
    return res;
}

/* ============================================================
   VERIFICATION : IFFT o FFT = identite (test interne)
   Retourne 1 si l'erreur quadratique moyenne < seuil, 0 sinon.
   ============================================================ */
int verifier_fft_inverse(const ImageGris *img)
{
    int fW, fH, x, y;
    double erreur = 0.0;
    Complexe *data = calculer_fft2d(img, &fW, &fH, 0);
    if (!data) return 0;

    /* IFFT */
    fft2d_inplace(data, fW, fH, -1);

    for (y = 0; y < img->hauteur; y++) {
        for (x = 0; x < img->largeur; x++) {
            double d = data[y*fW+x].re - PIXEL(img, x, y);
            erreur += d*d;
        }
    }
    erreur /= (img->largeur * img->hauteur);
    free(data);
    return (erreur < 1.0); /* EQM < 1 niveau de gris */
}
