/* ============================================================
   images_binaires.c  -  Cours 07 : Images Binaires

   Contenu :
     - Connexite 4 et 8 (voisinage)
     - Distances discretes D4 (Manhattan) et D8 (Echiquier)
     - Codage de Freeman (8 directions)
     - Etiquetage de composantes connexes (2 passes + table equiv.)
     - Operateurs morphologiques :
         erosion, dilatation, ouverture, fermeture
         gradient interne, gradient externe, gradient morphologique
     - Fermeture de contours par seuillage par hysteresis

   Convention : pixel blanc (255) = objet, pixel noir (0) = fond
   ============================================================ */

#include "image.h"

/* ============================================================
   UTILITAIRES INTERNES
   ============================================================ */

/* Cree une image binaire (0/255) a partir d'une image gris
   avec un seuil donne. */
ImageGris *binariser(const ImageGris *img, int seuil)
{
    int w = img->largeur, h = img->hauteur, i, n = w * h;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;
    for (i = 0; i < n; i++)
        res->pixels[i] = (img->pixels[i] >= seuil) ? 255 : 0;
    return res;
}

/* ============================================================
   CONNEXITE : VOISINAGES N4 et N8
   ============================================================ */

/* Retourne 1 si les pixels (x1,y1) et (x2,y2) sont connexes-4
   (partage d'un cote commun) */
int connexe_4(int x1, int y1, int x2, int y2)
{
    int dx = abs(x1 - x2), dy = abs(y1 - y2);
    return (dx + dy == 1);
}

/* Retourne 1 si les pixels (x1,y1) et (x2,y2) sont connexes-8
   (partage d'un cote ou d'un coin) */
int connexe_8(int x1, int y1, int x2, int y2)
{
    int dx = abs(x1 - x2), dy = abs(y1 - y2);
    return (dx <= 1 && dy <= 1 && (dx + dy > 0));
}

/* ============================================================
   DISTANCES DISCRETES
   ============================================================ */

/* Distance D4 (Manhattan) : |x1-x2| + |y1-y2|
   Forme un diamant autour du point central. */
int distance_D4(int x1, int y1, int x2, int y2)
{
    return abs(x1 - x2) + abs(y1 - y2);
}

/* Distance D8 (echiquier) : max(|x1-x2|, |y1-y2|)
   Forme un carre autour du point central. */
int distance_D8(int x1, int y1, int x2, int y2)
{
    int dx = abs(x1 - x2), dy = abs(y1 - y2);
    return (dx > dy) ? dx : dy;
}

/* Carte de distances D4 : chaque pixel vaut sa distance D4
   au pixel le plus proche de l'objet (valeur 255).
   Utilise un double passage (avant + arriere). */
ImageGris *carte_distance_D4(const ImageGris *bin)
{
    int w = bin->largeur, h = bin->hauteur, x, y, v;
    int *dist = (int *)malloc(w * h * sizeof(int));
    ImageGris *res = allouer_image_gris(w, h);
    if (!dist || !res) { free(dist); liberer_image_gris(res); return NULL; }

    /* Initialisation */
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            dist[y*w+x] = (PIXEL(bin,x,y) == 0) ? 0 : 999999;

    /* Passe avant : haut-gauche -> bas-droite */
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (dist[y*w+x] == 0) continue;
            v = dist[y*w+x];
            if (y > 0 && dist[(y-1)*w+x] + 1 < v) v = dist[(y-1)*w+x] + 1;
            if (x > 0 && dist[y*w+(x-1)] + 1 < v) v = dist[y*w+(x-1)] + 1;
            dist[y*w+x] = v;
        }
    }
    /* Passe arriere : bas-droite -> haut-gauche */
    for (y = h-1; y >= 0; y--) {
        for (x = w-1; x >= 0; x--) {
            v = dist[y*w+x];
            if (y < h-1 && dist[(y+1)*w+x] + 1 < v) v = dist[(y+1)*w+x] + 1;
            if (x < w-1 && dist[y*w+(x+1)] + 1 < v) v = dist[y*w+(x+1)] + 1;
            dist[y*w+x] = v;
        }
    }

    /* Normalisation vers [0,255] */
    int dmax = 1;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            if (dist[y*w+x] < 999999 && dist[y*w+x] > dmax)
                dmax = dist[y*w+x];
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            int d = dist[y*w+x];
            PIXEL(res,x,y) = (d >= 999999) ? 255 : (unsigned char)(d * 255 / dmax);
        }

    free(dist);
    return res;
}

