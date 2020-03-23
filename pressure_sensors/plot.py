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

t_len = 600             # Number of points to display
p_range = [-5, 700]      # Range of the data to plot

fig = plt.figure()
ax = fig.add_subplot(1 , 1, 1)

T_mes = 50e-3

t = np.arange(0, int(t_len)) * T_mes
p = [0] * t_len
ax.set_ylim(p_range)

line, = ax.plot(t, p, linewidth=5)

plt.title("Pressure measurements over time")
plt.ylabel("Pressure [mmH2O]")
plt.xlabel("Time [s]")
plt.grid(True)

def animate(i, p):
    ser.flushInput()
    data = ser.readline()
    print(data)

    try:
        p.append(float(data.decode()))
    except ValueError:
        return line,

    p = p[-t_len:]
    # Smooth a bit the data
    N = 5
    p = np.convolve(p, np.ones((N,))/N, mode='full')
    p = p[0:t_len]

    line.set_ydata(p)

    return line,

ani = animation.FuncAnimation(fig,
                              animate,
                              fargs = (p,),
                              interval = 20,
                              blit = True)
plt.show()

ser.close()
