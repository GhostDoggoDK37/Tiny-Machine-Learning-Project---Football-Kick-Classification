import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.ensemble import RandomForestClassifier
import os

input_path = r"c:/TML_Particle_EH5/Project/Project/ModelWork/features_dataset.csv"
output_dir = r"c:/TML_Particle_EH5/Project/Project/ModelWork/"

df = pd.read_csv(input_path)
X = df.drop(columns=["filename", "label"])
y = df["label"]

clf = RandomForestClassifier(n_estimators=300, class_weight='balanced', random_state=42)
clf.fit(X, y)

importances = pd.Series(clf.feature_importances_, index=X.columns).sort_values(ascending=False)
importances.to_csv(os.path.join(output_dir, "feature_importance.csv"), header=["importance"])

print("\nTop 15 vigtigste features:\n")
print(importances.head(15))

plt.figure(figsize=(10,5))
sns.barplot(x=importances.head(15), y=importances.head(15).index)
plt.title("Feature importance (Top 15)")
plt.tight_layout()
plt.savefig(os.path.join(output_dir, "feature_importance.png"))
plt.close()

corr = X.corr()
plt.figure(figsize=(12,10))
sns.heatmap(corr, cmap='coolwarm', center=0)
plt.title("Feature correlation")
plt.tight_layout()
plt.savefig(os.path.join(output_dir, "feature_correlation.png"))
plt.close()

print(f"\nGemte feature_importance.csv, feature_importance.png og feature_correlation.png i {output_dir}")
