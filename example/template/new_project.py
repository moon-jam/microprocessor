import sys
from pathlib import Path


TEMPLATE = Path(__file__).resolve().parent
ROOT = TEMPLATE.parents[1]
FILES = ("main.c", "makefile")


def ask(message):
    print(message)
    return input(": ")


def create_project(destination, pending, files):
    created_dirs = []
    created_files = []
    try:
        for directory in pending:
            try:
                directory.mkdir()
            except FileExistsError:
                if not directory.is_dir():
                    raise
            else:
                created_dirs.append(directory)
        for name, content in files.items():
            path = destination / name
            with path.open("x", encoding="utf-8") as output:
                created_files.append(path)
                output.write(content)
    except (OSError, KeyboardInterrupt):
        for path in created_files:
            path.unlink(missing_ok=True)
        for directory in reversed(created_dirs):
            try:
                directory.rmdir()
            except OSError:
                pass
        raise


def main():
    entered = ask("Project directory (inside this repository)").strip()
    if not entered:
        print("Cancelled.")
        return

    destination = (ROOT / Path(entered).expanduser()).resolve()
    if destination == ROOT or not destination.is_relative_to(ROOT):
        raise ValueError("Choose a subdirectory inside this repository.")
    relative = destination.relative_to(ROOT)

    for name in (*FILES, "Makefile", "GNUmakefile"):
        path = destination / name
        if path.exists() or path.is_symlink():
            raise FileExistsError(f"{path.relative_to(ROOT)} already exists.")

    pending = []
    directory = ROOT
    for part in relative.parts:
        directory /= part
        if directory.is_dir():
            continue
        if directory.exists():
            raise NotADirectoryError(f"{directory.relative_to(ROOT)} is not a directory.")
        answer = ask(f"{directory.relative_to(ROOT)} does not exist. Create it? [y/N]")
        if answer.strip().lower() not in ("y", "yes"):
            print("Cancelled.")
            return
        pending.append(directory)

    lib = "../" * len(relative.parts) + "lib/"
    files = {
        name: (TEMPLATE / name).read_text(encoding="utf-8").replace("../../lib/", lib)
        for name in FILES
    }
    create_project(destination, pending, files)
    print(f"Created {relative / 'main.c'} and {relative / 'makefile'}.")


if __name__ == "__main__":
    try:
        main()
    except (EOFError, KeyboardInterrupt):
        print("\nCancelled.")
    except (OSError, RuntimeError, ValueError) as error:
        sys.exit(str(error))
