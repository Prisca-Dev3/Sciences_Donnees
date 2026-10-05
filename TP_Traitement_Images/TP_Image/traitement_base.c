/* ============================================================
   traitement_base.c  -  Cours 02 : Traitement de base
   Contenu :
     - Histogramme
     - Luminance / Contraste
     - Transformations de contraste (lineaire, saturation,
       morceaux, gamma, inversion, seuillage)
     - Egalisation d'histogramme (globale + locale)
     - Operations arithmetiques (+, -, * scalaire, fusion)
     - Operations logiques (ET, OU, XOR, NON, NON-ET)
     - Interpolation (plus proche voisin, bilineaire, bicubique)
     - Profil d'intensite
   ============================================================ */

#include "image.h"

/* ============================================================
   HISTOGRAMME
   ============================================================ */

/*
 * calculer_histogramme
 * H(k) = nombre de pixels ayant la valeur k  (k in [0,255])
 */
void calculer_histogramme(const ImageGris *img, int hist[256])
{
    int i, n;
    memset(hist, 0, 256 * sizeof(int));
    n = img->largeur * img->hauteur;
    for (i = 0; i < n; i++)
        hist[img->pixels[i]]++;
}

/*
 * Normalise sur 50 colonnes pour la lisibilite.
 */
void afficher_histogramme(const int hist[256])
{
    int i, j, max_h = 0;
    for (i = 0; i < 256; i++)
        if (hist[i] > max_h) max_h = hist[i];
    if (max_h == 0) return;

    printf("Histogramme (valeur | barre) :\n");
    for (i = 0; i < 256; i++) {
        int nb = (int)((double)hist[i] / max_h * 50 + 0.5);
        printf("%3d |", i);
        for (j = 0; j < nb; j++) putchar('*');
        printf(" (%d)\n", hist[i]);
    }
}

/* ============================================================
   LUMINANCE ET CONTRASTE
   ============================================================ */

/*
 * calculer_luminance
 * Luminance = moyenne de tous les pixels de l'image.
 */
double calculer_luminance(const ImageGris *img)
{
    long long somme = 0;
    int i, n = img->largeur * img->hauteur;
    for (i = 0; i < n; i++)
        somme += img->pixels[i];
    return (double)somme / n;
}

/*
 * calculer_contraste_ecarttype
 * C = sqrt( (1/MN) * sum( (f(x,y) - moy)^2 ) )
 */
double calculer_contraste_ecarttype(const ImageGris *img)
{
    double moy = calculer_luminance(img);
    double somme = 0.0;
    int i, n = img->largeur * img->hauteur;
    for (i = 0; i < n; i++) {
        double d = img->pixels[i] - moy;
        somme += d * d;
    }
    return sqrt(somme / n);
}

/*
 * calculer_contraste_minmax
 * C = (max - min) / (max + min)
 */
double calculer_contraste_minmax(const ImageGris *img)
{
    int vmin = 255, vmax = 0;
    int i, n = img->largeur * img->hauteur;
    for (i = 0; i < n; i++) {
        if (img->pixels[i] < vmin) vmin = img->pixels[i];
        if (img->pixels[i] > vmax) vmax = img->pixels[i];
    }
    if (vmax + vmin == 0) return 0.0;
    return (double)(vmax - vmin) / (vmax + vmin);
}

/* ============================================================
   TRANSFORMATIONS DE CONTRASTE
   ============================================================ */

/*
 * transformation_lineaire
 * Etire l'histogramme pour occuper [0, 255].
 * I'(i,j) = 255 * (I(i,j) - min) / (max - min)
 * Utilise une LUT pour l'efficacite.
 */
ImageGris *transformation_lineaire(const ImageGris *img)
{
    int vmin = 255, vmax = 0;
    int i, n = img->largeur * img->hauteur;
    unsigned char lut[256];
    ImageGris *res;

    /* Trouver min et max */
    for (i = 0; i < n; i++) {
        if (img->pixels[i] < vmin) vmin = img->pixels[i];
        if (img->pixels[i] > vmax) vmax = img->pixels[i];
    }

    /* Construire LUT */
    for (i = 0; i < 256; i++) {
        if (vmax == vmin)
            lut[i] = 0;
        else
            lut[i] = (unsigned char)CLAMP(
                (int)(255.0 * (i - vmin) / (vmax - vmin) + 0.5), 0, 255);
    }

    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = lut[img->pixels[i]];
    return res;
}

