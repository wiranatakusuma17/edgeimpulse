# Program ini digunakan untuk merekam gerakan tangan selama 50 sesi dengan durasi 5 detik per sesinya dengan 
# Setiap sesi terdapat jeda antar sesi selama 3 detik
# Terdapat jeda 10 detik yang ditandai dengan indikator LED internal ESP32 yang berkedip sebelum program mulai merekam gerakan pengguna
# Pada saat merekam LED internal ESP32 akan menyala, perekaman berlangsung selama kurang lebih 7 menit untuk tiap 50 sesi
# MPU6050 SDA(21) SCL(21)
# modul sdcard SCK(18) MOSI(23) MISO (19) CS(5)


from machine import Pin, I2C, SPI
import utime
import uos
import sdcard
from mpu6050 import init_mpu6050, get_mpu6050_data

# Inisialisasi I2C untuk MPU6050
i2c = I2C(0, scl=Pin(22), sda=Pin(21), freq=400000)
init_mpu6050(i2c)

# Inisialisasi SPI untuk SD Card
spi = SPI(2, baudrate=1000000, sck=Pin(18), mosi=Pin(23), miso=Pin(19))
cs = Pin(5, Pin.OUT)
sd = sdcard.SDCard(spi, cs)
vfs = uos.VfsFat(sd)
uos.mount(vfs, "/sd")

# Inisialisasi LED indikator
led = Pin(2, Pin.OUT)  
led.off()

# Konstanta
g = 9.80665
sampling_rate = 100  # 100 Hz
interval_ms = int(1000 / sampling_rate)
samples_per_session = 500
total_sessions = 50
delay_between_sessions = 3000  

# Delay awal untuk persiapan
print("Menunggu 10 detik sebelum mulai logging...")
for i in range(10):
    led.on()
    utime.sleep(0.5)
    led.off()
    utime.sleep(0.5)

# Mulai Logging
led.on()  

for session in range(1, total_sessions + 1):
    print("Mulai sesi", session)
    filename = "/sd/berlari.{}.csv".format(session) # nama file dapat diganti sesuai aktifitas

    buffer = ["time_ms,x,y,z\n"]  # Header file CSV
    t0 = utime.ticks_ms()

    for i in range(samples_per_session):
        start = utime.ticks_ms()

        t_now = utime.ticks_diff(start, t0)
        data = get_mpu6050_data(i2c)

        ax_raw = data['accel']['x']
        ay_raw = data['accel']['y']
        az_raw = data['accel']['z']

        # Orientasi baru (karena pemasangan mpu horizontal)
        ax = ay_raw * g
        ay = -ax_raw * g
        az = az_raw * g

        buffer.append("{},{:.2f},{:.2f},{:.2f}\n".format(t_now, ax, ay, az))

        elapsed = utime.ticks_diff(utime.ticks_ms(), start)
        if elapsed < interval_ms:
            utime.sleep_ms(interval_ms - elapsed)

    # Simpan ke SD card
    with open(filename, "w") as f:
        for line in buffer:
            f.write(line)

    print("Sesi {} selesai. File: {}".format(session, filename))
    utime.sleep_ms(delay_between_sessions)

led.off()  

print("Semua {} sesi selesai. Data tersimpan di SD card.".format(total_sessions))
