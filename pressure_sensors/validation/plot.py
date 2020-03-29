import csv
import sys
import numpy as np
import matplotlib.pyplot as plt

from parse import parse

def main():
    # Data from Flowlab
    date, title, notes, labels, data_citrex = \
    parse("data/pressure-mes-distanced-long-tube.log")

    plt.figure()
    plt.plot(data_citrex[:-1, 0], data_citrex[:-1, 1], color='g', label="CITREX H4")

    # Data from flow sensor right in front of the CITREX H4
    data_sensor = np.load("data/pressure-mes-distanced-long-tube.npy", allow_pickle=True)
    plt.plot(data_sensor[0] + 0.2, data_sensor[1], color='r', label="MPX5010DP")

    plt.title(title + " - Sensor at BMV output, long tube - "  + date + "\n" + ', '.join(notes))
    plt.legend()
    plt.xlabel(labels[0])
    plt.grid(True)
    plt.show()

if __name__ == "__main__":
    main()
