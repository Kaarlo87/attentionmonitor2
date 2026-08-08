# Lentokoneen asentomonitori (STM32)

STM32-pohjainen laite, joka mittaa ja tulkitsee lentokoneen asennon reaaliajassa. 
Laite lukee IMU-anturia I2C-väylän yli, yhdistää gyroskoopin ja kiihtyvyysanturin 
datan itse kirjoitetulla Kalman-suodattimella vakaaksi kulma-arvioksi, 
piirtää asennon tekohorisonttina OLED-näytölle 
ja varoittaa vaarallisesta asennosta LEDillä ja summerilla
jo ennen kuin raja ylittyy.

