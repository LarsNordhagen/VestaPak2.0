from bleak import BleakClient
from bleak.exc import BleakCharacteristicNotFoundError

NO_WATER_CHAR = "19B10002-E8F2-537E-4F6C-D104768A1214"

#Warn user about low water
async def low_water():

    #Write to BLE device without a response. Ideally, it should write to the app, but we can test it on the bluetooth module to see if it works
    warning_msg = "300 mL of water left in your hydration bladder"
    print(warning_msg)

#Warn user about no water
async def no_water(client: BleakClient):

    #Write to BLE device with a response. Ideally, it should alert the user if there is no water in the mobile app, but we can test it on the bluetooth module to see if it works
    no_water_msg = "No Water"
    try:
        await client.write_gatt_char(NO_WATER_CHAR, data = no_water_msg.encode(), response = False)
    except BleakCharacteristicNotFoundError:
        print("NO_WATER_CHARACTERISTIC is not found")