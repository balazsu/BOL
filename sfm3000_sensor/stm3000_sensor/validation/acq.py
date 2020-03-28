import serial
import numpy as np
import datetime as dt
import matplotlib.pyplot as plt

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

t = np.arange(0, int(N)) * T_s
flow = [0] * N

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
        value = int(data.decode()) / 1e3
        print(value)
        flow[n] = value
        n += 1
    except ValueError:
        print(data)
        print("ValueError occured.")

filename = 'data/vol-mes-at-ambubag-long-tube'
print("Acquired " + str(T_acq) + " seconds of data. Saved in " + filename + ".npy.")
np.save(filename, [t, flow])

print(flow)

fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)

ax.plot(t, flow, linewidth=4)

plt.title("Flow measurements over time")
plt.ylabel("Flow [L/min]")
plt.xlabel("Time [s]")
plt.grid(True)

plt.show()

ser.close()
print("Serial port closed.")

