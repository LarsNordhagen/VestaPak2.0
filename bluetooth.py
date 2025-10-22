import datetime
import asyncio
import pandas 
from bleak import BleakScanner, BleakClient
from warning import no_water, low_water

VOLUME_CHAR = "19B10001-E8F2-537E-4F6C-D104768A1214"

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
        
        dictionary = {}
        
        #Documentation: https://bleak.readthedocs.io/en/latest/api/client.html
        async with BleakClient(device) as client:

            #Update dictionary with what is printed    
            while True:
                volume_byte = await client.read_gatt_char(VOLUME_CHAR)
                volume = volume_byte.decode('utf-8')

                #If the volume is not in our dictionary, add the volume & respective time to the dictionary
                if volume not in dictionary:
                    dictionary[volume] = datetime.now()

                    try:
                        volume_as_num = float(volume)
                        #If the volume is over 1980 mL, there is almost (or no) water 
                        if volume_as_num >= 1980:

                            #Send note to BLE (and wait for it)
                            await no_water(client)

                            #Write to a CSV file
                            df = pandas.DataFrame({'Volume': list(dictionary.keys()), 'Time': list(dictionary.values())})
                            df.to_csv("output.csv")
                            
                            #break from infinite while loop
                            break
                        
                        #If the volume is over 1700 mL, warn user. Have warning task run in the background so we can continue to have the main program run
                        elif volume_as_num >= 1700:
                            asyncio.create_task(low_water())

                    except ValueError:
                        print("Unable to convert volume into float")
                
    except OSError:
        print("Unable to connect via bluetooth. Did you turn on Bluetooth mode?")


if __name__ == "__main__":
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    try:
        loop.run_until_complete(main())
    finally:
        loop.close()




    

