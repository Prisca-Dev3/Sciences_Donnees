/* ============================================================
   io_image.c - Lecture/ecriture PGM/PPM, allocation, conversion,
                chargement universel, operations couleur
   ============================================================ */

#include "image.h"

/* ============================================================
   ALLOCATION / LIBERATION / COPIE
   ============================================================ */

ImageGris *allouer_image_gris(int largeur, int hauteur)
{
    ImageGris *img = (ImageGris *)malloc(sizeof(ImageGris));
    if (!img) { perror("malloc ImageGris"); return NULL; }
    img->largeur = largeur;
    img->hauteur = hauteur;
    img->max_val = 255;
    img->pixels  = (unsigned char *)calloc(largeur * hauteur,
                                            sizeof(unsigned char));
    if (!img->pixels) {
        perror("calloc pixels"); free(img); return NULL;
    }
    return img;
}

ImageCouleur *allouer_image_couleur(int largeur, int hauteur)
{
    ImageCouleur *img = (ImageCouleur *)malloc(sizeof(ImageCouleur));
    if (!img) { perror("malloc ImageCouleur"); return NULL; }
    img->largeur = largeur;
    img->hauteur = hauteur;
    img->max_val = 255;
    img->r = (unsigned char *)calloc(largeur*hauteur, sizeof(unsigned char));
    img->g = (unsigned char *)calloc(largeur*hauteur, sizeof(unsigned char));
    img->b = (unsigned char *)calloc(largeur*hauteur, sizeof(unsigned char));
    if (!img->r || !img->g || !img->b) {
        perror("calloc canaux"); free(img->r); free(img->g);
        free(img->b); free(img); return NULL;
    }
    return img;
}

void liberer_image_gris(ImageGris *img)
{
    if (!img) return;
    free(img->pixels);
    free(img);
}

void liberer_image_couleur(ImageCouleur *img)
{
    if (!img) return;
    free(img->r); free(img->g); free(img->b); free(img);
}

ImageGris *copier_image_gris(const ImageGris *src)
{
    ImageGris *dst;
    if (!src) return NULL;
    dst = allouer_image_gris(src->largeur, src->hauteur);
    if (!dst) return NULL;
    dst->max_val = src->max_val;
    memcpy(dst->pixels, src->pixels,
           src->largeur * src->hauteur * sizeof(unsigned char));
    return dst;
}

ImageCouleur *copier_image_couleur(const ImageCouleur *src)
{
    int n;
    ImageCouleur *dst;
    if (!src) return NULL;
    dst = allouer_image_couleur(src->largeur, src->hauteur);
    if (!dst) return NULL;
    dst->max_val = src->max_val;
    n = src->largeur * src->hauteur;
    memcpy(dst->r, src->r, n); memcpy(dst->g, src->g, n);
    memcpy(dst->b, src->b, n);
    return dst;
}

/* ============================================================
   CONVERSION
   ============================================================ */

/* Couleur -> Gris  (ITU-R BT.601) */
ImageGris *couleur_vers_gris(const ImageCouleur *src)
{
    int x, y;
    ImageGris *dst;
    if (!src) return NULL;
    dst = allouer_image_gris(src->largeur, src->hauteur);
    if (!dst) return NULL;
    dst->max_val = src->max_val;
    for (y = 0; y < src->hauteur; y++)
        for (x = 0; x < src->largeur; x++) {
            double v = 0.299 * PIXEL_R(src,x,y)
                     + 0.587 * PIXEL_G(src,x,y)
                     + 0.114 * PIXEL_B(src,x,y);
            PIXEL(dst,x,y) = (unsigned char)CLAMP((int)(v+0.5), 0, 255);
        }
    return dst;
}

