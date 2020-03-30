import csv
import sys
import numpy as np
import matplotlib.pyplot as plt

from parse import parse

def main():
    files = [
                "test",
                "test-lung"
            ]

    for f in files:
        # Data from Flowlab
        date, title, notes, labels, data_citrex = parse("data/" + f +".log")


        plt.figure()
        plt.plot(data_citrex[:-1, 0], data_citrex[:-1, 1], color='g', label="CITREX H4 Gas Flow Analyzer")

        # Data from differentil pressure flow sensor
        data_sensor = np.load("data/" + f + ".npy", allow_pickle=True)
        plt.plot(data_sensor[0] + 0.2, data_sensor[1], color='r',
                 label="Differential pressure flow-meter iFlow 200S + MPX5004DP")

        plt.title("Differential pressure flow-meter feasibility analysis - "  + date + "\n" + ', '.join(notes))
        plt.legend()
        plt.xlabel(labels[0])
        plt.grid(True)

    plt.show()

if __name__ == "__main__":
    main()
