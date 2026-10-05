import numpy as np
import pandas as pd
from pathlib import Path
import itertools


class StandardScalerScratch:
    """Standardisation simple: (X - moyenne) / ecart-type."""

    def __init__(self, epsilon=1e-8):
        self.epsilon = epsilon
        self.mean_ = None
        self.std_ = None

    def fit(self, X):
        self.mean_ = np.mean(X, axis=0)
        self.std_ = np.std(X, axis=0)
        self.std_[self.std_ < self.epsilon] = 1.0  # Eviter les divisions par 0
        return self

    def transform(self, X):
        if self.mean_ is None or self.std_ is None:
            raise RuntimeError("Le scaler doit etre entraine avec fit() avant transform().")
        return (X - self.mean_) / self.std_

    def fit_transform(self, X):
        return self.fit(X).transform(X)


class NoyauRBF:
    def __init__(self, gamma=0.01):
        self.gamma = gamma

    def calculer(self, x1, x2):
        distance_carre = np.linalg.norm(x1 - x2) ** 2
        return np.exp(-self.gamma * distance_carre)

    def matrice(self, X1, X2=None):
        """
        Calcule une matrice de noyau RBF vectorisee.
        K[i, j] = exp(-gamma * ||X1[i] - X2[j]||^2)
        """
        if X2 is None:
            X2 = X1

        X1_norm = np.sum(X1 ** 2, axis=1).reshape(-1, 1)
        X2_norm = np.sum(X2 ** 2, axis=1).reshape(1, -1)
        distances_carrees = X1_norm + X2_norm - 2.0 * (X1 @ X2.T)
        distances_carrees = np.maximum(distances_carrees, 0.0)
        return np.exp(-self.gamma * distances_carrees)


