"""
Functions for Weather Lantern Project

Revision 1.0
Updated - 7/27/2020
Matthew E. Nelson

Some code was inspired or copied from Adafruit's website.  Please
support them by visiting their website at www.adafruit.com
"""

upper_reasonable_bound = 200
lower_reasonable_bound = 0
rolling_average_size = 20

def check(token):
    if token == "A":
        return a_button.value
    if token == "B":
        return b_button.value

def average(measurements):
  """
  Use the builtin functions sum and len to make a quick average function
  """
  # Handle division by zero error
  if len(measurements) != 0:
    return sum(measurements)/len(measurements)
  else:
    # When you use the average later, make sure to include something like
    # sensor_average = rolling_average(sensor_measurements)
    # if (conditions) and sensor_average > -1:
    # This way, -1 can be used as an "invalid" value
    return -1

def rolling_average(measurement, measurements):
    # Update rolling average if measurement is ok, otherwise
    # skip to returning the average from previous values
    if lower_reasonable_bound < measurement < upper_reasonable_bound:
    # Remove first item from list if it's full according to our chosen size
        if len(measurements) >= rolling_average_size:
            measurements.pop(0)
        measurements.append(measurement)
    return average(measurements)

def map_values(value, xMin, xMax, yMin, yMax):
    xSpan = xMax - xMin
    ySpan = yMax - yMin
    scaled = float(value - xMin) / float(xSpan)
    return yMin + (scaled * ySpan)