/*
 * transformation_lineaire_saturation
 * Saturation aux seuils smin et smax :
 *   I'(i,j) = 0           si I < smin
 *   I'(i,j) = 255         si I >= smax
 *   I'(i,j) = 255*(I-smin)/(smax-smin)  sinon
 */
ImageGris *transformation_lineaire_saturation(const ImageGris *img,
                                               int smin, int smax)
{
    int i, n = img->largeur * img->hauteur;
    unsigned char lut[256];
    ImageGris *res;

    for (i = 0; i < 256; i++) {
        if (i <= smin)
            lut[i] = 0;
        else if (i >= smax)
            lut[i] = 255;
        else
            lut[i] = (unsigned char)CLAMP(
                (int)(255.0 * (i - smin) / (smax - smin) + 0.5), 0, 255);
    }

    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = lut[img->pixels[i]];
    return res;
}

/*
 * transformation_lineaire_morceaux
 * Trois segments :
 *   [0, smin]         -> pente douce (compression des sombres)
 *   [smin, smax]      -> pente forte (etirement de la zone utile)
 *   [smax, 255]       -> sature a 255
 * Le parametre s est le seuil de valeur de sortie a smin (0 <= s <= 255).
 */
ImageGris *transformation_lineaire_morceaux(const ImageGris *img,
                                             int smin, int smax, int s)
{
    int i, n = img->largeur * img->hauteur;
    unsigned char lut[256];
    ImageGris *res;

    for (i = 0; i < 256; i++) {
        double val;
        if (i <= smin) {
            /* Segment 1 : [0,smin] -> [0, s] */
            val = (smin > 0) ? (double)s * i / smin : 0.0;
        } else if (i <= smax) {
            /* Segment 2 : [smin, smax] -> [s, 255] */
            val = (smax > smin)
                ? s + (double)(255 - s) * (i - smin) / (smax - smin)
                : 255.0;
        } else {
            /* Segment 3 : [smax, 255] -> 255 */
            val = 255.0;
        }
        lut[i] = (unsigned char)CLAMP((int)(val + 0.5), 0, 255);
    }

    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = lut[img->pixels[i]];
    return res;
}

/*
 * correction_gamma
 * n' = n^(1/gamma)   (normalise sur [0,1] puis remultiplie par 255)
 * gamma > 1 : assombrit ; gamma < 1 : eclaircit
 */
ImageGris *correction_gamma(const ImageGris *img, double gamma)
{
    int i, n = img->largeur * img->hauteur;
    double inv_gamma;
    unsigned char lut[256];
    ImageGris *res;

    if (gamma <= 0.0) gamma = 1.0;
    inv_gamma = 1.0 / gamma;

    for (i = 0; i < 256; i++) {
        double norm = i / 255.0;
        lut[i] = (unsigned char)CLAMP(
            (int)(255.0 * pow(norm, inv_gamma) + 0.5), 0, 255);
    }

    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = lut[img->pixels[i]];
    return res;
}

/*
 * inversion
 * I'(x,y) = 255 - I(x,y)
 */
ImageGris *inversion(const ImageGris *img)
{
    int i, n = img->largeur * img->hauteur;
    ImageGris *res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = (unsigned char)(255 - img->pixels[i]);
    return res;
}

/*
 * seuillage
 * I'(x,y) = 255 si I(x,y) >= seuil, 0 sinon  (image binaire)
 */
ImageGris *seuillage(const ImageGris *img, int seuil)
{
    int i, n = img->largeur * img->hauteur;
    ImageGris *res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = (img->pixels[i] >= seuil) ? 255 : 0;
    return res;
}

/* ============================================================
   EGALISATION D'HISTOGRAMME
   ============================================================ */

/*
 * egalisation_histogramme  (globale)
 * Etape 1 : h(i) = histogramme
 * Etape 2 : hn(i) = h(i) / Nbp    (normalisation)
 * Etape 3 : Ci = sum_{j=0}^{i} hn(j)  (cumul)
 * Etape 4 : f'(x,y) = Cf(x,y) * 255
 */
