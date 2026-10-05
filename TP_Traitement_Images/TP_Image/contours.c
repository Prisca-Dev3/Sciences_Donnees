/* ============================================================
   contours.c  -  Cours 05 : Detection de contours

   Contenu :
     - Derivee discrete simple ([-1,1] horizontal et vertical)
     - Filtre de Roberts  (Gx, Gy, norme, direction, seuillage)
     - Filtre de Prewitt  (Gx, Gy, norme, direction, seuillage)
     - Filtre de Sobel    (Gx, Gy, norme, direction, seuillage)
     - Filtre isotropique (Gx, Gy, norme)
     - Gradient generique : norme euclidienne, approximation L1,
       direction, carte binaire par seuillage
     - Laplacien 4-connexe
     - Laplacien 8-connexe
     - Laplacien of Gaussian (LoG) : lissage gaussien + laplacien
     - Passage a zero du laplacien (zero-crossing)
     - Rehaussement par Laplacien (image - laplacien)
   ============================================================ */

#include "image.h"

/* Assurer la disponibilite de M_PI */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============================================================
   UTILITAIRES INTERNES
   ============================================================ */

/*
 * acces_pixel_miroir
 * Retourne la valeur du pixel (x,y) avec extension miroir aux bords.
 */
static int acces_pixel_miroir(const ImageGris *img, int x, int y)
{
    int W = img->largeur;
    int H = img->hauteur;
    if (x < 0)  x = -x;
    if (x >= W) x = 2*(W-1) - x;
    if (y < 0)  y = -y;
    if (y >= H) y = 2*(H-1) - y;
    x = CLAMP(x, 0, W-1);
    y = CLAMP(y, 0, H-1);
    return PIXEL(img, x, y);
}

/* ============================================================
   DERIVEE DISCRETE SIMPLE
   Approximation : df/dx(x,y) = I(x+1,y) - I(x,y)
                   df/dy(x,y) = I(x,y+1) - I(x,y)
   Norme L1 : |dx| + |dy|, normalisee dans [0,255].
   ============================================================ */

ImageGris *derivee_discrete_x(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y, val;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            /* I(x+1,y) - I(x,y), valeur absolue, decalee de 128 */
            int p0 = acces_pixel_miroir(img, x,   y);
            int p1 = acces_pixel_miroir(img, x+1, y);
            val = CLAMP(p1 - p0 + 128, 0, 255);
            PIXEL(res, x, y) = (unsigned char)val;
        }
    }
    return res;
}

ImageGris *derivee_discrete_y(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y, val;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int p0 = acces_pixel_miroir(img, x, y);
            int p1 = acces_pixel_miroir(img, x, y+1);
            val = CLAMP(p1 - p0 + 128, 0, 255);
            PIXEL(res, x, y) = (unsigned char)val;
        }
    }
    return res;
}

ImageGris *derivee_discrete_norme(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int p00 = acces_pixel_miroir(img, x,   y);
            int p10 = acces_pixel_miroir(img, x+1, y);
            int p01 = acces_pixel_miroir(img, x,   y+1);
            int dx = p10 - p00;
            int dy = p01 - p00;
            int norme = abs(dx) + abs(dy);
            PIXEL(res, x, y) = (unsigned char)CLAMP(norme, 0, 255);
        }
    }
    return res;
}

/* ============================================================
   FILTRE DE ROBERTS (1965)
   Masques 2x2 en diagonale :
     Gx = | 0  1|    Gy = | 1  0|
          |-1  0|         | 0 -1|
   ============================================================ */

ImageGris *roberts_gx(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int v = acces_pixel_miroir(img, x+1, y)
                  - acces_pixel_miroir(img, x,   y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v + 128, 0, 255);
        }
    }
    return res;
}

ImageGris *roberts_gy(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int v = acces_pixel_miroir(img, x,   y)
                  - acces_pixel_miroir(img, x+1, y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v + 128, 0, 255);
        }
    }
    return res;
}

/*
 * roberts_norme
 * |G| = |Gx| + |Gy|  (approximation L1)
 */
ImageGris *roberts_norme(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int gx = acces_pixel_miroir(img, x+1, y)
                   - acces_pixel_miroir(img, x,   y+1);
            int gy = acces_pixel_miroir(img, x,   y)
                   - acces_pixel_miroir(img, x+1, y+1);
            int norme = abs(gx) + abs(gy);
            PIXEL(res, x, y) = (unsigned char)CLAMP(norme, 0, 255);
        }
    }
    return res;
}

