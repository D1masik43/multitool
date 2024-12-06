# multitool
esp32 based autonomous device with some cool features that phones dont have(network scanning and analising, environment sensors and 1W led) and basic visual interface from 2000-2010 phones

files provided here is arduino IDE project ready to be compiled and and flashed into hardware <br>
<mark>Note</mark> you need to add all libraries from libraries.rar to you libraries folder here is  <br>  [Dropbox URL](https://www.dropbox.com/scl/fi/wp7r0yfxdzot3yjx6nobu/libraries.rar?rlkey=o91tcoq83o2jp8xmjinzu1evk&st=tn82knd4&dl=0)  
to avoid any issues i posted my full folder (some additional megabytes must not be a problem)
generally entire project is a collection of different already created be someone funtions and methods to work with them. My part was to put everything together using some king of visual interface and make it usable with buttons.
# hardware
Ina219 sensor
1.8 SPI TFT display
ina219 current sensor
qmc5883l commpas
ds3231 time module
AHT20+BMP280 temperature/humidity/pressure module
MAX98357 audio amplifier + speaker
5v 2A powerbank module
nrf24l01 module
1w led + 3.3v regulator
buzzer 
line of buttons and resistors (analog button input using single gpio pin)


project is still under development but here is some images of UI:

![App Menu](https://github.com/D1masik43/multitool/blob/images/app%20menu.jpg)  <br>
![NRF Scanner](https://github.com/D1masik43/multitool/blob/images/nrf%20scanner.jpg) <br>
for more look at Images branch