ImageGris *egalisation_histogramme(const ImageGris *img)
{
    int hist[256];
    double hn[256];
    double cumul[256];
    unsigned char lut[256];
    int i, n = img->largeur * img->hauteur;
    ImageGris *res;

    calculer_histogramme(img, hist);

    /* Normalisation */
    for (i = 0; i < 256; i++)
        hn[i] = (double)hist[i] / n;

    /* Cumul */
    cumul[0] = hn[0];
    for (i = 1; i < 256; i++)
        cumul[i] = cumul[i-1] + hn[i];

    /* LUT */
    for (i = 0; i < 256; i++)
        lut[i] = (unsigned char)CLAMP((int)(cumul[i] * 255.0 + 0.5), 0, 255);

    res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = lut[img->pixels[i]];
    return res;
}

/*
 * egalisation_locale
 * Pour chaque pixel, calcule l'histogramme d'une fenetre
 * taille_fenetre x taille_fenetre centree sur ce pixel,
 * puis applique l'egalisation localement.
 */
ImageGris *egalisation_locale(const ImageGris *img, int taille_fenetre)
{
    int x, y, u, v;
    int demi = taille_fenetre / 2;
    int W    = img->largeur;
    int H    = img->hauteur;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;

    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int hist_loc[256] = {0};
            double hn[256];
            double cumul[256];
            int count = 0;
            int val_pixel = PIXEL(img, x, y);

            /* Accumuler l'histogramme local */
            for (v = y - demi; v <= y + demi; v++) {
                for (u = x - demi; u <= x + demi; u++) {
                    int xu = CLAMP(u, 0, W-1);
                    int yv = CLAMP(v, 0, H-1);
                    hist_loc[PIXEL(img, xu, yv)]++;
                    count++;
                }
            }

            /* Normalisation et cumul */
            hn[0] = (double)hist_loc[0] / count;
            cumul[0] = hn[0];
            for (u = 1; u < 256; u++) {
                hn[u] = (double)hist_loc[u] / count;
                cumul[u] = cumul[u-1] + hn[u];
            }

            PIXEL(res, x, y) = (unsigned char)
                CLAMP((int)(cumul[val_pixel] * 255.0 + 0.5), 0, 255);
        }
    }
    return res;
}

/* ============================================================
   OPERATIONS ARITHMETIQUES SUR LES IMAGES
   ============================================================ */

/*
 * addition_images
 * R(x,y) = min( f(x,y) + g(x,y) , 255 )
 */
ImageGris *addition_images(const ImageGris *a, const ImageGris *b)
{
    int i, n;
    ImageGris *b_compat = NULL;
    const ImageGris *b_use = b;
    ImageGris *res;
    if (!meme_dimensions(a, b)) {
        fprintf(stderr,
            "addition_images : redimensionnement %dx%d -> %dx%d\n",
            b->largeur, b->hauteur, a->largeur, a->hauteur);
        b_compat = redimensionner_compatible(b, a->largeur, a->hauteur);
        if (!b_compat) return NULL;
        b_use = b_compat;
    }
    n   = a->largeur * a->hauteur;
    res = allouer_image_gris(a->largeur, a->hauteur);
    if (!res) { liberer_image_gris(b_compat); return NULL; }
    for (i = 0; i < n; i++) {
        int s = a->pixels[i] + b_use->pixels[i];
        res->pixels[i] = (unsigned char)MIN2(s, 255);
    }
    liberer_image_gris(b_compat);
    return res;
}

/*
 * soustraction_images
 * S(x,y) = max( f(x,y) - g(x,y) , 0 )
 */
ImageGris *soustraction_images(const ImageGris *a, const ImageGris *b)
{
    int i, n;
    ImageGris *b_compat = NULL;
    const ImageGris *b_use = b;
    ImageGris *res;
    if (!meme_dimensions(a, b)) {
        fprintf(stderr,
            "soustraction_images : redimensionnement %dx%d -> %dx%d\n",
            b->largeur, b->hauteur, a->largeur, a->hauteur);
        b_compat = redimensionner_compatible(b, a->largeur, a->hauteur);
        if (!b_compat) return NULL;
        b_use = b_compat;
    }
    n   = a->largeur * a->hauteur;
    res = allouer_image_gris(a->largeur, a->hauteur);
    if (!res) { liberer_image_gris(b_compat); return NULL; }
    for (i = 0; i < n; i++) {
        int s = a->pixels[i] - b_use->pixels[i];
        res->pixels[i] = (unsigned char)MAX2(s, 0);
    }
    liberer_image_gris(b_compat);
    return res;
}