/*
 * roberts_norme_euclidienne
 * |G| = sqrt(Gx^2 + Gy^2)
 */
ImageGris *roberts_norme_euclidienne(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            double gx = acces_pixel_miroir(img, x+1, y)
                      - acces_pixel_miroir(img, x,   y+1);
            double gy = acces_pixel_miroir(img, x,   y)
                      - acces_pixel_miroir(img, x+1, y+1);
            double n = sqrt(gx*gx + gy*gy);
            PIXEL(res, x, y) = (unsigned char)CLAMP((int)(n+0.5), 0, 255);
        }
    }
    return res;
}

/*
 * roberts_direction
 * theta = arctan(Gy / Gx), normalise dans [0,255] depuis [-pi,pi].
 */
ImageGris *roberts_direction(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            double gx = acces_pixel_miroir(img, x+1, y)
                      - acces_pixel_miroir(img, x,   y+1);
            double gy = acces_pixel_miroir(img, x,   y)
                      - acces_pixel_miroir(img, x+1, y+1);
            double theta = atan2(gy, gx); /* [-pi, pi] */
            int val = (int)((theta + M_PI) / (2.0*M_PI) * 255.0 + 0.5);
            PIXEL(res, x, y) = (unsigned char)CLAMP(val, 0, 255);
        }
    }
    return res;
}

/*
 * roberts_seuillage
 * Carte binaire : pixel blanc si norme > seuil, noir sinon.
 */
ImageGris *roberts_seuillage(const ImageGris *img, int seuil)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int gx = acces_pixel_miroir(img, x+1, y)
                   - acces_pixel_miroir(img, x,   y+1);
            int gy = acces_pixel_miroir(img, x,   y)
                   - acces_pixel_miroir(img, x+1, y+1);
            int norme = abs(gx) + abs(gy);
            PIXEL(res, x, y) = (norme > seuil) ? 255 : 0;
        }
    }
    return res;
}

/* ============================================================
   FILTRE DE PREWITT
   Lissage 1D uniforme + derivee :
     Gx = |-1 0 1|    Gy = |-1 -1 -1|
          |-1 0 1|         | 0  0  0|
          |-1 0 1|         | 1  1  1|
   ============================================================ */

ImageGris *prewitt_gx(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int v =
               -acces_pixel_miroir(img, x-1, y-1)
               +acces_pixel_miroir(img, x+1, y-1)
               -acces_pixel_miroir(img, x-1, y  )
               +acces_pixel_miroir(img, x+1, y  )
               -acces_pixel_miroir(img, x-1, y+1)
               +acces_pixel_miroir(img, x+1, y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v + 128, 0, 255);
        }
    }
    return res;
}

ImageGris *prewitt_gy(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int v =
               -acces_pixel_miroir(img, x-1, y-1)
               -acces_pixel_miroir(img, x,   y-1)
               -acces_pixel_miroir(img, x+1, y-1)
               +acces_pixel_miroir(img, x-1, y+1)
               +acces_pixel_miroir(img, x,   y+1)
               +acces_pixel_miroir(img, x+1, y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v + 128, 0, 255);
        }
    }
    return res;
}

ImageGris *prewitt_norme(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int gx =
               -acces_pixel_miroir(img, x-1, y-1)
               +acces_pixel_miroir(img, x+1, y-1)
               -acces_pixel_miroir(img, x-1, y  )
               +acces_pixel_miroir(img, x+1, y  )
               -acces_pixel_miroir(img, x-1, y+1)
               +acces_pixel_miroir(img, x+1, y+1);
            int gy =
               -acces_pixel_miroir(img, x-1, y-1)
               -acces_pixel_miroir(img, x,   y-1)
               -acces_pixel_miroir(img, x+1, y-1)
               +acces_pixel_miroir(img, x-1, y+1)
               +acces_pixel_miroir(img, x,   y+1)
               +acces_pixel_miroir(img, x+1, y+1);
            double n = sqrt((double)(gx*gx + gy*gy));
            PIXEL(res, x, y) = (unsigned char)CLAMP((int)(n+0.5), 0, 255);
        }
    }
    return res;
}

