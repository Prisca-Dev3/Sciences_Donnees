/* ============================================================
   segmentation.c  -  Cours 06 : Segmentation

   Contenu :
     - Seuillage simple (global, multi-seuils)
     - Seuillage automatique : Otsu
     - Seuillage local adaptatif (fenetre glissante)
     - Algorithme des k-moyennes (k-means) sur intensite
     - Division-Fusion (Split & Merge) avec quadtree
     - Croissance de regions (region growing)

   ============================================================ */

#include "image.h"

/* ============================================================
   SEUILLAGE SIMPLE (global)
   ============================================================ */

/* Seuillage binaire : pixels >= seuil -> 255, sinon -> 0
   Equivalent au seuillage(img, seuil) de traitement_base.c
   mais ici on retourne une vraie image binaire propre. */
ImageGris *seuillage_global(const ImageGris *img, int seuil)
{
    int w = img->largeur, h = img->hauteur, i, n = w * h;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = (img->pixels[i] >= seuil) ? 255 : 0;
    return res;
}

/* Seuillage multi-classes : nb_seuils valeurs dans le tableau seuils[].
   Les pixels sont classes en nb_seuils+1 niveaux (0, 1, ..., nb_seuils),
   puis lineairement ramenes sur [0,255]. */
ImageGris *seuillage_multiple(const ImageGris *img,
                               const int *seuils, int nb_seuils)
{
    int w = img->largeur, h = img->hauteur, i, n = w * h;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;
    int nb_classes = nb_seuils + 1;
    for (i = 0; i < n; i++) {
        int v = img->pixels[i], classe = 0, k;
        for (k = 0; k < nb_seuils; k++)
            if (v >= seuils[k]) classe = k + 1;
        /* Normaliser vers [0,255] */
        res->pixels[i] = (unsigned char)((classe * 255) / (nb_classes > 1 ? nb_classes - 1 : 1));
    }
    return res;
}

/* ============================================================
   SEUILLAGE AUTOMATIQUE - METHODE D'OTSU
   Minimise la variance intra-classes (equivalent a maximiser
   la variance inter-classes).
   Retourne le seuil optimal calcule.
   ============================================================ */
int otsu_seuil(const ImageGris *img)
{
    int w = img->largeur, h = img->hauteur, i;
    int n = w * h;
    int hist[256] = {0};

    /* Calcul de l'histogramme */
    for (i = 0; i < n; i++) hist[img->pixels[i]]++;

    double total = (double)n;
    double somme_totale = 0.0;
    for (i = 0; i < 256; i++) somme_totale += i * hist[i];

    double somme_b = 0.0;
    double poids_b = 0.0; /* poids classe 1 (arriere-plan) */
    double var_max = 0.0;
    int seuil = 0;

    for (int t = 0; t < 256; t++) {
        poids_b += hist[t];
        if (poids_b == 0.0) continue;

        double poids_f = total - poids_b; /* poids classe 2 (objet) */
        if (poids_f == 0.0) break;

        somme_b += t * hist[t];

        double mu_b = somme_b / poids_b;
        double mu_f = (somme_totale - somme_b) / poids_f;

        /* Variance inter-classes */
        double var = poids_b * poids_f * (mu_b - mu_f) * (mu_b - mu_f);
        if (var > var_max) {
            var_max = var;
            seuil = t;
        }
    }
    return seuil;
}

/* Seuillage automatique par la methode d'Otsu */
ImageGris *seuillage_otsu(const ImageGris *img)
{
    int s = otsu_seuil(img);
    printf("    [Otsu] seuil optimal = %d\n", s);
    return seuillage_global(img, s);
}

/* ============================================================
   SEUILLAGE LOCAL ADAPTATIF
   Pour chaque pixel, le seuil est calcule localement sur une
   fenetre de taille (taille x taille) autour du pixel.
   Le seuil local = moyenne locale (eventuellement - offset).
   ============================================================ */