/* Carte de distances D8 (echiquier) */
ImageGris *carte_distance_D8(const ImageGris *bin)
{
    int w = bin->largeur, h = bin->hauteur, x, y, v;
    int *dist = (int *)malloc(w * h * sizeof(int));
    ImageGris *res = allouer_image_gris(w, h);
    if (!dist || !res) { free(dist); liberer_image_gris(res); return NULL; }

    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            dist[y*w+x] = (PIXEL(bin,x,y) == 0) ? 0 : 999999;

    /* Passe avant */
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (dist[y*w+x] == 0) continue;
            v = dist[y*w+x];
            if (y > 0)             { int t = dist[(y-1)*w+x]+1; if(t<v) v=t; }
            if (x > 0)             { int t = dist[y*w+(x-1)]+1; if(t<v) v=t; }
            if (y > 0 && x > 0)    { int t = dist[(y-1)*w+(x-1)]+1; if(t<v) v=t; }
            if (y > 0 && x < w-1)  { int t = dist[(y-1)*w+(x+1)]+1; if(t<v) v=t; }
            dist[y*w+x] = v;
        }
    }
    /* Passe arriere */
    for (y = h-1; y >= 0; y--) {
        for (x = w-1; x >= 0; x--) {
            v = dist[y*w+x];
            if (y < h-1)             { int t = dist[(y+1)*w+x]+1; if(t<v) v=t; }
            if (x < w-1)             { int t = dist[y*w+(x+1)]+1; if(t<v) v=t; }
            if (y < h-1 && x < w-1) { int t = dist[(y+1)*w+(x+1)]+1; if(t<v) v=t; }
            if (y < h-1 && x > 0)   { int t = dist[(y+1)*w+(x-1)]+1; if(t<v) v=t; }
            dist[y*w+x] = v;
        }
    }

    int dmax = 1;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            if (dist[y*w+x] < 999999 && dist[y*w+x] > dmax)
                dmax = dist[y*w+x];
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            int d = dist[y*w+x];
            PIXEL(res,x,y) = (d >= 999999) ? 255 : (unsigned char)(d * 255 / dmax);
        }

    free(dist);
    return res;
}

/* ============================================================
   CODAGE DE FREEMAN (8 directions)
   Direction : 0=E, 1=SE, 2=S, 3=SO, 4=O, 5=NO, 6=N, 7=NE
     5 6 7
     4 x 0
     3 2 1
   ============================================================ */

/* Increments dx, dy pour chaque direction Freeman */
static const int FREEMAN_DX[8] = { 1,  1,  0, -1, -1, -1,  0,  1};
static const int FREEMAN_DY[8] = { 0,  1,  1,  1,  0, -1, -1, -1};

/* Code le contour d'un objet binaire en codage Freeman 8-connexe.
   Retourne la longueur du code, ou -1 si aucun pixel trouve.
   Le tableau 'code' doit etre assez grand (largeur*hauteur).
   start_x, start_y : premier pixel du contour (haut-gauche). */