ImageGris *prewitt_direction(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            double gx =
               -acces_pixel_miroir(img, x-1, y-1)
               +acces_pixel_miroir(img, x+1, y-1)
               -acces_pixel_miroir(img, x-1, y  )
               +acces_pixel_miroir(img, x+1, y  )
               -acces_pixel_miroir(img, x-1, y+1)
               +acces_pixel_miroir(img, x+1, y+1);
            double gy =
               -acces_pixel_miroir(img, x-1, y-1)
               -acces_pixel_miroir(img, x,   y-1)
               -acces_pixel_miroir(img, x+1, y-1)
               +acces_pixel_miroir(img, x-1, y+1)
               +acces_pixel_miroir(img, x,   y+1)
               +acces_pixel_miroir(img, x+1, y+1);
            double theta = atan2(gy, gx);
            int val = (int)((theta + M_PI) / (2.0*M_PI) * 255.0 + 0.5);
            PIXEL(res, x, y) = (unsigned char)CLAMP(val, 0, 255);
        }
    }
    return res;
}

ImageGris *prewitt_seuillage(const ImageGris *img, int seuil)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int gx =
               -acces_pixel_miroir(img, x-1, y-1)
               +acces_pixel_miroir(img, x+1, y-1)
               -acces_pixel_miroir(img, x-1, y  )
               +acces_pixel_miroir(img, x+1, y  )
               -acces_pixel_miroir(img, x-1, y+1)
               +acces_pixel_miroir(img, x+1, y+1);
            int gy =
               -acces_pixel_miroir(img, x-1, y-1)
               -acces_pixel_miroir(img, x,   y-1)
               -acces_pixel_miroir(img, x+1, y-1)
               +acces_pixel_miroir(img, x-1, y+1)
               +acces_pixel_miroir(img, x,   y+1)
               +acces_pixel_miroir(img, x+1, y+1);
            double n = sqrt((double)(gx*gx + gy*gy));
            PIXEL(res, x, y) = (n > seuil) ? 255 : 0;
        }
    }
    return res;
}

/* ============================================================
   FILTRE DE SOBEL
   Lissage gaussien 1D + derivee :
     Gx = |-1  0  1|    Gy = |-1 -2 -1|
          |-2  0  2|         | 0  0  0|
          |-1  0  1|         | 1  2  1|
   ============================================================ */

ImageGris *sobel_gx(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int v =
               -    acces_pixel_miroir(img, x-1, y-1)
               + 0* acces_pixel_miroir(img, x,   y-1)
               +    acces_pixel_miroir(img, x+1, y-1)
               - 2* acces_pixel_miroir(img, x-1, y  )
               + 0* acces_pixel_miroir(img, x,   y  )
               + 2* acces_pixel_miroir(img, x+1, y  )
               -    acces_pixel_miroir(img, x-1, y+1)
               + 0* acces_pixel_miroir(img, x,   y+1)
               +    acces_pixel_miroir(img, x+1, y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v + 128, 0, 255);
        }
    }
    return res;
}

ImageGris *sobel_gy(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int v =
               -    acces_pixel_miroir(img, x-1, y-1)
               - 2* acces_pixel_miroir(img, x,   y-1)
               -    acces_pixel_miroir(img, x+1, y-1)
               + 0* acces_pixel_miroir(img, x-1, y  )
               + 0* acces_pixel_miroir(img, x,   y  )
               + 0* acces_pixel_miroir(img, x+1, y  )
               +    acces_pixel_miroir(img, x-1, y+1)
               + 2* acces_pixel_miroir(img, x,   y+1)
               +    acces_pixel_miroir(img, x+1, y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v + 128, 0, 255);
        }
    }
    return res;
}

ImageGris *sobel_norme(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int gx =
               - acces_pixel_miroir(img, x-1, y-1)
               + acces_pixel_miroir(img, x+1, y-1)
               - 2*acces_pixel_miroir(img, x-1, y  )
               + 2*acces_pixel_miroir(img, x+1, y  )
               - acces_pixel_miroir(img, x-1, y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            int gy =
               - acces_pixel_miroir(img, x-1, y-1)
               - 2*acces_pixel_miroir(img, x,   y-1)
               - acces_pixel_miroir(img, x+1, y-1)
               + acces_pixel_miroir(img, x-1, y+1)
               + 2*acces_pixel_miroir(img, x,   y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            double n = sqrt((double)(gx*gx + gy*gy));
            PIXEL(res, x, y) = (unsigned char)CLAMP((int)(n+0.5), 0, 255);
        }
    }
    return res;
}

