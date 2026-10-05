import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
import time

# ====================== CHARGEMENT ======================
train = pd.read_csv('mnist_train_32.csv')
test = pd.read_csv('mnist_test_32.csv')

X_train = train.iloc[:, 1:].values.astype(np.float32) / 255.0
y_train = train.iloc[:, 0].values.astype(np.int32)
X_test = test.iloc[:, 1:].values.astype(np.float32) / 255.0
y_test = test.iloc[:, 0].values.astype(np.int32)

# ====================== ACTIVATIONS ======================
def sigmoid(z):
    return 1 / (1 + np.exp(-np.clip(z, -500, 500)))

def sigmoid_deriv(a):
    return a * (1 - a)

def softmax(z):
    exp = np.exp(z - np.max(z, axis=1, keepdims=True))
    return exp / np.sum(exp, axis=1, keepdims=True)

# ====================== PARAMÈTRES ======================
input_size = 1024
hidden_size = 64
output_size = 10
lr = 0.12
epochs = 50
batch_size = 1


np.random.seed(42)
W1 = np.random.randn(input_size, hidden_size) * np.sqrt(1.0 / input_size)
b1 = np.zeros((1, hidden_size))
W2 = np.random.randn(hidden_size, output_size) * np.sqrt(1.0 / hidden_size)
b2 = np.zeros((1, output_size))

weight_history = []
loss_history = []

print("=== Phase 1 : Entraînement ===\n")

for epoch in range(epochs):
    indices = np.random.permutation(len(X_train))
    X_shuf = X_train[indices]
    y_shuf = y_train[indices]
    epoch_loss = 0
    
    for i in range(0, len(X_train), batch_size):
        Xb = X_shuf[i:i+batch_size]
        yb = y_shuf[i:i+batch_size]
        m = Xb.shape[0]
        
        z1 = Xb @ W1 + b1
        a1 = sigmoid(z1)
        z2 = a1 @ W2 + b2
        a2 = softmax(z2)
        
        y_onehot = np.zeros((m, 10))
        y_onehot[np.arange(m), yb] = 1
        
        loss = -np.sum(y_onehot * np.log(a2 + 1e-8)) / m
        epoch_loss += loss
        
        dz2 = (a2 - y_onehot) / m
        dW2 = a1.T @ dz2
        db2 = np.sum(dz2, axis=0, keepdims=True)
        
        dz1 = (dz2 @ W2.T) * sigmoid_deriv(a1)
        dW1 = Xb.T @ dz1
        db1 = np.sum(dz1, axis=0, keepdims=True)
        
        W1 -= lr * dW1
        b1 -= lr * db1
        W2 -= lr * dW2
        b2 -= lr * db2
    
    weight_history.append(W1.copy())
    loss_history.append(epoch_loss / len(X_train) * batch_size)
    
    if (epoch + 1) % 5 == 0:
        print(f"Epoch {epoch+1}/{epochs} | Loss: {loss_history[-1]:.4f}")

print("\nEntraînement terminé !")

input("\nAppuie sur Entrée pour passer à la phase d'analyse et de test...")

# ====================== HEATMAPS ÉVOLUTION (une par une) ======================
print("\nAffichage des heatmaps d'évolution des poids W1 (une par une)...")
for i in range(0, len(weight_history), 5):
    plt.figure(figsize=(12, 7))
    sns.heatmap(weight_history[i][:300, :], cmap='RdBu_r', center=0)
    plt.title(f"Évolution des poids W1 - Epoch {i}")
    plt.xlabel("64 Neurones cachés")
    plt.ylabel("Pixels d'entrée (300 premiers)")
    plt.tight_layout()
    plt.show(block=False)
    
    print(f"Epoch {i} affichée. Appuie sur Entrée pour voir la suivante...")
    input()

# ====================== ANALYSE DES POIDS ======================
print("\n=== Analyse des poids les plus influents ===")
contribution = np.abs(W2.T)  # (10, 64)

for d in range(10):
    top5_idx = np.argsort(contribution[d])[-5:][::-1]
    top5_weights = W2[top5_idx, d]   # poids réels (positif ou négatif)
    print(f"\nChiffre {d} :")
    for idx, w in zip(top5_idx, top5_weights):
        sign = "POSITIF" if w > 0 else "NÉGATIF"
        print(f"   Neurone {idx:2d} → Poids = {w:.4f} ({sign})")

# ====================== TEST ET MÉTRIQUES ======================
def predict(X):
    z1 = X @ W1 + b1
    a1 = sigmoid(z1)
    z2 = a1 @ W2 + b2
    a2 = softmax(z2)
    return np.argmax(a2, axis=1)

print("\nPrédiction sur le set de test...")
y_pred = predict(X_test)
accuracy = accuracy_score(y_test, y_pred)

print(f"\n=== RÉSULTATS FINAUX ===")
print(f"Accuracy : {accuracy:.4f} ({accuracy*100:.2f}%)")

print("\nMatrice de Confusion :")
cm = confusion_matrix(y_test, y_pred)
plt.figure(figsize=(8, 6))
sns.heatmap(cm, annot=True, fmt='d', cmap='Blues')
plt.title('Matrice de Confusion')
plt.xlabel('Prédiction')
plt.ylabel('Vraie valeur')
plt.show()

print("\nRapport détaillé par classe :")
print(classification_report(y_test, y_pred, digits=4))

# Courbe de convergence
plt.figure(figsize=(10, 5))
plt.plot(loss_history)
plt.title("Évolution de la Loss pendant l'entraînement")
plt.xlabel("Epoch")
plt.ylabel("Loss")
plt.grid(True)
plt.show()

# ====================== SAUVEGARDE DU MODÈLE ======================
np.savez('modele_mlp.npz', 
         W1=W1, 
         b1=b1, 
         W2=W2, 
         b2=b2)

print("Modèle sauvegardé sous le nom 'modele_mlp.npz'")