int coder_freeman(const ImageGris *bin, int *code, int max_code,
                  int *start_x, int *start_y)
{
    int w = bin->largeur, h = bin->hauteur, x, y, d, nx, ny;
    int cx, cy, longueur = 0;

    /* Trouver le premier pixel objet (parcours haut-gauche) */
    *start_x = *start_y = -1;
    for (y = 0; y < h && *start_y < 0; y++)
        for (x = 0; x < w && *start_y < 0; x++)
            if (PIXEL(bin, x, y) == 255)
            { *start_x = x; *start_y = y; }

    if (*start_y < 0) return -1; /* aucun pixel objet */

    cx = *start_x; cy = *start_y;

    /* Direction de debut : on part de la direction 0 (est) */
    d = 0;
    do {
        int trouve = 0;
        /* Chercher le premier voisin objet en tournant dans le sens
           des aiguilles d'une montre a partir de la direction d */
        int di;
        for (di = 0; di < 8; di++) {
            int dir = (d + di) % 8;
            nx = cx + FREEMAN_DX[dir];
            ny = cy + FREEMAN_DY[dir];
            if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
            if (PIXEL(bin, nx, ny) == 255) {
                if (longueur < max_code)
                    code[longueur] = dir;
                longueur++;
                /* Direction de retour pour le prochain pas :
                   on repart depuis (dir+5)%8 pour tourner CW */
                d = (dir + 5) % 8;
                cx = nx; cy = ny;
                trouve = 1;
                break;
            }
        }
        if (!trouve) break;
    } while ((cx != *start_x || cy != *start_y) && longueur < max_code);

    return longueur;
}

/* Visualise le code de Freeman : dessine le contour sur une image gris
   en marquant chaque pixel de contour a 128 et le point de depart a 255. */
ImageGris *visualiser_freeman(const ImageGris *bin)
{
    int w = bin->largeur, h = bin->hauteur;
    int *code = (int *)malloc(w * h * sizeof(int));
    ImageGris *res = copier_image_gris(bin);
    if (!code || !res) { free(code); liberer_image_gris(res); return NULL; }

    int sx, sy, len, i, cx, cy;
    len = coder_freeman(bin, code, w*h, &sx, &sy);

    if (len > 0) {
        cx = sx; cy = sy;
        PIXEL(res, cx, cy) = 200; /* point de depart */
        for (i = 0; i < len && i < w*h; i++) {
            cx += FREEMAN_DX[code[i]];
            cy += FREEMAN_DY[code[i]];
            if (cx >= 0 && cx < w && cy >= 0 && cy < h)
                PIXEL(res, cx, cy) = 128;
        }
    }
    free(code);
    return res;
}

/* ============================================================
   ETIQUETAGE DE COMPOSANTES CONNEXES
   Algorithme : 2 passes (Rosenfeld & Pfaltz) avec table d'equivalence.
   Connexite parametre : 4 ou 8.
   Retourne une image gris ou chaque niveau encode l'etiquette
   (normalise a l'affichage). nb_composantes est mis a jour.
   ============================================================ */

/* Table d'union-find est utile pour la gestion des equivalences */
static int uf_find(int *parent, int x)
{
    while (parent[x] != x) {
        parent[x] = parent[parent[x]]; /* compression de chemin */
        x = parent[x];
    }
    return x;
}

static void uf_union(int *parent, int a, int b)
{
    int ra = uf_find(parent, a);
    int rb = uf_find(parent, b);
    if (ra != rb) parent[rb] = ra;
}

