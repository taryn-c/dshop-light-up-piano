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
                    return 0
                else:
                    prev_output = output
    return 1

def main():
    failed = init_bluetooth()
    if failed == 1:
        print('Connecting Failed')
    else:
        print('Device Connected Successfuly')

if __name__ == "__main__":
    main()
