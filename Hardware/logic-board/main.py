import os
import subprocess
import time
import serial

def kill_rfcomm_processes():
    subprocess.run(['export TERM=xterm-256color'], shell=True)
    try:
        broken_process = subprocess.Popen(['top -b -n 1 | grep \' rfcomm\''], shell=True, stdout=subprocess.PIPE)
        output = broken_process.stdout.readline()
        while output:
            output = output[:len(output)-1].decode()
            print(output)
            os.system('sudo kill ' + output[:output.index('r')-1])
            broken_process = subprocess.Popen(['top -b -n 1 | grep \' rfcomm\''], shell=True, stdout=subprocess.PIPE)
            output = broken_process.stdout.readline()
    except:
        print('exception in kill_rfcomm_processes')


def init_bluetooth():
    # Turn on bluetooth
    os.system('rfkill unblock bluetooth')
    # Make rpi discoverable
    os.system('bluetoothctl discoverable on')
    # Accept any incoming connection
    connect = subprocess.Popen(['bt-agent', '--capability=NoInputNoOutput'], shell=False, stdout=subprocess.PIPE)
    
    prev_output = ""
    while True:
        try:
            output = connect.stdout.readline()
        except:
            continue
        if connect.poll() is not None:
            break
        if output:
            # Convert from byte type to string
            output = output[:len(output)-1].decode()
            print(output)
            if "Device:" in output:
                # output = Name + UID of device
                output = output[:output.rindex('for')-1]       
                if prev_output == output:
                    # Device is connected
                    time.sleep(7)
                    # Turn discoverable off and terminate scan 
                    os.system('bluetoothctl discoverable off')
                    connect.terminate()
                    print('Device Connected Successfuly')
                    return 0
                else:
                    prev_output = output
    print('Connecting Failed')
    return 1

def init_serial_comm():
    while True:
        try:
            serial_result = subprocess.Popen(['sudo rfcomm watch hci0'], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if serial_result.stderr.readline():
                output = serial_result.stderr.readline()
            else:
                output = serial_result.stdout.readline()
        except:
            continue
        if serial_result.poll() is not None:
            break
        if output:
            # Convert from byte type to string
            output = output[:len(output)-1].decode()
            print(output)
            if 'Connection from' in output:
                print('Serial Connection Created Successfuly')
                return 0
            elif 'bind RFCOMM socket: Address already in use' in output:
                kill_rfcomm_processes()
        else:
            ser = serial.Serial('/dev/rfcomm0')
            ser.write(b'Hello There')
    return 1

def main():
    failed = init_bluetooth()
    if not failed:
        init_serial_comm()

if __name__ == "__main__":
    main()
