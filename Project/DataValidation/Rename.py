import os

folder = r"c:\TML_Particle_EH5\Project\Project\DataValidation\Clipped_passes"  # Skift til din mappe

files = os.listdir(folder)

counter = 1
for f in files:
    old_path = os.path.join(folder, f)
    if not os.path.isfile(old_path):
        continue

    ext = os.path.splitext(f)[1]      # Beholder filendelsen
    new_name = f"Clipped_pass{counter}{ext}" # Nyt navn
    new_path = os.path.join(folder, new_name)

    os.rename(old_path, new_path)
    counter += 1
    print(f"Renamed {f} to {new_name}")