ImageGris *etiqueter_composantes(const ImageGris *bin, int connexite,
                                  int *nb_composantes)
{
    int w = bin->largeur, h = bin->hauteur, x, y;
    int max_labels = w * h / 2 + 2;
    int *labels  = (int *)calloc(w * h, sizeof(int));
    int *parent  = (int *)malloc(max_labels * sizeof(int));
    ImageGris *res = allouer_image_gris(w, h);
    if (!labels || !parent || !res) {
        free(labels); free(parent); liberer_image_gris(res);
        return NULL;
    }

    /* Initialisation union-find */
    int next_label = 1;
    for (int k = 0; k < max_labels; k++) parent[k] = k;

    /* ---- Premiere passe ---- */
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (PIXEL(bin, x, y) == 0) { labels[y*w+x] = 0; continue; }

            /* Collecter les etiquettes des voisins deja traites */
            int voisins[4], nv = 0;
            /* voisin haut */
            if (y > 0 && PIXEL(bin, x, y-1) != 0)
                voisins[nv++] = labels[(y-1)*w+x];
            /* voisin gauche */
            if (x > 0 && PIXEL(bin, x-1, y) != 0)
                voisins[nv++] = labels[y*w+(x-1)];
            /* Connexite 8 : diagonales haut */
            if (connexite == 8) {
                if (y > 0 && x > 0 && PIXEL(bin, x-1, y-1) != 0)
                    voisins[nv++] = labels[(y-1)*w+(x-1)];
                if (y > 0 && x < w-1 && PIXEL(bin, x+1, y-1) != 0)
                    voisins[nv++] = labels[(y-1)*w+(x+1)];
            }

            if (nv == 0) {
                /* Nouveau label */
                if (next_label < max_labels)
                    labels[y*w+x] = next_label++;
            } else {
                /* Trouver le plus petit label racine parmi les voisins */
                int min_root = uf_find(parent, voisins[0]);
                int vi;
                for (vi = 1; vi < nv; vi++) {
                    int r = uf_find(parent, voisins[vi]);
                    if (r < min_root) min_root = r;
                }
                labels[y*w+x] = min_root;
                /* Fusionner toutes les equivalences */
                for (vi = 0; vi < nv; vi++)
                    uf_union(parent, min_root, voisins[vi]);
            }
        }
    }

    /* ---- Deuxieme passe : appliquer la racine a chaque pixel ---- */
    /* Recompacter les labels : creer une table de remapping */
    int *remap = (int *)calloc(max_labels, sizeof(int));
    int count = 0;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (labels[y*w+x] == 0) continue;
            int root = uf_find(parent, labels[y*w+x]);
            labels[y*w+x] = root;
            if (remap[root] == 0) remap[root] = ++count;
        }
    }

    if (nb_composantes) *nb_composantes = count;

    /* Ecrire dans l'image resultat en normalisant vers [1,255] */
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (labels[y*w+x] == 0) {
                PIXEL(res, x, y) = 0;
            } else {
                int etiq = remap[labels[y*w+x]];
                /* Distribue les etiquettes sur [32, 255] pour la lisibilite */
                PIXEL(res, x, y) = (unsigned char)(32 + (etiq * 223) / (count > 1 ? count : 1));
            }
        }
    }

    free(labels); free(parent); free(remap);
    return res;
}

/* ============================================================
   OPERATEURS MORPHOLOGIQUES
   Element structurant : carre NxN (taille impaire)
   Convention : objet = 255, fond = 0
   ============================================================ */

/* ----- EROSION -----
   Un pixel reste objet SI TOUS les pixels de l'element structurant
   centres sur lui sont des pixels objet. */
ImageGris *erosion(const ImageGris *bin, int taille)
{
    int w = bin->largeur, h = bin->hauteur, x, y, kx, ky;
    int r = taille / 2;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            if (PIXEL(bin, x, y) == 0) { PIXEL(res, x, y) = 0; continue; }
            int ok = 1;
            for (ky = -r; ky <= r && ok; ky++) {
                for (kx = -r; kx <= r && ok; kx++) {
                    int nx = x + kx, ny = y + ky;
                    if (nx < 0 || nx >= w || ny < 0 || ny >= h) { ok = 0; break; }
                    if (PIXEL(bin, nx, ny) == 0) ok = 0;
                }
            }
            PIXEL(res, x, y) = ok ? 255 : 0;
        }
    }
    return res;
}

/* ----- DILATATION -----
   Un pixel devient objet SI AU MOINS UN pixel de l'element structurant
   centres sur lui est un pixel objet. */
ImageGris *dilatation(const ImageGris *bin, int taille)
{
    int w = bin->largeur, h = bin->hauteur, x, y, kx, ky;
    int r = taille / 2;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int trouve = 0;
            for (ky = -r; ky <= r && !trouve; ky++) {
                for (kx = -r; kx <= r && !trouve; kx++) {
                    int nx = x + kx, ny = y + ky;
                    if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
                    if (PIXEL(bin, nx, ny) == 255) trouve = 1;
                }
            }
            PIXEL(res, x, y) = trouve ? 255 : 0;
        }
    }
    return res;
}

/* ----- OUVERTURE : erosion puis dilatation -----
   Supprime les petits objets isoles et lisse les contours. */
