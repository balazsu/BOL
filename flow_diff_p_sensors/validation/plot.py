import serial
import numpy as np
import datetime as dt
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# Setup serial port
ser = serial.Serial()
ser.port = '/dev/ttyACM0'
ser.baudrate = 9600

ser.close()
ser.open()

T_acq = 60
T_s = 50e-3

# Number of points to display
N = int(T_acq/T_s) + 1

# Range of the data to plot
flow_range = [0, 100]

fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)

t = np.arange(0, int(N)) * T_s
flow = [0] * N
ax.set_ylim(flow_range)

line, = ax.plot(t, flow, linewidth=4)

plt.title("Flow measurements over time")
plt.ylabel("Flow [L/min]")
plt.xlabel("Time [s]")
plt.grid(True)

def animate(i, flow):
    if i == N:
        print("Acquired 1 minute of data and saved.")
        np.save('calibration', [t, flow])
        return line,

    if i > N:
        return line,

    ser.flushInput()
    data = ser.readline()

    try:
        reading = float(data.decode())
        print(reading)
        flow[i] = reading
    except ValueError:
        print(data)
        print("ValueError occured.")
        return line,

    line.set_ydata(flow)

    return line,

ani = animation.FuncAnimation(fig,
                              animate,
                              fargs = (flow,),
                              interval = 40,
                              blit = True)
plt.show()

ser.close()
