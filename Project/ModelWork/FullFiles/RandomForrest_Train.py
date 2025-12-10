import pandas as pd
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, confusion_matrix
import joblib
import os

input_path = r"c:/TML_Particle_EH5/Project/Project/ModelWork/features_dataset.csv"
model_path = r"c:/TML_Particle_EH5/Project/Project/ModelWork/rf_model.pkl"

df = pd.read_csv(input_path)
X = df.drop(columns=["filename", "label"])
y = df["label"]

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42, stratify=y)

# Train Random Forest Classifier
clf = RandomForestClassifier(n_estimators=500, class_weight='balanced',max_depth=None ,random_state=42)
clf.fit(X_train, y_train)

# Print out model details (Tree count and max depth)
print(f"Trained Random Forest with {clf.n_estimators} trees and depth {clf.max_depth}")

# Make predictions and evaluate
y_pred = clf.predict(X_test)
report = classification_report(y_test, y_pred, output_dict=False)
matrix = confusion_matrix(y_test, y_pred)

# Print evaluation results
print("Accuracy:", clf.score(X_test, y_test))
print("\nClassification report:\n", report)
print("Confusion matrix:\n", matrix)

# Save the trained model
os.makedirs(os.path.dirname(model_path), exist_ok=True)
joblib.dump(clf, model_path)
print(f"\nGemte model som {model_path}")

