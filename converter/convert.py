import sys
import os
import shutil
from subprocess import run

def main():
    print("\n=== 7z2zip and zip27z converter ===\n")

    mode = input("Enter mode (27z or 2zip): ")

    if not(mode.lower() == "27z" or mode.lower() == "2zip"):
        print("Error: Incorrect mode -", mode)
        sys.exit(1)

    filename = input("\nEnter archive path: ")

    if not os.path.isfile(filename):
        print("Error: No such file -", filename)
        sys.exit(1)
    
    try:
        f, ext = os.path.splitext(filename)      # "filename", ".ext"

        if (mode.lower() == "27z"):
            run(["unzip", "-q", filename])       # extract zip
            run(["7za", "a", f+".7z", f])        # create 7z
        else:
            run(["7za", "x", filename])          # extract 7z
            run(["zip", "-r", f+".zip", f])      # create zip

        shutil.rmtree(f)                         # remove temp directory
    except Exception as e:
        print("Error:", e)
        sys.exit(1)


if __name__ == "__main__":
    main()