/*
 * sobel_norme_l1
 * Approximation rapide : |Gx| + |Gy| (comme cours slide 16)
 */
ImageGris *sobel_norme_l1(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int gx =
               - acces_pixel_miroir(img, x-1, y-1)
               + acces_pixel_miroir(img, x+1, y-1)
               - 2*acces_pixel_miroir(img, x-1, y  )
               + 2*acces_pixel_miroir(img, x+1, y  )
               - acces_pixel_miroir(img, x-1, y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            int gy =
               - acces_pixel_miroir(img, x-1, y-1)
               - 2*acces_pixel_miroir(img, x,   y-1)
               - acces_pixel_miroir(img, x+1, y-1)
               + acces_pixel_miroir(img, x-1, y+1)
               + 2*acces_pixel_miroir(img, x,   y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            int norme = abs(gx) + abs(gy);
            PIXEL(res, x, y) = (unsigned char)CLAMP(norme, 0, 255);
        }
    }
    return res;
}

ImageGris *sobel_direction(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            double gx =
               - acces_pixel_miroir(img, x-1, y-1)
               + acces_pixel_miroir(img, x+1, y-1)
               - 2*acces_pixel_miroir(img, x-1, y  )
               + 2*acces_pixel_miroir(img, x+1, y  )
               - acces_pixel_miroir(img, x-1, y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            double gy =
               - acces_pixel_miroir(img, x-1, y-1)
               - 2*acces_pixel_miroir(img, x,   y-1)
               - acces_pixel_miroir(img, x+1, y-1)
               + acces_pixel_miroir(img, x-1, y+1)
               + 2*acces_pixel_miroir(img, x,   y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            double theta = atan2(gy, gx);
            int val = (int)((theta + M_PI) / (2.0*M_PI) * 255.0 + 0.5);
            PIXEL(res, x, y) = (unsigned char)CLAMP(val, 0, 255);
        }
    }
    return res;
}

ImageGris *sobel_seuillage(const ImageGris *img, int seuil)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int gx =
               - acces_pixel_miroir(img, x-1, y-1)
               + acces_pixel_miroir(img, x+1, y-1)
               - 2*acces_pixel_miroir(img, x-1, y  )
               + 2*acces_pixel_miroir(img, x+1, y  )
               - acces_pixel_miroir(img, x-1, y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            int gy =
               - acces_pixel_miroir(img, x-1, y-1)
               - 2*acces_pixel_miroir(img, x,   y-1)
               - acces_pixel_miroir(img, x+1, y-1)
               + acces_pixel_miroir(img, x-1, y+1)
               + 2*acces_pixel_miroir(img, x,   y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            double n = sqrt((double)(gx*gx + gy*gy));
            PIXEL(res, x, y) = (n > seuil) ? 255 : 0;
        }
    }
    return res;
}

/* ============================================================
   FILTRE ISOTROPIQUE (Frei-Chen approximation)
   Gx = |-1     0   1   |    Gy = |-1  -sqrt2  -1|
        |-sqrt2  0  sqrt2|         | 0    0      0|
        |-1     0   1   |         | 1   sqrt2   1|
   ============================================================ */

ImageGris *isotropique_gx(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    double s2 = sqrt(2.0);
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            double v =
               - acces_pixel_miroir(img, x-1, y-1)
               + acces_pixel_miroir(img, x+1, y-1)
               - s2 * acces_pixel_miroir(img, x-1, y  )
               + s2 * acces_pixel_miroir(img, x+1, y  )
               - acces_pixel_miroir(img, x-1, y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP((int)(v + 128.5), 0, 255);
        }
    }
    return res;
}

ImageGris *isotropique_gy(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    double s2 = sqrt(2.0);
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            double v =
               - acces_pixel_miroir(img, x-1, y-1)
               - s2 * acces_pixel_miroir(img, x,   y-1)
               - acces_pixel_miroir(img, x+1, y-1)
               + acces_pixel_miroir(img, x-1, y+1)
               + s2 * acces_pixel_miroir(img, x,   y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP((int)(v + 128.5), 0, 255);
        }
    }
    return res;
}