class SVM_SMO_Optimise:
    def __init__(
            self,
            noyau,
            C=1.0,
            tolerance=1e-3,
            max_passes=5,
            support_threshold=1e-5,
            random_state=42,
    ):
        self.noyau = noyau
        self.C = C
        self.tolerance = tolerance
        self.max_passes = max_passes
        self.support_threshold = support_threshold
        self.rng = np.random.default_rng(random_state)

        self.lambdas = None
        self.b = 0.0
        self.X_train = None
        self.y_train = None
        self.K = None
        self.errors = None
        self.support_indices_ = None

    def _decision_interne(self):
        return self.K @ (self.lambdas * self.y_train) + self.b

    def _mettre_a_jour_cache_erreurs(self):
        self.errors = self._decision_interne() - self.y_train

    def _choisir_j(self, i, E_i):
        non_bornes = np.where(
            (self.lambdas > self.support_threshold)
            & (self.lambdas < self.C - self.support_threshold)
        )[0]
        candidats = non_bornes[non_bornes != i]

        if candidats.size == 0:
            candidats = np.arange(self.y_train.shape[0])
            candidats = candidats[candidats != i]

        if candidats.size == 0:
            raise ValueError("SMO necessite au moins deux echantillons.")

        ecarts = np.abs(E_i - self.errors[candidats])
        meilleurs = candidats[ecarts == np.max(ecarts)]
        return int(self.rng.choice(meilleurs))

    def _objectif_pair(self, i, j, lambda_i, lambda_j):
        y_i = self.y_train[i]
        y_j = self.y_train[j]
        return (
                lambda_i
                + lambda_j
                - 0.5 * self.K[i, i] * lambda_i ** 2
                - 0.5 * self.K[j, j] * lambda_j ** 2
                - y_i * y_j * self.K[i, j] * lambda_i * lambda_j
        )

    def fit(self, X, y):
        self.X_train = np.asarray(X, dtype=np.float64)
        self.y_train = np.asarray(y, dtype=np.float64)
        n_samples = self.X_train.shape[0]

        if n_samples < 2:
            raise ValueError("Il faut au moins deux echantillons pour entrainer le SVM.")

        self.K = self.noyau.matrice(self.X_train)
        self.lambdas = np.zeros(n_samples, dtype=np.float64)
        self.b = 0.0
        self.errors = -self.y_train.copy()

        passes = 0
        iteration = 0

        while passes < self.max_passes:
            num_changed_lambdas = 0
            ordre = self.rng.permutation(n_samples)

            for i in ordre:
                E_i = self.errors[i]
                y_i = self.y_train[i]
                lambda_i_old = self.lambdas[i]

                viole_kkt = (
                                    y_i * E_i < -self.tolerance and lambda_i_old < self.C
                            ) or (
                                    y_i * E_i > self.tolerance and lambda_i_old > 0
                            )

                if not viole_kkt:
                    continue

                j = self._choisir_j(i, E_i)
                E_j = self.errors[j]
                y_j = self.y_train[j]
                lambda_j_old = self.lambdas[j]

                if y_i != y_j:
                    L = max(0.0, lambda_j_old - lambda_i_old)
                    H = min(self.C, self.C + lambda_j_old - lambda_i_old)
                else:
                    L = max(0.0, lambda_i_old + lambda_j_old - self.C)
                    H = min(self.C, lambda_i_old + lambda_j_old)

                if np.isclose(L, H):
                    continue

                k_ii = self.K[i, i]
                k_jj = self.K[j, j]
                k_ij = self.K[i, j]
                eta = 2.0 * k_ij - k_ii - k_jj

                if eta < 0:
                    lambda_j_new = lambda_j_old - y_j * (E_i - E_j) / eta
                    lambda_j_new = np.clip(lambda_j_new, L, H)
                else:
                    lambda_i_L = lambda_i_old + y_i * y_j * (lambda_j_old - L)
                    lambda_i_H = lambda_i_old + y_i * y_j * (lambda_j_old - H)
                    objectif_L = self._objectif_pair(i, j, lambda_i_L, L)
                    objectif_H = self._objectif_pair(i, j, lambda_i_H, H)

                    if objectif_L > objectif_H + self.tolerance:
                        lambda_j_new = L
                    elif objectif_H > objectif_L + self.tolerance:
                        lambda_j_new = H
                    else:
                        continue

                if abs(lambda_j_new - lambda_j_old) < self.support_threshold:
                    continue

                lambda_i_new = lambda_i_old + y_i * y_j * (lambda_j_old - lambda_j_new)
                lambda_i_new = np.clip(lambda_i_new, 0.0, self.C)

                b_old = self.b
                b1 = self.b - E_i - y_i * (lambda_i_new - lambda_i_old) * k_ii - y_j * (
                            lambda_j_new - lambda_j_old) * k_ij
                b2 = self.b - E_j - y_i * (lambda_i_new - lambda_i_old) * k_ij - y_j * (
                            lambda_j_new - lambda_j_old) * k_jj

                if self.support_threshold < lambda_i_new < self.C - self.support_threshold:
                    self.b = b1
                elif self.support_threshold < lambda_j_new < self.C - self.support_threshold:
                    self.b = b2
                else:
                    self.b = (b1 + b2) / 2.0

                self.lambdas[i] = lambda_i_new
                self.lambdas[j] = lambda_j_new

                delta_i = lambda_i_new - lambda_i_old
                delta_j = lambda_j_new - lambda_j_old
                delta_b = self.b - b_old
                self.errors += (
                        y_i * delta_i * self.K[:, i]
                        + y_j * delta_j * self.K[:, j]
                        + delta_b
                )

                num_changed_lambdas += 1

            iteration += 1
            if num_changed_lambdas == 0:
                passes += 1
            else:
                passes = 0

        self.support_indices_ = np.where(self.lambdas > self.support_threshold)[0]
        return self

    def decision_function(self, X):
        if self.support_indices_ is None:
            raise RuntimeError("Le modele doit etre entraine avant la prediction.")

        X = np.asarray(X, dtype=np.float64)
        sv = self.support_indices_
        K_test = self.noyau.matrice(X, self.X_train[sv])
        coefficients = self.lambdas[sv] * self.y_train[sv]
        return K_test @ coefficients + self.b

    def predict(self, X):
        scores = self.decision_function(X)
        return np.where(scores >= 0.0, 1.0, -1.0)


