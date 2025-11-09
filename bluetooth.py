import datetime
import asyncio
import pandas 
from bleak import BleakScanner, BleakClient
from warning import no_water, low_water

CONSUMED_VOLUME_CHAR = "19B10001-E8F2-537E-4F6C-D104768A1214"
TOTAL_VOLUME_CHAR = "19B10004-E8F2-537E-4F6C-D104768A1214"

volume_consumed_queue = asyncio.Queue()
total_volume_queue = asyncio.Queue()

#This method tracks the volume that is consumed from the user (flow rate)
def handle_volume_consumed_by_user(sender, data):
    volume_consumed = data.decode('utf-8')
    time_of_consumption = datetime.now()
    asyncio.create_task(volume_consumed_queue.put(volume_consumed, time_of_consumption))

#This method tracks the total volume of water in the hydration bladder (pressure sensor)
def handle_current_volume_in_hydration_bladder(sender, data):
    volume_calculated_from_pressure = data.decode('utf-8')
    asyncio.create_task(total_volume_queue.put(volume_calculated_from_pressure))

#main function
async def main():

    #Documentation https://bleak.readthedocs.io/en/latest/api/scanner.html
    
    #Look for bluetooth Device called Vestapak. Examine for 5 seconds
    try:
        device = await BleakScanner.find_device_by_name("Vestapak", timeout=5)

        #If device is not found, return early
        if device is None:
            print("Failed to connect to said device")
            return 0
        
        total_sessions = []
        while True:
            dictionary = {}
            client = BleakClient(device) #We will create a object without Context Manager so we can easily reconnect if bluetooth disconnects

            #Documentation: https://bleak.readthedocs.io/en/latest/api/client.html
            try:
                await client.connect()

                #Subscribe to Volume and Pressure Characteristics
                await client.start_notify(CONSUMED_VOLUME_CHAR, handle_volume_consumed_by_user)
                await client.start_notify(TOTAL_VOLUME_CHAR, handle_current_volume_in_hydration_bladder)

                #Run while we are connected to bluetooth. If we lose connection, the call back function will be handled
                while client.is_connected:  
                    try:
                        volume_time_pair = await volume_consumed_queue.get() #Example output: (100, 12:56 PM)
                        total_volume_consumed = volume_time_pair[0]

                        #If the volume is not in our dictionary, add the volume & respective time to the dictionary
                        if total_volume_consumed not in dictionary:
                            dictionary[total_volume_consumed] = volume_time_pair[1]
                            try:
                                volume_as_num = float(total_volume_consumed)
                                #If the volume is over 1980 mL, there is almost (or no) water 
                                if volume_as_num >= 1980:

                                    #Send note to BLE (and wait for it)
                                    await no_water(client)

                                    #Write to a CSV file
                                    df = pandas.DataFrame({'Volume': list(dictionary.keys()), 'Time': list(dictionary.values())})
                                    df.to_csv("output.csv")
                                            
                                #If the volume is over 1700 mL, warn user. Have warning task run in the background so we can continue to have the main program run
                                elif volume_as_num >= 1700:
                                    asyncio.create_task(low_water())

                            except ValueError:
                                print("Unable to convert volume into float")
            
                    except asyncio.QueueShutDown:
                        print("Queue has been shutdown")
            finally:
                total_sessions.append(dictionary)
                client.disconnect()        
    except OSError:
        print("Unable to connect via bluetooth. Did you turn on Bluetooth mode?")

if __name__ == "__main__":
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    try:
        loop.run_until_complete(main())
    finally:
        loop.close()