ImageGris *isotropique_norme(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    double s2 = sqrt(2.0);
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            double gx =
               - acces_pixel_miroir(img, x-1, y-1)
               + acces_pixel_miroir(img, x+1, y-1)
               - s2 * acces_pixel_miroir(img, x-1, y  )
               + s2 * acces_pixel_miroir(img, x+1, y  )
               - acces_pixel_miroir(img, x-1, y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            double gy =
               - acces_pixel_miroir(img, x-1, y-1)
               - s2 * acces_pixel_miroir(img, x,   y-1)
               - acces_pixel_miroir(img, x+1, y-1)
               + acces_pixel_miroir(img, x-1, y+1)
               + s2 * acces_pixel_miroir(img, x,   y+1)
               + acces_pixel_miroir(img, x+1, y+1);
            double n = sqrt(gx*gx + gy*gy);
            PIXEL(res, x, y) = (unsigned char)CLAMP((int)(n+0.5), 0, 255);
        }
    }
    return res;
}

/* ============================================================
   LAPLACIEN
   Operateur de la seconde derivee (detection aux passages a zero)

   4-connexe :  | 0  1  0|     8-connexe :  |1  1  1|
                | 1 -4  1|                  |1 -8  1|
                | 0  1  0|                  |1  1  1|
   ============================================================ */

/*
 * laplacien_4
 * Masque 4-connexe. Resultat centre a 128 pour visualisation.
 */
ImageGris *laplacien_4(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int v =
                  acces_pixel_miroir(img, x,   y-1)
                + acces_pixel_miroir(img, x-1, y  )
                - 4 * acces_pixel_miroir(img, x, y)
                + acces_pixel_miroir(img, x+1, y  )
                + acces_pixel_miroir(img, x,   y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v + 128, 0, 255);
        }
    }
    return res;
}

/*
 * laplacien_8
 * Masque 8-connexe (inclut les diagonales).
 */
ImageGris *laplacien_8(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int v =
                  acces_pixel_miroir(img, x-1, y-1)
                + acces_pixel_miroir(img, x,   y-1)
                + acces_pixel_miroir(img, x+1, y-1)
                + acces_pixel_miroir(img, x-1, y  )
                - 8 * acces_pixel_miroir(img, x, y)
                + acces_pixel_miroir(img, x+1, y  )
                + acces_pixel_miroir(img, x-1, y+1)
                + acces_pixel_miroir(img, x,   y+1)
                + acces_pixel_miroir(img, x+1, y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP(v + 128, 0, 255);
        }
    }
    return res;
}

/*
 * laplacien_absolu
 * Valeur absolue du laplacien 8-connexe, pour la norme brute.
 */
ImageGris *laplacien_absolu(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int v =
                  acces_pixel_miroir(img, x-1, y-1)
                + acces_pixel_miroir(img, x,   y-1)
                + acces_pixel_miroir(img, x+1, y-1)
                + acces_pixel_miroir(img, x-1, y  )
                - 8 * acces_pixel_miroir(img, x, y)
                + acces_pixel_miroir(img, x+1, y  )
                + acces_pixel_miroir(img, x-1, y+1)
                + acces_pixel_miroir(img, x,   y+1)
                + acces_pixel_miroir(img, x+1, y+1);
            PIXEL(res, x, y) = (unsigned char)CLAMP(abs(v), 0, 255);
        }
    }
    return res;
}

/* ============================================================
   PASSAGE A ZERO DU LAPLACIEN
   Un pixel est un contour si son laplacien change de signe
   dans son voisinage 4-connexe.
   ============================================================ */
ImageGris *zero_crossing(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    /* Calcul du laplacien brut (non clamp) dans un buffer double */
    double *lap = (double *)malloc(W * H * sizeof(double));
    ImageGris *res;
    if (!lap) return NULL;

    res = allouer_image_gris(W, H);
    if (!res) { free(lap); return NULL; }

    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            lap[y*W+x] =
                  acces_pixel_miroir(img, x,   y-1)
                + acces_pixel_miroir(img, x-1, y  )
                - 4.0 * acces_pixel_miroir(img, x, y)
                + acces_pixel_miroir(img, x+1, y  )
                + acces_pixel_miroir(img, x,   y+1);
        }
    }

    /* Passage a zero : pixel blanc si changement de signe dans les 4-voisins */
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            double c = lap[y*W+x];
            int contour = 0;
            /* Verifie les 4 directions */
            if (x > 0   && c * lap[y*W+(x-1)] < 0) contour = 1;
            if (x < W-1 && c * lap[y*W+(x+1)] < 0) contour = 1;
            if (y > 0   && c * lap[(y-1)*W+x] < 0) contour = 1;
            if (y < H-1 && c * lap[(y+1)*W+x] < 0) contour = 1;
            PIXEL(res, x, y) = contour ? 255 : 0;
        }
    }
    free(lap);
    return res;
}

