import os
import random
import math
import numpy as np


def parse_instances(path):
    with open(path, "r") as f:
        data = list(map(int, f.read().split()))

    idx = 0
    total = data[idx]
    idx += 1
    instances = []

    for k in range(1, total + 1):
        m = data[idx]
        n = data[idx + 1]
        idx += 2

        C = []
        for _ in range(m):
            C.append(data[idx : idx + n])
            idx += n

        A = F = C.__class__()  # placeholder; rewritten next
        A = []
        for _ in range(m):
            A.append(data[idx : idx + n])
            idx += n

        b = data[idx : idx + m]
        idx += m

        instances.append((m, n, C, A, b))

    return instances


def get_random_parameters(nN, nR, nH, q, Q):
    c = np.random.randint(30, 51, size=(nN, nN, nR))
    i, j = np.indices((nN, nN))
    mask = i >= j
    c[mask] = 0

    if nH > 1:
        q_cols = []
        for h in range(nH - 1):
            col_k = np.random.randint(105, 126, size=(nN, nR))
            q_cols.append(col_k)
        new_q = np.stack(q_cols, axis=2)
        # print(new_q)

        sum_i_h = new_q.sum(axis=1)
        max_cap = sum_i_h / nN

        # new_Q = np.ceil(0.8 * max_cap).astype(int).tolist()

        factors = np.full((nN, nH), 1.2)
        factors[:, 0] = 0.8

        new_Q = np.ceil(factors * max_cap).astype(int).tolist()

        # print(new_Q)
        # print(Q)
        # print()

        # print(new_q)
        # print(q)
        # print()

        Q_list = []
        for i in range(nN):
            line = []
            for h in range(nH):
                if h == 0:
                    line.append(Q[i][h])
                else:
                    line.append(new_Q[i][h - 1])
            Q_list.append(line)

        # print(Q_list)
        # print()
        # print()

        new_q = new_q.tolist()
        q_list = []
        for i in range(nN):
            matrix = []
            for r in range(nR):
                line = []
                for h in range(nH):
                    if h == 0:
                        line.append(q[i][r][h])
                    else:
                        line.append(new_q[i][r][h - 1])
                matrix.append(line)
            q_list.append(matrix)

        # print(q_list)
        # print()
        # print()
    else:
        q_list = q
        Q_list = Q
    return (c, q_list, Q_list)


def save_instances(instances, nH, original_gap_instances=False):

    if original_gap_instances:
        nH = 1

    for inst, (nN, nR, a, q, Q) in enumerate(instances, start=1):
        folder = f"gap_{nN}_{nR}_{nH}"
        os.makedirs(folder, exist_ok=True)

        file_path = os.path.join(folder, f"gap_{nN}_{nR}_{nH}-{inst}.dat")
        q_list = [[[v] for v in row] for row in q]
        Q_list = [[v] for v in Q]

        if not original_gap_instances:
            c, q_list, Q_list = get_random_parameters(nN, nR, nH, q_list, Q_list)

        a = np.array(a).tolist()
        if not original_gap_instances:
            c = np.array(c).tolist()

        q_list = np.array(q_list).tolist()
        Q_list = np.array(Q_list).tolist()

        with open(file_path, "w") as f:
            f.write(f"nN = {nN};\nnR = {nR};\nnH = {nH};\n\n")
            f.write(f"a = {a};\n")
            if not original_gap_instances:
                f.write(f"c = {c};\n\n")
            f.write(f"q = {q_list};\n")
            f.write(f"Q = {Q_list};\n")


if __name__ == "__main__":

    for filename in os.listdir("."):
        if os.path.isfile(filename) and filename[:3] == "gap":
            print(filename)
            instances = parse_instances(filename)
            # nH = 1
            # save_instances(instances, nH, False)
            nH = 2
            save_instances(instances, nH, False)