/* Gris -> Couleur  (R=G=B=gris, utile pour recomposer l'img) */
ImageCouleur *gris_vers_couleur(const ImageGris *src)
{
    int x, y;
    ImageCouleur *dst;
    if (!src) return NULL;
    dst = allouer_image_couleur(src->largeur, src->hauteur);
    if (!dst) return NULL;
    dst->max_val = src->max_val;
    for (y = 0; y < src->hauteur; y++)
        for (x = 0; x < src->largeur; x++) {
            unsigned char v = PIXEL(src,x,y);
            PIXEL_R(dst,x,y) = v;
            PIXEL_G(dst,x,y) = v;
            PIXEL_B(dst,x,y) = v;
        }
    return dst;
}

/* ============================================================
   OPERATIONS SUR CANAUX COULEUR
   ============================================================ */

/* Extrait un canal (0=R, 1=G, 2=B) comme ImageGris */
ImageGris *extraire_canal(const ImageCouleur *src, int canal)
{
    int x, y;
    ImageGris *dst;
    if (!src || canal < 0 || canal > 2) return NULL;
    dst = allouer_image_gris(src->largeur, src->hauteur);
    if (!dst) return NULL;
    for (y = 0; y < src->hauteur; y++)
        for (x = 0; x < src->largeur; x++) {
            switch(canal) {
                case 0: PIXEL(dst,x,y) = PIXEL_R(src,x,y); break;
                case 1: PIXEL(dst,x,y) = PIXEL_G(src,x,y); break;
                case 2: PIXEL(dst,x,y) = PIXEL_B(src,x,y); break;
            }
        }
    return dst;
}

/* Recompose une couleur depuis 3 canaux gris */
ImageCouleur *assembler_canaux(const ImageGris *r,
                                const ImageGris *g,
                                const ImageGris *b)
{
    int x, y;
    ImageCouleur *dst;
    if (!r || !g || !b) return NULL;
    if (r->largeur != g->largeur || r->largeur != b->largeur ||
        r->hauteur != g->hauteur || r->hauteur != b->hauteur) {
        fprintf(stderr, "assembler_canaux : dimensions incompatibles\n");
        return NULL;
    }
    dst = allouer_image_couleur(r->largeur, r->hauteur);
    if (!dst) return NULL;
    for (y = 0; y < r->hauteur; y++)
        for (x = 0; x < r->largeur; x++) {
            PIXEL_R(dst,x,y) = PIXEL(r,x,y);
            PIXEL_G(dst,x,y) = PIXEL(g,x,y);
            PIXEL_B(dst,x,y) = PIXEL(b,x,y);
        }
    return dst;
}

/* Applique une fonction gris->gris sur chaque canal R,G,B */
ImageCouleur *appliquer_sur_canaux(const ImageCouleur *src, FonctionGris f)
{
    ImageGris *cr, *cg, *cb;
    ImageGris *rr, *rg, *rb;
    ImageCouleur *res;
    if (!src || !f) return NULL;
    cr = extraire_canal(src, 0);
    cg = extraire_canal(src, 1);
    cb = extraire_canal(src, 2);
    rr = f(cr); rg = f(cg); rb = f(cb);
    res = assembler_canaux(rr, rg, rb);
    liberer_image_gris(cr); liberer_image_gris(cg); liberer_image_gris(cb);
    liberer_image_gris(rr); liberer_image_gris(rg); liberer_image_gris(rb);
    return res;
}

/* ============================================================
   COMPATIBILITE ENTRE IMAGES
   ============================================================ */

int meme_dimensions(const ImageGris *a, const ImageGris *b)
{
    return (a && b && a->largeur == b->largeur && a->hauteur == b->hauteur);
}

/* Si les dimensions different, la fonction redimensionne src aux dimensions cibles
   (zoom bilineaire) pour rendre les operations possibles */