/*
 * multiplication_images  (pixel a pixel entre deux images)
 * R(x,y) = clamp( f(x,y) * g(x,y) / 255 , 0, 255 )
 * Division par 255 pour rester dans [0,255].
 * Si les dimensions sont differentes, g est redimensionne.
 */
ImageGris *multiplication_images(const ImageGris *a, const ImageGris *b)
{
    int i, n;
    ImageGris *b_compat = NULL;
    const ImageGris *b_use = b;
    ImageGris *res;

    if (!meme_dimensions(a, b)) {
        fprintf(stderr,
            "multiplication_images : dimensions differentes "
            "(%dx%d vs %dx%d), redimensionnement automatique\n",
            a->largeur, a->hauteur, b->largeur, b->hauteur);
        b_compat = redimensionner_compatible(b, a->largeur, a->hauteur);
        if (!b_compat) return NULL;
        b_use = b_compat;
    }
    n   = a->largeur * a->hauteur;
    res = allouer_image_gris(a->largeur, a->hauteur);
    if (!res) { liberer_image_gris(b_compat); return NULL; }
    for (i = 0; i < n; i++) {
        int val = (a->pixels[i] * b_use->pixels[i]) / 255;
        res->pixels[i] = (unsigned char)CLAMP(val, 0, 255);
    }
    liberer_image_gris(b_compat);
    return res;
}

/*
 * multiplication_image  (scalaire)
 * S(x,y) = min( f(x,y) * ratio , 255 )
 */
ImageGris *multiplication_image(const ImageGris *img, double ratio)
{
    int i, n = img->largeur * img->hauteur;
    ImageGris *res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++) {
        int val = (int)(img->pixels[i] * ratio + 0.5);
        res->pixels[i] = (unsigned char)CLAMP(val, 0, 255);
    }
    return res;
}

/*
 * fusion_images  (0.5*F + 0.5*G generalise avec alpha)
 * R(x,y) = alpha * a(x,y) + (1-alpha) * b(x,y)
 */
ImageGris *fusion_images(const ImageGris *a, const ImageGris *b, double alpha)
{
    int i, n;
    ImageGris *res;
    if (a->largeur != b->largeur || a->hauteur != b->hauteur) {
        fprintf(stderr, "fusion_images : dimensions incompatibles\n");
        return NULL;
    }
    n   = a->largeur * a->hauteur;
    res = allouer_image_gris(a->largeur, a->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++) {
        double val = alpha * a->pixels[i] + (1.0 - alpha) * b->pixels[i];
        res->pixels[i] = (unsigned char)CLAMP((int)(val + 0.5), 0, 255);
    }
    return res;
}

/* ============================================================
   OPERATIONS LOGIQUES SUR LES IMAGES
   ============================================================ */

