import os
import subprocess
import time

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
                    time.sleep(6)
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
    serial_result = subprocess.Popen(['sudo', 'rfcomm', 'watch', 'hci0'], shell=False, stdout=subprocess.PIPE)
    while True:
        try:
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
                subprocess.Popen(['top', '-b', '|', 'grep', '\' rfcomm\''], shell=False, stdout=subprocess.PIPE)
                try:
                    output = serial_result.stdout.readline()
                except:
                    continue
                if output:
                    output = output[:len(output)-1].decode()
                    os.system('kill ' + output[:output.index(' ')-1])
    return 1

def main():
    failed = init_bluetooth()
    if not failed:
        init_serial_comm()

if __name__ == "__main__":
    main()
