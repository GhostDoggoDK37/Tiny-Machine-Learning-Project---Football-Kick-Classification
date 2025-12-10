import pandas as pd
import numpy as np
import glob, os
from scipy.stats import kurtosis, skew

data_base = r"c:/TML_Particle_EH5/Project/Project/DataValidation/"
output_path = r"c:/TML_Particle_EH5/Project/Project/ModelWork/features_dataset.csv"

def extract_features(filepath, label):
    df = pd.read_csv(filepath)
    f = {"filename": os.path.basename(filepath), "label": label}

    for col in ['ax', 'ay', 'az']:
        x = df[col]
        f[f"{col}_mean"] = np.mean(x)
        f[f"{col}_std"] = np.std(x)
        f[f"{col}_var"] = np.var(x)
        f[f"{col}_range"] = np.max(x) - np.min(x)
        f[f"{col}_rms"] = np.sqrt(np.mean(x**2))
        f[f"{col}_skew"] = skew(x)
        f[f"{col}_kurtosis"] = kurtosis(x, fisher=False)

    x = df["a_total"]
    f["a_total_energy"] = np.sum(x**2) / len(x)
    f["a_total_max"] = np.max(x)
    f["a_total_min"] = np.min(x)
    f["a_total_median"] = np.median(x)
    # f["a_total_iqr"] = np.percentile(x, 75) - np.percentile(x, 25)
    f["a_total_absmean"] = np.mean(np.abs(x))
    # f["a_total_zero_crossings"] = ((x[:-1] * x[1:]) < 0).sum()
    return f

def load_all_features():
    dataframes = []
    folders = [
        ("Clipped_shots", "shot"),
        ("Clipped_crosses", "cross"),
        ("Clipped_passes", "pass"),     # Korrekt mappenavn
        ("Clipped_noise", "noise")
    ]

    for folder, label in folders:
        path = os.path.join(data_base, folder)
        for f in glob.glob(os.path.join(path, "*.csv")):
            dataframes.append(extract_features(f, label))

    return pd.DataFrame(dataframes)

if __name__ == "__main__":
    df = load_all_features()
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    df.to_csv(output_path, index=False)
    print(f"Gemte {len(df)} samples og {df.shape[1]-2} features i {output_path}")
