import serial
import xmodem
import sys

def send(port, filename):
    ser = serial.Serial(
        port,
        baudrate=115200,
        bytesize=8,
        parity='N',
        stopbits=1,
        timeout=2,
        rtscts=False,
        dsrdtr=False
    )

    def getc(size, timeout=1):
        return ser.read(size) or None

    def putc(data, timeout=1):
        return ser.write(data)

    modem = xmodem.XMODEM(getc, putc)

    print(f"Waiting for bootloader on {port}...")
    
    with open(filename, 'rb') as f:
        success = modem.send(f, quiet=False)
        if success:
            print("Transfer complete!")
        else:
            print("Transfer failed!")

    ser.close()

if __name__ == '__main__':
    send('COM3', 'application.bin')