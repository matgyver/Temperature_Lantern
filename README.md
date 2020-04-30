# Temperature_Lantern

![Temperature Lantern](https://raw.githubusercontent.com/matgyver/Temperature_Lantern/master/images/lantern.jpg)

For Christmas, my sister gave me a storm glass.  It is supposed to predict the weather.  Depending on if it is cloudy, or crystals at the top or like now when they are feathery, it indicates current or near current conditions.  It doesn't really work.  But, I thought I would improve on it.  This was a fairly simple and straightforward project using a Circuit Playground Express board.  Circuit Python was used to code it.  A few features were then also added and overall it turned out pretty well.

## The idea
The idea behind this project was to have the storm class change colors based on temperature.  If it was "cool" in the room, it show blue or bluish colors.  If it was typical room temperature, greens, and warmer temperatures would be the orange to red colors.  

To do this, we need to measure the temperature and then map that temperature to values that we can send as colors to the Neopixels.  Overall, I wanted the following features

- Measure the temperature
- Adjust the brightness
- Look nice

I wanted the board to update fairly often so it reacts well to temperature changes and to lighting conditions.  However, I also wanted it to not bounce around as well.  This warranted an averaging filter.

## The hardware
![CPX](https://raw.githubusercontent.com/matgyver/Temperature_Lantern/master/images/cpx.jpg)

A Circuit Playground Express (CPX) made by Adafruit was used for this project.  This board already has a temperature sensor on board, so it can easily measure the temperature.  It actually technically has two, a dedicated temperature sensor and one that is part of the Cortex M0 processor on board.  We'll talk about how we use that to our advantage later.  

The CPX has 10 Neopixels on board.  Neopixels are addresseable RGB LEDs.  These particular ones are just RGB instead of some which are RGBW (includes a white LED).  With these, we can create just about any color.  

This board also has a light sensor.  It will measure how much ambient light there is.  Since this lantern was going to be in our master bedroom, I thought it would be good that it would auto-dim as the lights change.  
