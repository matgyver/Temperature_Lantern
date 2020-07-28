"""
Color temperature light
Matthew E. Nelson
Rev. 1.1
5/1/2020
"""

"""
Some snippets of code was either copied or inspired by code on Adafruit's
website.  Please support them by visiting their website at www.adafruit.com.
The Circuit Playground Express used for this code can be purchased from
their website.
"""
import time
import microcontroller
import adafruit_fancyled.adafruit_fancyled as fancy
from adafruit_circuitplayground import cp
from setup import map_values,check,rolling_average

cp.pixels.auto_write = False
cp.pixels.brightness = 0.005

# Set these based on your ambient temperature in Celsius for best results!
minimum_temp = 15
maximum_temp = 32

# Create empty list-type variable so we can
# store measurements
temp_measurements = []
light_measurements = []

# For state machine
button_a_state = False
button_b_state = False

# This function maps the temps and light levels to control the RGB
# color and brightness used.
def color_temp():
    color = fancy.CHSV(map_values(tempC,minimum_temp,maximum_temp,0.6,0.0),1.0,1.0)
    packed = color.pack()
    cp.pixels
    for i in range(10):
        cp.pixels[i]=packed
    cp.pixels.brightness = map_values(Lux,0,50,0.009,.25)
    cp.pixels.show()
    return

def pulse_color_temp():
    color = fancy.CHSV(map_values(tempC,minimum_temp,maximum_temp,0.6,0.0),1.0,1.0)
    packed = color.pack()
    cp.pixels
    for i in range(10):
        cp.pixels[i]=packed

    for i in range(0,500,1):
        cp.pixels.brightness = map_values(i,0,500,0,.25)
        cp.pixels.show()

    for i in range(500,0,-1):
        cp.pixels.brightness = map_values(i,500,0,.25,0)
        cp.pixels.show()

    return

state = 0

while True:
    #print("Temperature C:", cp.temperature)
    #print("Temperature F:", cp.temperature * 1.8 + 32)
    tempProc = microcontroller.cpu.temperature
    tempF = cp.temperature * 1.8 + 32
    tempC = rolling_average(cp.temperature,temp_measurements)
    Lux = rolling_average(cp.light,light_measurements)
    time.sleep(0.1)
    print((tempC,tempProc,tempF,Lux,))
    if cp.button_a:
        state = 0
    if cp.button_b:
        state = 1
    if state == 0:
        color_temp()
    if state == 1:
        pulse_color_temp()
    if state == 2:
        state = 0