# classe SVM un-contre-ub
class SVM_MultiClass_OvO:
    def __init__(self, C=1.0, gamma=0.001, tolerance=1e-3, max_passes=3):
        self.C = C
        self.gamma = gamma
        self.tolerance = tolerance
        self.max_passes = max_passes
        self.modeles = {}
        self.classes_ = None

    def fit(self, X, y):
        self.classes_ = np.unique(y)
        paires = list(itertools.combinations(self.classes_, 2))
        total_modeles = len(paires)

        print(f"Demarrage de l'entrainement OvO: {total_modeles} modeles a entrainer.")

        for idx, (c1, c2) in enumerate(paires):
            print(f"  -> Entrainement {idx + 1}/{total_modeles} : Classe {c1} vs Classe {c2}", end="\r")

            # 1. Isoler les données pour la paire actuelle
            masque = (y == c1) | (y == c2)
            X_pair = X[masque]
            y_pair = y[masque]

            # 2. Binariser les labels: classe c1 devient +1, classe c2 devient -1
            y_binaire = np.where(y_pair == c1, 1.0, -1.0)

            # 3. Entraîner le modèle SVM binaire sur cette paire
            noyau = NoyauRBF(gamma=self.gamma)
            modele = SVM_SMO_Optimise(
                noyau=noyau,
                C=self.C,
                tolerance=self.tolerance,
                max_passes=self.max_passes
            )
            modele.fit(X_pair, y_binaire)

            # 4. Sauvegarder le modèle entraîné
            self.modeles[(c1, c2)] = modele

        print("\nEntrainement de tous les modeles termine !")

    def predict(self, X):
        n_samples = X.shape[0]
        votes = np.zeros((n_samples, len(self.classes_)))
        class_to_idx = {c: i for i, c in enumerate(self.classes_)}

        # Faire voter chaque modèle
        for (c1, c2), modele in self.modeles.items():
            predictions_binaires = modele.predict(X)

            for i in range(n_samples):
                if predictions_binaires[i] == 1.0:
                    votes[i, class_to_idx[c1]] += 1
                else:
                    votes[i, class_to_idx[c2]] += 1

        # La prédiction finale est la classe qui a récolté le plus de votes
        indices_gagnants = np.argmax(votes, axis=1)
        return self.classes_[indices_gagnants]


def charger_csv_mnist(nom_fichier):
    dossier_script = Path(__file__).resolve().parent
    candidats = [
        dossier_script / ".." / "dataset" / nom_fichier,
        dossier_script / ".." / nom_fichier,
        Path(nom_fichier),
    ]
    for chemin in candidats:
        chemin = chemin.resolve()
        if chemin.exists():
            return pd.read_csv(chemin)
    chemins = ", ".join(str(c.resolve()) for c in candidats)
    raise FileNotFoundError(f"Fichier introuvable. Chemins testes: {chemins}")


def preparer_donnees_multiclasse(X_train, X_test):
    # Standardisation globale sur toutes les classes
    scaler = StandardScalerScratch()
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)
    return X_train_scaled, X_test_scaled, scaler


if __name__ == "__main__":
    print("=== SVM Multi-Classes (OvO) From Scratch: MNIST ===")

    try:
        print("Chargement des donnees...")
        df_train = charger_csv_mnist("dataset/mnist_train_32.csv")
        df_test = charger_csv_mnist("dataset/mnist_test_32.csv")

        y_train_brut = df_train["label"].values
        X_train_brut = df_train.drop("label", axis=1).values
        y_test_brut = df_test["label"].values
        X_test_brut = df_test.drop("label", axis=1).values

        print("Standardisation des donnees...")
        X_train_scaled, X_test_scaled, _ = preparer_donnees_multiclasse(
            X_train_brut, X_test_brut
        )

        # /!\ ATTENTION: Pour un test "from scratch", on sous-échantillonne.
        # Faire tourner SMO pur Python sur 60 000 images prendra des jours.
        N_SAMPLES_TRAIN = 3000
        N_SAMPLES_TEST = 1000

        X_train_sub = X_train_scaled[:N_SAMPLES_TRAIN]
        y_train_sub = y_train_brut[:N_SAMPLES_TRAIN]
        X_test_sub = X_test_scaled[:N_SAMPLES_TEST]
        y_test_sub = y_test_brut[:N_SAMPLES_TEST]

        print(f"Entrainement sur un sous-ensemble de {N_SAMPLES_TRAIN} images...")

        # Initialisation du modèle multi-classes
        modele_multiclasse = SVM_MultiClass_OvO(
            C=1.0,
            gamma=0.001,
            max_passes=3  # Réduit pour accélérer l'entraînement
        )

        # Entraînement (45 modèles vont être générés en interne)
        modele_multiclasse.fit(X_train_sub, y_train_sub)

        print(f"Prediction sur {N_SAMPLES_TEST} images test...")
        predictions = modele_multiclasse.predict(X_test_sub)

        precision = np.mean(predictions == y_test_sub) * 100.0
        print(f">>> Precision test globale (10 classes): {precision:.2f}% <<<")

    except FileNotFoundError as e:
        print(f"Erreur: {e}")