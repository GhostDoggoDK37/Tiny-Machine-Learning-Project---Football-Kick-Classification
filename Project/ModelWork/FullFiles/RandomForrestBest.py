# randomforest_train.py

import pandas as pd
import numpy as np
import os
import joblib
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import (
    classification_report,
    confusion_matrix,
    accuracy_score,
    f1_score,
)
import matplotlib.pyplot as plt
import seaborn as sns

input_path = r"c:/TML_Particle_EH5/Project/Project/ModelWork/features_dataset.csv"
output_dir = r"c:/TML_Particle_EH5/Project/Project/ModelWork/"
model_path = os.path.join(output_dir, "rf_best_model.pkl")
report_path = os.path.join(output_dir, "rf_test_report.txt")
matrix_path = os.path.join(output_dir, "rf_confusion_matrix.png")

df = pd.read_csv(input_path)

X = df.drop(columns=["filename", "label"])
y = df["label"]

X = X.replace([np.inf, -np.inf], np.nan)
X = X.fillna(0)

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.3, random_state=42, stratify=y
)

clf = RandomForestClassifier(
    n_estimators=300,
    max_depth=None,
    class_weight='balanced',
    random_state=42
)

clf.fit(X_train, y_train)

y_pred = clf.predict(X_test)

acc = accuracy_score(y_test, y_pred)
f1 = f1_score(y_test, y_pred, average='weighted')
report = classification_report(y_test, y_pred)
matrix = confusion_matrix(y_test, y_pred, labels=sorted(y.unique()))

print("\n--- Model performance på testdata ---")
print(f"Accuracy: {acc:.3f}")
print(f"F1-score (weighted): {f1:.3f}")
print("\nClassification report:\n", report)
print("Confusion matrix:\n", matrix)

os.makedirs(output_dir, exist_ok=True)
joblib.dump(clf, model_path)

with open(report_path, "w") as f:
    f.write(f"Accuracy: {acc:.3f}\n")
    f.write(f"F1-score (weighted): {f1:.3f}\n\n")
    f.write(report)

plt.figure(figsize=(6,5))
sns.heatmap(matrix, annot=True, fmt='d', cmap='Blues',
            xticklabels=sorted(y.unique()),
            yticklabels=sorted(y.unique()))
plt.title("Confusion Matrix (Test Data)")
plt.xlabel("Predicted")
plt.ylabel("True")
plt.tight_layout()
plt.savefig(matrix_path)
plt.close()

print(f"\nGemte model: {model_path}")
print(f"Gemte test report: {report_path}")
print(f"Gemte confusion matrix: {matrix_path}")
