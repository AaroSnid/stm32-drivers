import sys
import os
import shutil
from git import Repo

# Supported drivers
drivers = ("ICM-42688-P", "SN74HC595")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: py driver_update.py <project_directory> <driver_1> <driver_2>...")
        sys.exit(1)

    project_path = os.path.abspath(sys.argv[1])
    drivers_path = os.path.join(project_path, "Drivers")
    core_path = os.path.join(project_path, "Core")
    src_path = os.path.join(core_path, "Src")
    inc_path = os.path.join(core_path, "Inc")

    # Parse requested new drivers
    new_drivers = []
    for arg in sys.argv[2:]:
        arg = arg.upper()
        if arg in drivers:
            new_drivers.append(arg)

    if not new_drivers:
        print("Warning: No valid drivers requested")
        sys.exit(2)
    else:
        print(f"Processing {len(new_drivers)} driver(s): {', '.join(new_drivers)}")

    # Clone driver repository
    driver_repo_path = os.path.join(project_path, "drivers_repo")
    driver_url = "https://github.com/AaroSnid/stm32-drivers.git"

    if os.path.exists(driver_repo_path):
        shutil.rmtree(driver_repo_path)

    print("Cloning driver repository...")
    Repo.clone_from(driver_url, driver_repo_path)

    # repo_drivers_path = os.path.join(driver_repo_path, "Drivers")
    repo_drivers_path = driver_repo_path

    cmake_src = []
    cmake_inc = []

    # Add requested drivers
    for driver in new_drivers:
        print(f"Processing driver: {driver}")

        repo_driver_path = os.path.join(driver_repo_path, driver)
        target_driver_dir = os.path.join(drivers_path, driver)

        if not os.path.exists(repo_driver_path):
            print(f"  Driver {driver} not found in repository")
            continue

        # Folder-based driver
        if os.path.isdir(repo_driver_path):
            if os.path.exists(target_driver_dir):
                print(f"  Updating existing folder driver {driver}")
                shutil.rmtree(target_driver_dir)
            else:
                print(f"  Adding new folder driver {driver}")

            shutil.copytree(repo_driver_path, target_driver_dir)
            for root, _, files in os.walk(target_driver_dir):
                for f in files:
                    if f.endswith(".c") or f.endswith(".cpp"):
                        cmake_src.append(os.path.join(root, f))

            cmake_inc.append(target_driver_dir)

        # Flat C/H driver
        else:
            print(f"  Updating flat driver files for {driver}")

            repo_files = [
                f for f in os.listdir(repo_driver_path)
                if f.endswith((".c", ".h"))
            ]

            # Remove old driver files only
            for fname in repo_files:
                if fname.endswith(".c"):
                    target = os.path.join(src_path, fname)
                else:
                    target = os.path.join(inc_path, fname)

                if os.path.exists(target):
                    print(f"    Removing {target}")
                    os.remove(target)

            # Copy new files
            for fname in repo_files:
                src = os.path.join(repo_driver_path, fname)

                if fname.endswith(".c"):
                    dst = os.path.join(src_path, fname)
                    shutil.copy2(src, dst)
                    cmake_src.append(os.path.join(src_path, fname))

                else:
                    dst = os.path.join(inc_path, fname)
                    shutil.copy2(src, dst)
                    cmake_inc.append(inc_path)


    # Cleanup
    print("Removing temporary driver repository")
    try:
        shutil.rmtree(driver_repo_path)
    except:
        print("Unable to remove drivers_repo folder, please remove manually") 

    # Output CMake hints
    print("\nIf using CMake, make sure these paths are included:\n")

    if cmake_inc:
        print("Include directories:")
        for d in sorted(set(cmake_inc)):
            print(f"  {d}")

    if cmake_src:
        print("\nSource directories:")
        for d in sorted(set(cmake_src)):
            print(f"  {d}")