"""
create_instance.py

Generates multiple instance .dat files in an 'instances' folder in the current directory.

Changes implemented:
- Ensures b + c > a, b > c, b > e, d > b
- Adds a new set H with nH elements
- q: size nR * nH
- Q: size nN * nH
- Generates multiple instances with filenames: random_i_nN_nR_nH.dat
"""

from pathlib import Path
import numpy as np


def _format_list(lst, fmt=None):
    """Format a list (or nested list) as [a, b, c]."""
    if isinstance(lst, (list, np.ndarray)):
        return "[" + ", ".join(_format_list(x) for x in lst) + "]"
    else:
        return str(lst)


def create_instances(nN: int, nR: int, nH: int, total: int, seed: int = None):
    """
    Generate multiple instance files with nN, nR, nH elements.

    Parameters
    ----------
    nN : int
        Number of N elements (rows of Q and theta)
    nR : int
        Number of R elements (rows of q and columns of theta)
    nH : int
        Number of H elements (new dimension)
    total : int
        Total number of instance files to generate
    seed : int or None
        Optional random seed for reproducibility
    """
    if seed is not None:
        np.random.seed(seed)

    if nN == 0 and nR == 0 and nH == 0:
        instances_dir = Path.cwd() / "instances_test"
        nN, nR, nH = 3, 5, 2
    else:
        if nR <= 9:
            instances_dir = Path.cwd() / f"instances_{nN}_0{nR}_{nH}"
        else:
            instances_dir = Path.cwd() / f"instances_{nN}_{nR}_{nH}"
        instances_dir.mkdir(parents=True, exist_ok=True)

    results = []

    for i in range(1, total + 1):
        # filename
        if nR <= 9:
            filename = instances_dir / f"benchmark_{nN}_0{nR}_{nH}-{i}.dat"
        else:
            filename = instances_dir / f"benchmark_{nN}_{nR}_{nH}-{i}.dat"

        print(filename)

        # === Generate parameters with constraints ===
        a = np.random.randint(5, 26, size=(nN, nR))
        c = np.random.randint(30, 51, size=(nN, nN, nR))
        # b = np.random.randint(10, 20)
        # e = np.random.randint(5, 10)
        i, j = np.indices((nN, nN))
        mask = i >= j
        c[mask] = 0

        condition = True

        while condition:
            q_cols = []
            for h in range(nH):
                if h == 0:
                    col_k = np.random.randint(5, 26, size=(nN, nR))
                else:
                    col_k = np.random.randint(105, 126, size=(nN, nR))
                q_cols.append(col_k)
            q = np.stack(q_cols, axis=0)
            q = np.transpose(q, (1, 2, 0))

            sum_i_h = q.sum(axis=1)
            max_cap = sum_i_h / nN

            # Q: nN * nH
            # Q = np.ceil(0.8 * max_cap).astype(int).tolist()

            factors = np.full((nN, nH), 1.2)
            factors[:, 0] = 0.8  # 0.8

            Q = np.ceil(factors * max_cap).astype(int).tolist()
            break

            # for r in range(nN):
            #    for h in range(nH):
            #        total_capacity = Q[r][h] * nN
            #        total_requirement = sum_i_h[r][h]
            #        # assert (
            #        #    total_capacity >= total_requirement
            #        # ), f"Infeasible! Resource {r}, Agent {h}: capacity={total_capacity}, requirement={total_requirement}"
            #        if total_capacity < total_requirement:
            #            condition = False
            #            break
            #    if not condition:
            #        break

        q.tolist()

        p = np.round(np.random.uniform(0.0, 1.0, size=nR), 2).tolist()

        # write to file
        with filename.open("w", encoding="utf-8") as f:
            f.write(f"nN = {nN};\n")
            f.write(f"nR = {nR};\n")
            f.write(f"nH = {nH};\n\n")
            f.write(f"a = {_format_list(a)};\n")
            f.write(f"c = {_format_list(c)};\n")
            # f.write(f"b = {b};\n")
            # f.write(f"e = {e};\n\n")
            f.write(f"Q = {_format_list(Q)};\n\n")
            f.write(f"q = {_format_list(q)};\n")
            f.write(f"p = {_format_list(p, fmt='{:.2f}')};\n")

        results.append(
            {
                "nN": nN,
                "nR": nR,
                "nH": nH,
                "Q": Q,
                "q": q,
                "p": p,
                "a": a,
                "c": c,
                "filepath": str(filename.resolve()),
            }
        )

    return results


if __name__ == "__main__":
    # Small
    # create_instances(nN=2, nR=11, nH=1, total=10)
    # create_instances(nN=2, nR=11, nH=2, total=10)
    # create_instances(nN=2, nR=15, nH=1, total=10)
    # create_instances(nN=2, nR=15, nH=2, total=10)

    # create_instances(nN=2, nR=5, nH=1, total=10)
    # create_instances(nN=2, nR=5, nH=2, total=10)
    # reate_instances(nN=2, nR=8, nH=1, total=10)
    # create_instances(nN=2, nR=8, nH=2, total=10)
    # create_instances(nN=3, nR=8, nH=1, total=10)
    # create_instances(nN=3, nR=8, nH=2, total=10)
    ## Medium
    # create_instances(nN=3, nR=10, nH=1, total=10)
    # create_instances(nN=3, nR=10, nH=2, total=10)
    # create_instances(nN=3, nR=12, nH=1, total=10)
    # create_instances(nN=3, nR=12, nH=2, total=10)
    # create_instances(nN=4, nR=12, nH=1, total=10)
    # create_instances(nN=4, nR=12, nH=2, total=10)
    ### Large
    # create_instances(nN=4, nR=15, nH=1, total=10)
    # create_instances(nN=4, nR=15, nH=2, total=10)
    # create_instances(nN=5, nR=15, nH=1, total=10)
    # create_instances(nN=5, nR=15, nH=2, total=10)

    # create_instances(nN=4, nR=5, nH=1, total=10)
    # create_instances(nN=4, nR=7, nH=1, total=10)
    # create_instances(nN=4, nR=10, nH=1, total=10)

    # create_instances(nN=4, nR=17, nH=1, total=10)
    create_instances(nN=5, nR=15, nH=1, total=10)
    # create_instances(nN=6, nR=15, nH=1, total=10)
    # create_instances(nN=7, nR=15, nH=1, total=10)


# create_instances(nN=0, nR=0, nH=0, total=1)