ImageGris *ouverture(const ImageGris *bin, int taille)
{
    ImageGris *tmp = erosion(bin, taille);
    ImageGris *res = dilatation(tmp, taille);
    liberer_image_gris(tmp);
    return res;
}

/* ----- FERMETURE : dilatation puis erosion -----
   Comble les petits trous et soude les objets proches. */
ImageGris *fermeture(const ImageGris *bin, int taille)
{
    ImageGris *tmp = dilatation(bin, taille);
    ImageGris *res = erosion(tmp, taille);
    liberer_image_gris(tmp);
    return res;
}

/* ----- GRADIENT INTERNE : A - erosion(A) -----
   Contour interieur de l'objet. */
ImageGris *gradient_interne(const ImageGris *bin, int taille)
{
    int w = bin->largeur, h = bin->hauteur, i, n = w * h;
    ImageGris *erode = erosion(bin, taille);
    ImageGris *res   = allouer_image_gris(w, h);
    if (!erode || !res) { liberer_image_gris(erode); liberer_image_gris(res); return NULL; }
    for (i = 0; i < n; i++) {
        int v = (int)bin->pixels[i] - (int)erode->pixels[i];
        res->pixels[i] = (unsigned char)CLAMP(v, 0, 255);
    }
    liberer_image_gris(erode);
    return res;
}

/* ----- GRADIENT EXTERNE : dilatation(A) - A -----
   Contour exterieur de l'objet. */
ImageGris *gradient_externe(const ImageGris *bin, int taille)
{
    int w = bin->largeur, h = bin->hauteur, i, n = w * h;
    ImageGris *dilat = dilatation(bin, taille);
    ImageGris *res   = allouer_image_gris(w, h);
    if (!dilat || !res) { liberer_image_gris(dilat); liberer_image_gris(res); return NULL; }
    for (i = 0; i < n; i++) {
        int v = (int)dilat->pixels[i] - (int)bin->pixels[i];
        res->pixels[i] = (unsigned char)CLAMP(v, 0, 255);
    }
    liberer_image_gris(dilat);
    return res;
}

/* ----- GRADIENT MORPHOLOGIQUE : dilatation(A) - erosion(A) -----
   Contour symetrique (interieur + exterieur). */
ImageGris *gradient_morphologique(const ImageGris *bin, int taille)
{
    int w = bin->largeur, h = bin->hauteur, i, n = w * h;
    ImageGris *dilat = dilatation(bin, taille);
    ImageGris *erode = erosion(bin, taille);
    ImageGris *res   = allouer_image_gris(w, h);
    if (!dilat || !erode || !res) {
        liberer_image_gris(dilat); liberer_image_gris(erode);
        liberer_image_gris(res); return NULL;
    }
    for (i = 0; i < n; i++) {
        int v = (int)dilat->pixels[i] - (int)erode->pixels[i];
        res->pixels[i] = (unsigned char)CLAMP(v, 0, 255);
    }
    liberer_image_gris(dilat);
    liberer_image_gris(erode);
    return res;
}

/* ============================================================
   FERMETURE DE CONTOURS PAR SEUILLAGE PAR HYSTERESIS
   (Canny-style hysteresis thresholding)
   seuil_bas  : seuil minimum pour etre contour potentiel
   seuil_haut : seuil pour etre contour fort (anchor)
   Algorithme :
     1. Marquer les pixels > seuil_haut  comme contours forts (255)
     2. Marquer les pixels entre bas et haut comme contours faibles (128)
     3. Propager : un contour faible devient fort si connexe-8 a un fort
   ============================================================ */
