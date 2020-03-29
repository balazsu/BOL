import sys
import getopt
import serial
import numpy as np
import datetime as dt
import matplotlib.pyplot as plt

def main(argv):
    meas_type = ''
    filename = 'data/'

    try:
        opts, args = getopt.getopt(argv, 'm:o:', ['meas=', 'output='])
    except getopt.GetoptError as err:
        print('acq.py -m flow/volume -o <filename>')
        sys.exit(2)

    for opt, arg in opts:
        if opt == '-m':
            meas_type = arg
        elif opt == '-o':
            filename += arg

    # Setup serial port
    ser = serial.Serial()
    ser.port = '/dev/ttyUSB0'
    ser.baudrate = 9600

    ser.close()
    ser.open()

    print("Serial port opened.")

    T_acq = 60
    T_s = 50e-3
    N = int(T_acq/T_s) + 1

    time = np.arange(0, int(N)) * T_s
    meas = [0] * N

    # Throw first meaningless measurements + warm-up serial port (weird delay for
    # first readline() otherwise...)
    i = 0
    while i < 5:
        ser.flushInput()
        ser.readline()
        i += 1

    print("Ready to acquire. Press enter to start.")
    input()

    n = 0;
    while n < N:
        # Make sure to get the last value to avoid lagging behind
        ser.flushInput()
        data = ser.readline()
        try:
            # data sent from the Arduino is in mL, we want flow in L/min
            if meas_type == "volume":
                value = int(data.decode())
            else: # meas_type == "flow"
                value = int(data.decode()) / 1e3

            print(value)
            meas[n] = value
            n += 1
        except ValueError:
            print(data)
            print("ValueError occured.")

    print("Acquired " + str(T_acq) + " seconds of data.")
    print("Saved in data/" + filename + ".npy.")
    np.save(filename, [time, meas])

    fig = plt.figure()
    ax = fig.add_subplot(1, 1, 1)

    ax.plot(time, meas, linewidth=4)

    if meas_type == 'flow':
        plt.title("Flow measurement with Sensirion SMF3300-D")
        plt.ylabel("Flow [L/min]")
    else: # meas_type == 'volume'
        plt.title("Volume measurement with Sensirion SMF3300-D")
        plt.ylabel("Volume [mL]")

    plt.xlabel("Time [s]")
    plt.grid(True)

    plt.show()

    ser.close()
    print("Serial port closed.")

if __name__ == '__main__':
    main(sys.argv[1:])