ImageGris *seuillage_adaptatif(const ImageGris *img,
                                int taille_fenetre, int offset)
{
    int w = img->largeur, h = img->hauteur, x, y, kx, ky;
    int r = taille_fenetre / 2;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            long somme = 0;
            int count  = 0;
            for (ky = -r; ky <= r; ky++) {
                for (kx = -r; kx <= r; kx++) {
                    int nx = x + kx, ny = y + ky;
                    if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
                    somme += PIXEL(img, nx, ny);
                    count++;
                }
            }
            int seuil_local = (count > 0) ? (int)(somme / count) - offset : 128;
            PIXEL(res, x, y) = (PIXEL(img, x, y) >= seuil_local) ? 255 : 0;
        }
    }
    return res;
}

/* Seuillage adaptatif base sur variance :
   si variance locale < variance_min, la zone est homogene
   et on ne segmente pas (pixels laisses a 0).
   Sinon on applique Otsu local. */
ImageGris *seuillage_adaptatif_variance(const ImageGris *img,
                                         int taille_fenetre,
                                         double variance_min)
{
    int w = img->largeur, h = img->hauteur, x, y, kx, ky;
    int r = taille_fenetre / 2;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            /* Calcul moyenne et variance locale */
            double somme = 0.0, somme2 = 0.0;
            int count = 0;
            for (ky = -r; ky <= r; ky++) {
                for (kx = -r; kx <= r; kx++) {
                    int nx = x + kx, ny = y + ky;
                    if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
                    double v = PIXEL(img, nx, ny);
                    somme += v; somme2 += v * v;
                    count++;
                }
            }
            double mu  = (count > 0) ? somme / count : 0;
            double var = (count > 0) ? somme2 / count - mu * mu : 0;

            if (var < variance_min) {
                PIXEL(res, x, y) = 0; /* zone homogene */
            } else {
                /* Seuil local = moyenne */
                PIXEL(res, x, y) = (PIXEL(img, x, y) >= (int)mu) ? 255 : 0;
            }
        }
    }
    return res;
}

/* ============================================================
   ALGORITHME DES K-MOYENNES (k-means) SUR L'INTENSITE
   Parametres :
     k          : nombre de classes (clusters)
     max_iter   : iterations maximum
   Retourne une image segmentee ou chaque pixel prend la valeur
   du centre de son cluster (normalise sur [0,255]).
   ============================================================ */
ImageGris *kmoyennes(const ImageGris *img, int k, int max_iter)
{
    int w = img->largeur, h = img->hauteur, n = w * h, i, iter, j;
    if (k <= 0 || k > 256) k = 2;

    /* Tableau des centres (valeurs de gris) et des affectations */
    double *centres   = (double *)malloc(k * sizeof(double));
    int    *affectat  = (int    *)malloc(n * sizeof(int));
    ImageGris *res    = allouer_image_gris(w, h);
    if (!centres || !affectat || !res) {
        free(centres); free(affectat); liberer_image_gris(res);
        return NULL;
    }

    /* Initialisation des centres : distribues uniformement sur [0,255] */
    for (j = 0; j < k; j++)
        centres[j] = 255.0 * j / (k > 1 ? k - 1 : 1);

    /* Iterations */
    for (iter = 0; iter < max_iter; iter++) {
        /* E-step : affecter chaque pixel au cluster le plus proche */
        int changed = 0;
        for (i = 0; i < n; i++) {
            double v    = (double)img->pixels[i];
            int    best = 0;
            double dmin = fabs(v - centres[0]);
            for (j = 1; j < k; j++) {
                double d = fabs(v - centres[j]);
                if (d < dmin) { dmin = d; best = j; }
            }
            if (affectat[i] != best) { affectat[i] = best; changed = 1; }
        }
        if (!changed) break;

        /* M-step : recalculer les centres */
        double *somme = (double *)calloc(k, sizeof(double));
        int    *count = (int    *)calloc(k, sizeof(int));
        if (!somme || !count) { free(somme); free(count); break; }
        for (i = 0; i < n; i++) {
            somme[affectat[i]] += (double)img->pixels[i];
            count[affectat[i]]++;
        }
        for (j = 0; j < k; j++)
            if (count[j] > 0)
                centres[j] = somme[j] / count[j];
        free(somme); free(count);
    }

    /* Construire l'image resultat */
    for (i = 0; i < n; i++)
        res->pixels[i] = (unsigned char)CLAMP((int)centres[affectat[i]], 0, 255);

    free(centres); free(affectat);
    return res;
}

