"""Print random uppercase UUIDs for reflected type declarations."""

import argparse
import uuid


def positive_count(value: str) -> int:
    count = int(value)
    if count <= 0:
        raise argparse.ArgumentTypeError("count must be positive")
    return count


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("count", nargs="?", type=positive_count, default=1)
    args = parser.parse_args()
    for _ in range(args.count):
        print(str(uuid.uuid4()).upper())


if __name__ == "__main__":
    main()