ImageGris *seuillage_hysteresis(const ImageGris *gradient,
                                 int seuil_bas, int seuil_haut)
{
    int w = gradient->largeur, h = gradient->hauteur, x, y;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;

    /* Etape 1 & 2 : classification initiale */
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int v = PIXEL(gradient, x, y);
            if      (v >= seuil_haut) PIXEL(res, x, y) = 255; /* fort */
            else if (v >= seuil_bas)  PIXEL(res, x, y) = 128; /* faible */
            else                       PIXEL(res, x, y) = 0;   /* rien */
        }
    }

    /* Etape 3 : propagation iterative jusqu'a convergence */
    int changed = 1;
    while (changed) {
        changed = 0;
        for (y = 1; y < h-1; y++) {
            for (x = 1; x < w-1; x++) {
                if (PIXEL(res, x, y) != 128) continue;
                /* Verifier si un voisin 8-connexe est fort */
                int kx, ky, has_strong = 0;
                for (ky = -1; ky <= 1 && !has_strong; ky++)
                    for (kx = -1; kx <= 1 && !has_strong; kx++)
                        if (PIXEL(res, x+kx, y+ky) == 255)
                            has_strong = 1;
                if (has_strong) {
                    PIXEL(res, x, y) = 255;
                    changed = 1;
                }
            }
        }
    }

    /* Supprime les contours faibles non connectes */
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            if (PIXEL(res, x, y) == 128)
                PIXEL(res, x, y) = 0;

    return res;
}

/* ============================================================
   MORPHOLOGIE SUR IMAGE EN NIVEAUX DE GRIS
   (extension des operateurs binaires : min/max locaux)
   ============================================================ */

/* Erosion gris : minimum local dans la fenetre NxN */
ImageGris *erosion_gris(const ImageGris *img, int taille)
{
    int w = img->largeur, h = img->hauteur, x, y, kx, ky;
    int r = taille / 2;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int vmin = 255;
            for (ky = -r; ky <= r; ky++) {
                for (kx = -r; kx <= r; kx++) {
                    int nx = CLAMP(x+kx, 0, w-1);
                    int ny = CLAMP(y+ky, 0, h-1);
                    int v  = PIXEL(img, nx, ny);
                    if (v < vmin) vmin = v;
                }
            }
            PIXEL(res, x, y) = (unsigned char)vmin;
        }
    }
    return res;
}

/* Dilatation gris : maximum local dans la fenetre NxN */
ImageGris *dilatation_gris(const ImageGris *img, int taille)
{
    int w = img->largeur, h = img->hauteur, x, y, kx, ky;
    int r = taille / 2;
    ImageGris *res = allouer_image_gris(w, h);
    if (!res) return NULL;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int vmax = 0;
            for (ky = -r; ky <= r; ky++) {
                for (kx = -r; kx <= r; kx++) {
                    int nx = CLAMP(x+kx, 0, w-1);
                    int ny = CLAMP(y+ky, 0, h-1);
                    int v  = PIXEL(img, nx, ny);
                    if (v > vmax) vmax = v;
                }
            }
            PIXEL(res, x, y) = (unsigned char)vmax;
        }
    }
    return res;
}

/* Ouverture gris */
ImageGris *ouverture_gris(const ImageGris *img, int taille)
{
    ImageGris *tmp = erosion_gris(img, taille);
    ImageGris *res = dilatation_gris(tmp, taille);
    liberer_image_gris(tmp);
    return res;
}

/* Fermeture gris */
ImageGris *fermeture_gris(const ImageGris *img, int taille)
{
    ImageGris *tmp = dilatation_gris(img, taille);
    ImageGris *res = erosion_gris(tmp, taille);
    liberer_image_gris(tmp);
    return res;
}

/* Gradient morphologique gris */
ImageGris *gradient_morphologique_gris(const ImageGris *img, int taille)
{
    int w = img->largeur, h = img->hauteur, i, n = w * h;
    ImageGris *dilat = dilatation_gris(img, taille);
    ImageGris *erode = erosion_gris(img, taille);
    ImageGris *res   = allouer_image_gris(w, h);
    if (!dilat || !erode || !res) {
        liberer_image_gris(dilat); liberer_image_gris(erode);
        liberer_image_gris(res); return NULL;
    }
    for (i = 0; i < n; i++) {
        int v = (int)dilat->pixels[i] - (int)erode->pixels[i];
        res->pixels[i] = (unsigned char)CLAMP(v, 0, 255);
    }
    liberer_image_gris(dilat);
    liberer_image_gris(erode);
    return res;
}
