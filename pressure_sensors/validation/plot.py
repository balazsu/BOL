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
p_range = [-5, 60]

fig = plt.figure()
ax = fig.add_subplot(1 , 1, 1)

t = np.arange(0, int(N)) * T_s
p = [0] * N
ax.set_ylim(p_range)

line, = ax.plot(t, p, linewidth=4)

plt.title("Pressure measurements over time")
plt.ylabel("Pressure [mmH2O]")
plt.xlabel("Time [s]")
plt.grid(True)

def animate(i, p):
    if i == N:
        print("Acquired 1 minute of data and saved.")
        np.save('test-long-tube', [t, p])
        return line,

    if i > N:
        return line,

    ser.flushInput()
    data = ser.readline()

    try:
        print(float(data.decode()))
        p.append(float(data.decode()))
    except ValueError:
        print(data)
        print("ValueError occured.")
        return line,

    p = p[-N:]
    # Smooth a bit the data
    w = 1
    p = np.convolve(p, np.ones((w,))/w, mode='full')
    p = p[0:N]

    line.set_ydata(p)

    return line,

ani = animation.FuncAnimation(fig,
                              animate,
                              fargs = (p,),
                              interval = 40,
                              blit = True)
plt.show()

ser.close()