/* ============================================================
   DIVISION - FUSION (SPLIT & MERGE) avec Quadtree
   Critere d'homogeneite : max - min <= seuil_variance
   ============================================================ */

/* Noeud du quadtree */
typedef struct QuadNoeud {
    int x, y, w, h;      /* position et taille du bloc */
    int etiquette;        /* etiquette de region (apres fusion) */
    int est_feuille;
    struct QuadNoeud *enfants[4]; /* NW, NE, SW, SE */
} QuadNoeud;

/* Calcule max - min dans un bloc de l'image */
static int variance_bloc(const ImageGris *img, int bx, int by, int bw, int bh)
{
    int x, y, vmin = 255, vmax = 0;
    for (y = by; y < by + bh && y < img->hauteur; y++) {
        for (x = bx; x < bx + bw && x < img->largeur; x++) {
            int v = PIXEL(img, x, y);
            if (v < vmin) vmin = v;
            if (v > vmax) vmax = v;
        }
    }
    return vmax - vmin;
}

/* Moyenne d'un bloc */
static int moyenne_bloc(const ImageGris *img, int bx, int by, int bw, int bh)
{
    long somme = 0; int count = 0, x, y;
    for (y = by; y < by + bh && y < img->hauteur; y++)
        for (x = bx; x < bx + bw && x < img->largeur; x++) {
            somme += PIXEL(img, x, y); count++;
        }
    return count > 0 ? (int)(somme / count) : 0;
}

/* Creation d'un noeud */
static QuadNoeud *quad_creer(int x, int y, int w, int h)
{
    QuadNoeud *n = (QuadNoeud *)calloc(1, sizeof(QuadNoeud));
    if (!n) return NULL;
    n->x = x; n->y = y; n->w = w; n->h = h;
    n->est_feuille = 1;
    return n;
}

/* Liberation recursive du quadtree */
static void quad_liberer(QuadNoeud *n)
{
    if (!n) return;
    int i;
    for (i = 0; i < 4; i++) quad_liberer(n->enfants[i]);
    free(n);
}

/* Phase de division recursive */
static void quad_diviser(QuadNoeud *n, const ImageGris *img, int seuil_var)
{
    if (n->w <= 1 || n->h <= 1) return;
    if (variance_bloc(img, n->x, n->y, n->w, n->h) <= seuil_var) return;

    /* Division en 4 sous-blocs */
    int hw = n->w / 2, hh = n->h / 2;
    if (hw == 0) hw = 1;
    if (hh == 0) hh = 1;
    n->est_feuille = 0;
    n->enfants[0] = quad_creer(n->x,      n->y,      hw,         hh);
    n->enfants[1] = quad_creer(n->x + hw, n->y,      n->w - hw,  hh);
    n->enfants[2] = quad_creer(n->x,      n->y + hh, hw,         n->h - hh);
    n->enfants[3] = quad_creer(n->x + hw, n->y + hh, n->w - hw,  n->h - hh);

    int i;
    for (i = 0; i < 4; i++)
        if (n->enfants[i])
            quad_diviser(n->enfants[i], img, seuil_var);
}

/* Remplissage de l'image resultat depuis les feuilles */
static void quad_remplir(const QuadNoeud *n, const ImageGris *src,
                          ImageGris *dst)
{
    if (!n) return;
    if (n->est_feuille) {
        int val = moyenne_bloc(src, n->x, n->y, n->w, n->h);
        int x, y;
        for (y = n->y; y < n->y + n->h && y < dst->hauteur; y++)
            for (x = n->x; x < n->x + n->w && x < dst->largeur; x++)
                PIXEL(dst, x, y) = (unsigned char)val;
    } else {
        int i;
        for (i = 0; i < 4; i++) quad_remplir(n->enfants[i], src, dst);
    }
}