ImageGris *et_images(const ImageGris *a, const ImageGris *b)
{
    int i, n;
    ImageGris *res;
    if (a->largeur != b->largeur || a->hauteur != b->hauteur) {
        fprintf(stderr, "et_images : dimensions incompatibles\n"); return NULL;
    }
    n   = a->largeur * a->hauteur;
    res = allouer_image_gris(a->largeur, a->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = a->pixels[i] & b->pixels[i];
    return res;
}

ImageGris *ou_images(const ImageGris *a, const ImageGris *b)
{
    int i, n;
    ImageGris *res;
    if (a->largeur != b->largeur || a->hauteur != b->hauteur) {
        fprintf(stderr, "ou_images : dimensions incompatibles\n"); return NULL;
    }
    n   = a->largeur * a->hauteur;
    res = allouer_image_gris(a->largeur, a->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = a->pixels[i] | b->pixels[i];
    return res;
}

ImageGris *xor_images(const ImageGris *a, const ImageGris *b)
{
    int i, n;
    ImageGris *res;
    if (a->largeur != b->largeur || a->hauteur != b->hauteur) {
        fprintf(stderr, "xor_images : dimensions incompatibles\n"); return NULL;
    }
    n   = a->largeur * a->hauteur;
    res = allouer_image_gris(a->largeur, a->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = a->pixels[i] ^ b->pixels[i];
    return res;
}

ImageGris *non_image(const ImageGris *img)
{
    int i, n = img->largeur * img->hauteur;
    ImageGris *res = allouer_image_gris(img->largeur, img->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = ~img->pixels[i];
    return res;
}

/* NOT(A) AND B */
ImageGris *non_et_images(const ImageGris *a, const ImageGris *b)
{
    int i, n;
    ImageGris *res;
    if (a->largeur != b->largeur || a->hauteur != b->hauteur) {
        fprintf(stderr, "non_et_images : dimensions incompatibles\n"); return NULL;
    }
    n   = a->largeur * a->hauteur;
    res = allouer_image_gris(a->largeur, a->hauteur);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = (~a->pixels[i]) & b->pixels[i];
    return res;
}

/* ============================================================
   INTERPOLATION
   ============================================================ */

/*
 * zoom_plus_proche_voisin
 * Le pixel de l'image destination est la copie du pixel source
 * le plus proche.
 *   xs = round( xd / facteur_x )
 *   ys = round( yd / facteur_y )
 */
ImageGris *zoom_plus_proche_voisin(const ImageGris *img,
                                    double facteur_x, double facteur_y)
{
    int new_w = (int)(img->largeur  * facteur_x + 0.5);
    int new_h = (int)(img->hauteur * facteur_y + 0.5);
    int xd, yd;
    ImageGris *res = allouer_image_gris(new_w, new_h);
    if (!res) return NULL;

    for (yd = 0; yd < new_h; yd++) {
        for (xd = 0; xd < new_w; xd++) {
            int xs = (int)(xd / facteur_x + 0.5);
            int ys = (int)(yd / facteur_y + 0.5);
            xs = CLAMP(xs, 0, img->largeur  - 1);
            ys = CLAMP(ys, 0, img->hauteur - 1);
            PIXEL(res, xd, yd) = PIXEL(img, xs, ys);
        }
    }
    return res;
}

/*
 * zoom_bilineaire
 * Interpolation bilineaire a partir de 4 voisins.
 *   xs = xd / facteur_x
 *   ys = yd / facteur_y
 *   x0 = floor(xs), x1 = x0+1
 *   y0 = floor(ys), y1 = y0+1
 *   tx = xs - x0,   ty = ys - y0
 *   P = (1-tx)(1-ty)*P00 + tx(1-ty)*P10
 *     + (1-tx)ty*P01    + tx*ty*P11
 */
ImageGris *zoom_bilineaire(const ImageGris *img,
                            double facteur_x, double facteur_y)
{
    int new_w = (int)(img->largeur  * facteur_x + 0.5);
    int new_h = (int)(img->hauteur * facteur_y + 0.5);
    int xd, yd;
    ImageGris *res = allouer_image_gris(new_w, new_h);
    if (!res) return NULL;

    for (yd = 0; yd < new_h; yd++) {
        for (xd = 0; xd < new_w; xd++) {
            double xs = xd / facteur_x;
            double ys = yd / facteur_y;
            int x0 = (int)xs;
            int y0 = (int)ys;
            int x1 = x0 + 1;
            int y1 = y0 + 1;
            double tx = xs - x0;
            double ty = ys - y0;
            double p00, p10, p01, p11, val;

            x0 = CLAMP(x0, 0, img->largeur  - 1);
            x1 = CLAMP(x1, 0, img->largeur  - 1);
            y0 = CLAMP(y0, 0, img->hauteur - 1);
            y1 = CLAMP(y1, 0, img->hauteur - 1);

            p00 = PIXEL(img, x0, y0);
            p10 = PIXEL(img, x1, y0);
            p01 = PIXEL(img, x0, y1);
            p11 = PIXEL(img, x1, y1);

            val = (1-tx)*(1-ty)*p00 + tx*(1-ty)*p10
                + (1-tx)*ty    *p01 + tx*ty    *p11;

            PIXEL(res, xd, yd) = (unsigned char)CLAMP((int)(val + 0.5), 0, 255);
        }
    }
    return res;
}

/*
 * interp_cubique_1d
 * Formule exacte du cours (interpolation 1D entre 4 points).
 * Etant donne 4 points p0,p1,p2,p3 et un parametre t dans [0,1]
 * (t=0 -> p1, t=1 -> p2) :
 *
 *   Zoom x2 (t=0.5) :
 *     yi+1/2 = (-1*p0 + 9*p1 + 9*p2 - 1*p3) / 16
 *
 *   Formule generale :
 *     y(t) = (-p0 + 9*p1 + 9*p2 - p3) / 16   pour t=0.5
 *
 *   Pour facteur quelconque, on utilise le polynome de Hermite
 *   qui generalise la formule du cours :
 *     y(t) = 0.5 * [ (-p0 + 3p1 - 3p2 + p3)*t^3
 *                  + (2p0 - 5p1 + 4p2 - p3)*t^2
 *                  + (-p0 + p2)*t
 *                  + 2*p1 ]
 *   (equivalent a la formule /-5,60,30,-4/81 du cours pour t=1/3)
 */
static double interp_cubique_1d(double p0, double p1,
                                 double p2, double p3, double t)
{
    return 0.5 * (
        (-p0 + 3.0*p1 - 3.0*p2 + p3) * t*t*t
      + ( 2.0*p0 - 5.0*p1 + 4.0*p2 - p3) * t*t
      + (-p0 + p2) * t
      +  2.0*p1
    );
}

/*
 * zoom_bicubique
 * Interpolation bicubique (16 voisins : grille 4x4).
 * Formule du cours :
 *   - 1D : yi+1/2 = (-xi-1 + 9xi + 9xi+1 - xi+2) / 16  (zoom x2)
 *   - 2D : on applique deux fois la formule 1D
 *          d'abord sur les 4 lignes, puis sur les 4 resultats.
 * Pour un facteur quelconque, on utilise le polynome de Hermite
 * qui redonne exactement les coefficients du cours pour t=1/2 et t=1/3.
 */
ImageGris *zoom_bicubique(const ImageGris *img,
                           double facteur_x, double facteur_y)
{
    int new_w = (int)(img->largeur  * facteur_x + 0.5);
    int new_h = (int)(img->hauteur * facteur_y + 0.5);
    int xd, yd;
    ImageGris *res = allouer_image_gris(new_w, new_h);
    if (!res) return NULL;

    for (yd = 0; yd < new_h; yd++) {
        for (xd = 0; xd < new_w; xd++) {
            double xs = xd / facteur_x;
            double ys = yd / facteur_y;
            int x0 = (int)xs;
            int y0 = (int)ys;
            double tx = xs - x0;  /* fraction horizontale */
            double ty = ys - y0;  /* fraction verticale   */
            double col[4];        /* resultats des 4 lignes */
            int j, k;

            /* Etape 1 : interpolation cubique sur chaque ligne j */
            for (j = -1; j <= 2; j++) {
                double p[4];
                int yj = CLAMP(y0 + j, 0, img->hauteur - 1);
                for (k = -1; k <= 2; k++) {
                    int xk = CLAMP(x0 + k, 0, img->largeur - 1);
                    p[k+1] = PIXEL(img, xk, yj);
                }
                /* Formule cours 1D : p[0]=xi-1, p[1]=xi, p[2]=xi+1, p[3]=xi+2 */
                col[j+1] = interp_cubique_1d(p[0], p[1], p[2], p[3], tx);
            }

            /* Etape 2 : interpolation cubique sur les 4 resultats de colonnes */
            {
                double val = interp_cubique_1d(col[0], col[1], col[2], col[3], ty);
                PIXEL(res, xd, yd) = (unsigned char)CLAMP((int)(val + 0.5), 0, 255);
            }
        }
    }
    return res;
}

/* ============================================================
   PROFIL D'INTENSITE
   ============================================================ */

/*
 * profil_intensite
 * Extrait les valeurs de la ligne 'ligne' de l'image.
 * Le profil doit etre pre-alloue avec au moins img->largeur entiers.
 */
void profil_intensite(const ImageGris *img, int ligne,
                       int *profil, int *nb_points)
{
    int x;
    ligne = CLAMP(ligne, 0, img->hauteur - 1);
    *nb_points = img->largeur;
    for (x = 0; x < img->largeur; x++)
        profil[x] = PIXEL(img, x, ligne);
}

/*
 * profil_intensite_colonne
 * Extrait les valeurs d'une colonne verticale.
 */
void profil_intensite_colonne(const ImageGris *img, int colonne,
                               int *profil, int *nb_points)
{
    int y;
    colonne = CLAMP(colonne, 0, img->largeur - 1);
    *nb_points = img->hauteur;
    for (y = 0; y < img->hauteur; y++)
        profil[y] = PIXEL(img, colonne, y);
}
