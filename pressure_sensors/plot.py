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

n = 0
def animate(i, p):
    ser.flushInput()
    data = ser.readline()

    #try:
    p.append(float(data.decode()))
    n =+ 1
    #except ValueError:
    #    print("ValueError occured.")
    #    return line,

    p = p[-N:]
    # Smooth a bit the data
    w = 1
    p = np.convolve(p, np.ones((w,))/w, mode='full')
    p = p[0:N]

    line.set_ydata(p)

    if n >= N:
       print("Acquired " + N + " samples.")

    return line,

ani = animation.FuncAnimation(fig,
                              animate,
                              fargs = (p,),
                              interval = 50,
                              blit = True)
plt.show()

ser.close()