/* ============================================================
   LAPLACIEN OF GAUSSIAN (LoG) - "Chapeau mexicain"
   1. Lissage gaussien pour reduire le bruit
   2. Application du laplacien
   ============================================================ */
static ImageGris *gaussien_simple(const ImageGris *img, double sigma)
{
    /* Mini-gaussien 5x5 pour le pre-lissage */
    int taille = 5, demi = 2, i, u, v;
    double *noyau = (double *)malloc(taille * taille * sizeof(double));
    double somme = 0.0;
    ImageGris *res;
    if (!noyau) return NULL;

    i = 0;
    for (v = -demi; v <= demi; v++) {
        for (u = -demi; u <= demi; u++) {
            noyau[i] = exp(-(u*u + v*v) / (2.0*sigma*sigma));
            somme += noyau[i];
            i++;
        }
    }
    for (i = 0; i < taille*taille; i++) noyau[i] /= somme;

    /* Application manuelle sans dependance a convolution.c */
    {
        int W = img->largeur, H = img->hauteur, x, y;
        res = allouer_image_gris(W, H);
        if (!res) { free(noyau); return NULL; }
        for (y = 0; y < H; y++) {
            for (x = 0; x < W; x++) {
                double acc = 0.0;
                for (v = -demi; v <= demi; v++) {
                    for (u = -demi; u <= demi; u++) {
                        acc += noyau[(v+demi)*taille+(u+demi)]
                             * acces_pixel_miroir(img, x+u, y+v);
                    }
                }
                PIXEL(res, x, y) = (unsigned char)CLAMP((int)(acc+0.5), 0, 255);
            }
        }
    }
    free(noyau);
    return res;
}

ImageGris *laplacien_of_gaussian(const ImageGris *img, double sigma)
{
    ImageGris *lisse = gaussien_simple(img, sigma);
    ImageGris *res;
    if (!lisse) return NULL;
    res = laplacien_8(lisse);
    liberer_image_gris(lisse);
    return res;
}

/*
 * zero_crossing_log
 * Passage a zero sur le LoG.
 */
ImageGris *zero_crossing_log(const ImageGris *img, double sigma)
{
    ImageGris *lisse = gaussien_simple(img, sigma);
    ImageGris *res;
    if (!lisse) return NULL;
    res = zero_crossing(lisse);
    liberer_image_gris(lisse);
    return res;
}

/* ============================================================
   REHAUSSEMENT PAR LAPLACIEN
   Image rehaussee = image originale - laplacien(image)
   Permet d'accentuer les contours tout en conservant l'image.
   ============================================================ */
ImageGris *rehaussement_laplacien(const ImageGris *img)
{
    int W = img->largeur, H = img->hauteur;
    int x, y;
    ImageGris *res = allouer_image_gris(W, H);
    if (!res) return NULL;
    for (y = 0; y < H; y++) {
        for (x = 0; x < W; x++) {
            int orig = PIXEL(img, x, y);
            int lap =
                  acces_pixel_miroir(img, x-1, y-1)
                + acces_pixel_miroir(img, x,   y-1)
                + acces_pixel_miroir(img, x+1, y-1)
                + acces_pixel_miroir(img, x-1, y  )
                - 8 * orig
                + acces_pixel_miroir(img, x+1, y  )
                + acces_pixel_miroir(img, x-1, y+1)
                + acces_pixel_miroir(img, x,   y+1)
                + acces_pixel_miroir(img, x+1, y+1);
            /* I_rehaus = I - Laplacien(I) */
            int val = orig - lap;
            PIXEL(res, x, y) = (unsigned char)CLAMP(val, 0, 255);
        }
    }
    return res;
}