ImageGris *redimensionner_compatible(const ImageGris *src,
                                      int cible_w, int cible_h)
{
    double fx = (double)cible_w / src->largeur;
    double fy = (double)cible_h / src->hauteur;
    /* zoom bilineaire inline pour eviter dependance circulaire */
    int xd, yd;
    ImageGris *res = allouer_image_gris(cible_w, cible_h);
    if (!res) return NULL;
    for (yd = 0; yd < cible_h; yd++) {
        for (xd = 0; xd < cible_w; xd++) {
            double xs = xd / fx, ys = yd / fy;
            int x0 = (int)xs, y0 = (int)ys;
            int x1 = CLAMP(x0+1, 0, src->largeur-1);
            int y1 = CLAMP(y0+1, 0, src->hauteur-1);
            double tx = xs-x0, ty = ys-y0;
            x0 = CLAMP(x0, 0, src->largeur-1);
            y0 = CLAMP(y0, 0, src->hauteur-1);
            double v = (1-tx)*(1-ty)*PIXEL(src,x0,y0)
                     +    tx*(1-ty)*PIXEL(src,x1,y0)
                     + (1-tx)*ty   *PIXEL(src,x0,y1)
                     +    tx*ty    *PIXEL(src,x1,y1);
            PIXEL(res,xd,yd) = (unsigned char)CLAMP((int)(v+0.5), 0, 255);
        }
    }
    return res;
}

/* ============================================================
   DETECTION DE FORMAT
   ============================================================ */

FormatImage detecter_format(const char *fichier)
{
    FILE *f = fopen(fichier, "rb");
    char magic[3];
    FormatImage fmt = FORMAT_INCONNU;
    if (!f) { perror(fichier); return FORMAT_INCONNU; }
    if (fscanf(f, "%2s", magic) == 1) {
        if      (strcmp(magic, "P5") == 0) fmt = FORMAT_PGM;
        else if (strcmp(magic, "P6") == 0) fmt = FORMAT_PPM;
    }
    fclose(f);
    return fmt;
}

/* ============================================================
   CHARGEMENT UNIVERSEL
   Accepte PGM ou PPM, retourne toujours une ImageGris.
   Si PPM : conversion automatique en niveaux de gris.
   ============================================================ */
ImageGris *charger_image(const char *fichier)
{
    FormatImage fmt = detecter_format(fichier);
    if (fmt == FORMAT_PGM) {
        return lire_pgm(fichier);
    } else if (fmt == FORMAT_PPM) {
        ImageCouleur *c = lire_ppm(fichier);
        ImageGris    *g;
        if (!c) return NULL;
        g = couleur_vers_gris(c);
        liberer_image_couleur(c);
        return g;
    }
    fprintf(stderr, "charger_image : format inconnu pour '%s'\n", fichier);
    return NULL;
}

ImageCouleur *charger_image_couleur(const char *fichier)
{
    FormatImage fmt = detecter_format(fichier);
    if (fmt == FORMAT_PPM)
        return lire_ppm(fichier);
    if (fmt == FORMAT_PGM) {
        /* PGM lu comme couleur : R=G=B=gris */
        ImageGris    *g = lire_pgm(fichier);
        ImageCouleur *c;
        if (!g) return NULL;
        c = gris_vers_couleur(g);
        liberer_image_gris(g);
        return c;
    }
    fprintf(stderr, "charger_image_couleur : format inconnu '%s'\n", fichier);
    return NULL;
}

/* ============================================================
   LECTURE / ECRITURE PGM (P5)
   ============================================================ */

static void sauter_commentaires(FILE *f)
{
    int c;
    while (1) {
        while ((c = fgetc(f)) != EOF &&
               (c==' '||c=='\t'||c=='\n'||c=='\r'));
        if (c == '#') { while ((c=fgetc(f))!=EOF && c!='\n'); }
        else          { if (c!=EOF) ungetc(c,f); break; }
    }
}