/* Division-Fusion (split and merge).
   seuil_var : si max-min d'un bloc <= seuil, le bloc est homogene
               et ne sera pas divise davantage.
   La fusion est approximee en moyennant les blocs adjacents apres
   division (simplification implementatoire). */
ImageGris *division_fusion(const ImageGris *img, int seuil_var)
{
    int w = img->largeur, h = img->hauteur;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;

    QuadNoeud *racine = quad_creer(0, 0, w, h);
    if (!racine) { liberer_image_gris(res); return NULL; }

    /* Phase 1 : Division */
    quad_diviser(racine, img, seuil_var);

    /* Phase 2 : Remplissage (fusion implicite par moyenne de bloc) */
    quad_remplir(racine, img, res);

    quad_liberer(racine);
    return res;
}

/* Visualise les frontieres du quadtree (lignes de separation en blanc) */
static void quad_dessiner_bords(const QuadNoeud *n, ImageGris *dst)
{
    if (!n) return;
    if (n->est_feuille) {
        /* Dessiner le bord droit et bas du bloc */
        int x, y;
        int xmax = n->x + n->w - 1;
        int ymax = n->y + n->h - 1;
        if (xmax < dst->largeur)
            for (y = n->y; y < n->y + n->h && y < dst->hauteur; y++)
                PIXEL(dst, xmax, y) = 255;
        if (ymax < dst->hauteur)
            for (x = n->x; x < n->x + n->w && x < dst->largeur; x++)
                PIXEL(dst, x, ymax) = 255;
    } else {
        int i;
        for (i = 0; i < 4; i++) quad_dessiner_bords(n->enfants[i], dst);
    }
}

/* Retourne l'image avec les frontieres du quadtree tracees
   (utile pour visualiser la segmentation). */
ImageGris *division_fusion_bords(const ImageGris *img, int seuil_var)
{
    int w = img->largeur, h = img->hauteur;
    ImageGris *res = copier_image_gris(img);
    if (!res) return NULL;

    QuadNoeud *racine = quad_creer(0, 0, w, h);
    if (!racine) { liberer_image_gris(res); return NULL; }

    quad_diviser(racine, img, seuil_var);
    quad_dessiner_bords(racine, res);
    quad_liberer(racine);
    return res;
}

/* ============================================================
   CROISSANCE DE REGIONS (Region Growing)
   Germe(s) fournis en parametres, critere = ecart au niveau
   de gris moyen de la region courant <= tolerance.
   ============================================================ */
ImageGris *croissance_region(const ImageGris *img,
                              int germe_x, int germe_y,
                              int tolerance)
{
    int w = img->largeur, h = img->hauteur;
    ImageGris *res = allouer_image_gris(w, h);
    unsigned char *visite = (unsigned char *)calloc(w * h, 1);
    int *qx = (int *)malloc(w * h * sizeof(int));
    int *qy = (int *)malloc(w * h * sizeof(int));
    if (!res || !visite || !qx || !qy) {
        free(visite); free(qx); free(qy);
        liberer_image_gris(res);
        return NULL;
    }

    /* Connexite 4 : dx/dy des voisins */
    static const int DX4[4] = {1,-1,0,0};
    static const int DY4[4] = {0,0,1,-1};

    /* File d'attente (BFS) */
    int tete = 0, queue_fin = 0;
    long somme_region = PIXEL(img, germe_x, germe_y);
    int  taille_region = 1;
    double mu = (double)somme_region;

    qx[queue_fin] = germe_x;
    qy[queue_fin] = germe_y;
    queue_fin++;
    visite[germe_y * w + germe_x] = 1;
    PIXEL(res, germe_x, germe_y) = 255;

    while (tete < queue_fin) {
        int cx = qx[tete], cy = qy[tete];
        tete++;

        int d;
        for (d = 0; d < 4; d++) {
            int nx = cx + DX4[d], ny = cy + DY4[d];
            if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
            if (visite[ny*w+nx]) continue;
            visite[ny*w+nx] = 1;

            int v = PIXEL(img, nx, ny);
            if (fabs(v - mu) <= (double)tolerance) {
                PIXEL(res, nx, ny) = 255;
                qx[queue_fin] = nx;
                qy[queue_fin] = ny;
                queue_fin++;
                somme_region += v;
                taille_region++;
                mu = (double)somme_region / taille_region;
            }
        }
    }

    free(visite); free(qx); free(qy);
    return res;
}

