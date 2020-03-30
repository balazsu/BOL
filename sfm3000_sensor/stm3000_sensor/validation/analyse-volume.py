import csv
import sys
import numpy as np
import matplotlib.pyplot as plt

from parse import parse

def main():
    files = [   "C10-R20-PEEP10-105cm",
                "C10-R20-PEEP10-310cm",
                "C15-R20-PEEP10-105cm",
                "C15-R20-PEEP10-310cm",
                "C15-R20-PEEP15-105cm",
                "C15-R20-PEEP15-310cm"]

    for f in files:
        # Data from Flowlab
        date, title, notes, labels, data_citrex = parse("data/" + f +".log")

        # Remove non-pertinent data, for which Ppeak > 38cmH2O
        ppeak = data_citrex[:, 5]
        rem = ppeak > 38

        plt.figure()

        data_citrex[rem, 2] = 0
        plt.plot(data_citrex[:-1, 0], data_citrex[:-1, 2], color='g', label="CITREX H4 Gas Flow Analyzer")
        plt.plot(data_citrex[:-1, 0] - 2.5, data_citrex[:-1, 3], color='g', label="CITREX H4 Gas Flow Analyzer (Vti)")

        # Data from flow sensor right in front of the CITREX H4
        data_sensor = np.load("data/" + f + ".npy", allow_pickle=True)
        data_sensor[1][rem] = 0
        plt.plot(data_sensor[0] + 0.5, data_sensor[1], color='r', label="Sensirion SMF3300-D Flow Meter")

        plt.title("Sensirion SMF3300-D volume validation - "  + date + "\n" + ', '.join(notes))
        plt.legend()
        plt.xlabel(labels[0])
        plt.grid(True)

    plt.show()

if __name__ == "__main__":
    main()
