import serial
import numpy as np
import datetime as dt
import matplotlib.pyplot as plt

# Setup serial port
ser = serial.Serial()
ser.port = '/dev/ttyACM0'
ser.baudrate = 9600

ser.close()
ser.open()

print("Serial port opened.")

T_acq = 60
T_s = 50e-3
N = int(T_acq/T_s) + 1

t = np.arange(0, int(N)) * T_s
p = [0] * N

n = 0;
while n < N:
    ser.flushInput()
    data = ser.readline()
    try:
        value = float(data.decode()) / 1e4
        print(value)
        p[n] = value
        n += 1
    except ValueError:
        print(data)
        print("ValueError occured.")

filename = 'test-long-tube'
print("Acquired 1 minute of data. Saved in " + filename + ".npy.")
np.save('filename', [t, p])

fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)

ax.plot(t, p, linewidth=4)

plt.title("Pressure measurements over time")
plt.ylabel("Pressure [cmH2O]")
plt.xlabel("Time [s]")
plt.grid(True)

plt.show()

ser.close()
print("Serial port closed.")

