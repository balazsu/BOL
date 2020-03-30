import csv
import sys
import numpy as np
import matplotlib.pyplot as plt

from parse import parse

def main():
    # Data from Flowla
    date, title, notes, labels, data_citrex = \
    parse("data/vol-mes-distanced-short-tube.log")
    #parse("data/vol-mes-at-ambubag-long-tube-citrex.log")
    #parse("data/flow-mes-at-ambubag-long-tube-compl10-citrex.log")
    #parse("data/flow-mes-at-ambubag-short-tube-compl10-citrex.log")
    #parse("data/flow-mes-before-citrex-short-tube-citrex.log")
    #parse("data/vol-mes-face2face-short-tube-citrex.log")

    plt.figure()
    plt.plot(data_citrex[:-1, 0], data_citrex[:-1, 1], color='g', label="CITREX H4")

    # Data from flow sensor right in front of the CITREX H4
    data_sensor = np.load("data/vol-mes-distanced-short-tube.npy", allow_pickle=True)
    #data_sensor = np.load("data/vol-mes-at-ambubag-long-tube.npy", allow_pickle=True)
    #data_sensor = np.load("data/flow-mes-before-citrex-short-tube.npy", allow_pickle=True)
    #data_sensor = np.load("data/flow-mes-at-ambubag-short-tube-comp10.npy", allow_pickle=True)
    #data_sensor = np.load("data/flow-mes-at-ambubag-long-tube-comp10.npy", allow_pickle=True)
    #data_sensor = np.load("data/vol-mes-face2face-short-tube.npy", allow_pickle=True)
    plt.plot(data_sensor[0] + 3, data_sensor[1] * 1000, color='r', label="Sensirion flow sensor")

    plt.title("Volume validation - Sensor at BMV output, short tube - "  + date + "\n" + ', '.join(notes))
    plt.legend()
    plt.xlabel(labels[0])
    plt.grid(True)
    plt.show()

if __name__ == "__main__":
    main()
