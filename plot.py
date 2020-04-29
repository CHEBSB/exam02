import matplotlib.pyplot as plt
import numpy as np
import serial
import time

t = np.arange(0,10,0.1) # time vector; create Fs samples between 0 and 0.1 sec.
x = np.arange(-1,1,0.02) 
y = np.arange(-1,1,0.02) 
z = np.arange(-1,1,0.02) 
move = np.arange(0,1,0.01) 
n = len(y) # length of the signal

serdev = '/dev/ttyACM0'
s = serial.Serial(serdev,115200)
for i in range(n):    
    line=s.readline() # Read an echo string from K66F terminated with '\n'
    # print line
    # seperate the line into 4 parts #
    Tp = line.split()   # split whitespace
    x[i] = float(Tp[0])
    y[i] = float(Tp[1])
    z[i] = float(Tp[2])
    move[i] = int(Tp[3])
    #
    
fig, ax = plt.subplots(2, 1)
ax[0].plot(t,x, 'r')
ax[0].plot(t,y, 'y')
ax[0].plot(t,z, 'b')
ax[0].set_xlim(0, 10)
ax[0].legend("xyz",loc='center left', bbox_to_anchor=(1, 0.5))
ax[0].set_xlabel('Time')
ax[0].set_ylabel('Acc Vector')
ax[1].stem(t,move,'r') 
ax[1].set_xlim(0, 10)
ax[1].set_xlabel('Time')
ax[1].set_ylabel('move > 5cm')
plt.show()
s.close()