ImageGris *lire_pgm(const char *fichier)
{
    FILE *f = fopen(fichier, "rb");
    char magic[3];
    int largeur, hauteur, max_val, n;
    ImageGris *img;
    if (!f) { perror(fichier); return NULL; }
    if (fscanf(f, "%2s", magic) != 1 || strcmp(magic,"P5") != 0) {
        fprintf(stderr, "lire_pgm : pas un fichier P5 (%s)\n", fichier);
        fclose(f); return NULL;
    }
    sauter_commentaires(f);
    if (fscanf(f,"%d",&largeur)!=1) goto err;
    sauter_commentaires(f);
    if (fscanf(f,"%d",&hauteur)!=1) goto err;
    sauter_commentaires(f);
    if (fscanf(f,"%d",&max_val)!=1) goto err;
    fgetc(f);
    img = allouer_image_gris(largeur, hauteur);
    if (!img) { fclose(f); return NULL; }
    img->max_val = max_val;
    n = fread(img->pixels, 1, largeur*hauteur, f);
    fclose(f);
    if (n != largeur*hauteur) {
        fprintf(stderr, "lire_pgm : lecture incomplete %d/%d\n",
                n, largeur*hauteur);
        liberer_image_gris(img); return NULL;
    }
    return img;
err:
    fprintf(stderr, "lire_pgm : erreur en-tete\n");
    fclose(f); return NULL;
}

int ecrire_pgm(const ImageGris *img, const char *fichier)
{
    FILE *f;
    if (!img) return -1;
    f = fopen(fichier, "wb");
    if (!f) { perror(fichier); return -1; }
    fprintf(f, "P5\n%d %d\n%d\n", img->largeur, img->hauteur, img->max_val);
    fwrite(img->pixels, 1, img->largeur*img->hauteur, f);
    fclose(f); return 0;
}

/* ============================================================
   LECTURE / ECRITURE PPM (P6)
   ============================================================ */

ImageCouleur *lire_ppm(const char *fichier)
{
    FILE *f = fopen(fichier, "rb");
    char magic[3];
    int largeur, hauteur, max_val, x, y;
    ImageCouleur *img;
    unsigned char buf[3];
    if (!f) { perror(fichier); return NULL; }
    if (fscanf(f, "%2s", magic) != 1 || strcmp(magic,"P6") != 0) {
        fprintf(stderr, "lire_ppm : pas un fichier P6 (%s)\n", fichier);
        fclose(f); return NULL;
    }
    sauter_commentaires(f);
    if (fscanf(f,"%d",&largeur)!=1) goto err;
    sauter_commentaires(f);
    if (fscanf(f,"%d",&hauteur)!=1) goto err;
    sauter_commentaires(f);
    if (fscanf(f,"%d",&max_val)!=1) goto err;
    fgetc(f);
    img = allouer_image_couleur(largeur, hauteur);
    if (!img) { fclose(f); return NULL; }
    img->max_val = max_val;
    for (y = 0; y < hauteur; y++)
        for (x = 0; x < largeur; x++) {
            if (fread(buf, 1, 3, f) != 3) {
                fprintf(stderr, "lire_ppm : lecture incomplete (%d,%d)\n",x,y);
                liberer_image_couleur(img); fclose(f); return NULL;
            }
            PIXEL_R(img,x,y)=buf[0];
            PIXEL_G(img,x,y)=buf[1];
            PIXEL_B(img,x,y)=buf[2];
        }
    fclose(f); return img;
err:
    fprintf(stderr, "lire_ppm : erreur en-tete\n");
    fclose(f); return NULL;
}

int ecrire_ppm(const ImageCouleur *img, const char *fichier)
{
    FILE *f;
    int x, y;
    unsigned char buf[3];
    if (!img) return -1;
    f = fopen(fichier, "wb");
    if (!f) { perror(fichier); return -1; }
    fprintf(f, "P6\n%d %d\n%d\n", img->largeur, img->hauteur, img->max_val);
    for (y = 0; y < img->hauteur; y++)
        for (x = 0; x < img->largeur; x++) {
            buf[0]=PIXEL_R(img,x,y);
            buf[1]=PIXEL_G(img,x,y);
            buf[2]=PIXEL_B(img,x,y);
            fwrite(buf, 1, 3, f);
        }
    fclose(f); return 0;
}