/* Croissance multi-germes : les germes sont distribues sur une
   grille reguliere. Chaque region prend la valeur de son indice
   de germe normalise sur [0,255]. Utile pour la comparaison
   avec les k-moyennes implementes plus haut. */
ImageGris *croissance_multi_germes(const ImageGris *img,
                                    int nb_germes_x, int nb_germes_y,
                                    int tolerance)
{
    int w = img->largeur, h = img->hauteur;
    int nb_g = nb_germes_x * nb_germes_y;
    ImageGris *res = allouer_image_gris(w, h);
    unsigned char *affectat = (unsigned char *)calloc(w * h, 1);
    /* 0 = non affecte */
    int *qx = (int *)malloc(w * h * sizeof(int));
    int *qy = (int *)malloc(w * h * sizeof(int));
    double *mu_g = (double *)malloc(nb_g * sizeof(double));
    long   *sum_g = (long  *)malloc(nb_g * sizeof(long));
    int    *cnt_g = (int   *)malloc(nb_g * sizeof(int));
    if (!res || !affectat || !qx || !qy || !mu_g || !sum_g || !cnt_g) {
        free(affectat); free(qx); free(qy);
        free(mu_g); free(sum_g); free(cnt_g);
        liberer_image_gris(res);
        return NULL;
    }

    static const int DX4[4] = {1,-1,0,0};
    static const int DY4[4] = {0,0,1,-1};

    int g, tete = 0, queue_fin = 0;
    for (g = 0; g < nb_g; g++) {
        int gi = g % nb_germes_x;
        int gj = g / nb_germes_x;
        int gx = (gi + 1) * w / (nb_germes_x + 1);
        int gy = (gj + 1) * h / (nb_germes_y + 1);
        gx = CLAMP(gx, 0, w-1);
        gy = CLAMP(gy, 0, h-1);

        sum_g[g] = PIXEL(img, gx, gy);
        cnt_g[g] = 1;
        mu_g[g]  = (double)sum_g[g];

        affectat[gy*w+gx] = (unsigned char)(g + 1);
        qx[queue_fin] = gx;
        qy[queue_fin] = gy;
        queue_fin++;
    }

    while (tete < queue_fin) {
        int cx = qx[tete], cy = qy[tete];
        int gid = (int)affectat[cy*w+cx] - 1;
        tete++;

        int d;
        for (d = 0; d < 4; d++) {
            int nx = cx + DX4[d], ny = cy + DY4[d];
            if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
            if (affectat[ny*w+nx]) continue;

            int v = PIXEL(img, nx, ny);
            if (fabs(v - mu_g[gid]) <= (double)tolerance) {
                affectat[ny*w+nx] = (unsigned char)(gid + 1);
                sum_g[gid] += v;
                cnt_g[gid]++;
                mu_g[gid] = (double)sum_g[gid] / cnt_g[gid];
                qx[queue_fin] = nx;
                qy[queue_fin] = ny;
                queue_fin++;
            }
        }
    }

    /* Construire l'image resultat */
    int i;
    for (i = 0; i < w * h; i++) {
        int gid = (int)affectat[i] - 1;
        if (gid < 0) {
            res->pixels[i] = 0;
        } else {
            res->pixels[i] = (unsigned char)((gid * 255) / (nb_g > 1 ? nb_g - 1 : 1));
        }
    }

    free(affectat); free(qx); free(qy);
    free(mu_g); free(sum_g); free(cnt_g);
    return res;
}
