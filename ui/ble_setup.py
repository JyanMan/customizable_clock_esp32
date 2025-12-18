import argparse
import asyncio
import logging
import queue

from enum import Enum
from typing import Optional

from pyqt_thread import WriteData

from bleak import BleakScanner, BleakClient
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData
from protocol import WriteDataType, WriteData, StopwatchUiData, QueueRead


class Args(argparse.Namespace):

    def __init__(self, name: str, services: list[str] = None):
        self.name = name
        self.services = services
        self.macos_use_bdaddr = False
        self.pair = False
        self.debug = False



async def read_data_from_mcu(client, request_data, chr_uuid):
    try: 
        data= await client.read_gatt_char(chr_uuid)

        if len(data) != 0:
            print(f"received from mcu data: {data}")

            # send data to window
            data_type = data[0]
            match data_type:
                case QueueRead.Transform:
                    print("received current transform from mcu")
                    x = int.from_bytes(data[3:5], byteorder='little')
                    y = int.from_bytes(data[1:3], byteorder='little')
                    w = int.from_bytes(data[9:13], byteorder="little")
                    h = int.from_bytes(data[5:9], byteorder="little")
                    print(f"received bounds --> x: {x}, y: {y}, w: {w}, h: {h}")
            # self.main_window.update_time_label_pos(x, y, w, h)

            request_data.emit(StopwatchUiData(x, y, w, h))
            # read_queue.put_nowait(read_current_data)
    except Exception as e:
        print(e)


async def send_data_to_mcu(client, write_queue: queue.Queue, chr_uuid):
    if write_queue.empty():
        return

    result: WriteData = write_queue.get_nowait();  

    match result.data_type:
        case WriteDataType.RequestData:
            print("requested for data from mcu")
            await client.write_gatt_char(chr_uuid, bytes(b"\x00\x00\x00\x00\x00"), response=False)
        case WriteDataType.TimerPosition:
            # data_to_send: int =  result.x() | (result.y() << 16)
            x16 = result.timer_x & 0xFFFF
            y16 = result.timer_y & 0xFFFF

            data_to_send = x16 | (y16 << 16)
            pos_bytes= data_to_send.to_bytes(4, byteorder="little", signed=False)
            data_bytes = b"\x01" + pos_bytes

            print(f"sent to mcu in bytes -> x: {x16}, y: {y16}, combined: {data_bytes}")

            await client.write_gatt_char(chr_uuid, data_bytes, response=False)
    
    if result:
        print("received")

    write_queue.task_done()
    

async def ble_setup(args: Args, write_queue: queue.Queue, request_data):
    logger = logging.getLogger(__name__)
    log_level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(
        level=log_level,
        format="%(asctime)-15s %(name)-8s %(levelname)s: %(message)s",
    )
    logger.info("starting scan...")

    if args.name:
        device = await BleakScanner.find_device_by_name(
            args.name, cb={"use_bdaddr": args.macos_use_bdaddr}
        )
        if device is None:
            logger.error("could not find device with name '%s'", args.name)
            return
    else:
        raise ValueError("Either --name or --address must be provided")

    logger.info("connecting to device...")

    async with BleakClient(
        device,
        pair=args.pair,
        services=args.services,
        # Give the user plenty of time to enter a PIN code if paring is required.
        timeout=90 if args.pair else 10,
    ) as client:
        logger.info("connected to %s (%s)", client.name, client.address)

        svc_uuid = "002E4767-C69D-1382-9944-B99FE7FAF2D2"
        chr_uuid = "46F65758-1557-EF97-124E-D90845DBDAA2"
        nus = client.services.get_service(svc_uuid)
        
        if nus is None:
            logger.info("no service for controller found...")
        else:
            while True:

                await asyncio.sleep(0.05)  # prevent too fast change
                await read_data_from_mcu(client, request_data, chr_uuid)
                await send_data_to_mcu(client, write_queue, chr_uuid)

        logger.info("disconnecting...")

    logger.info("disconnected")


# if __name__ == "__main__":
    # parser = argparse.ArgumentParser()

    # device_group = parser.add_mutually_exclusive_group(required=True)

    # device_group.add_argument(
    #     "--name",
    #     metavar="NimBLE_GATT",
    #     help="the name of the bluetooth device to connect to",
    # )
    # # device_group.add_argument(
    # #     "--address",
    # #     metavar="<address>",
    # #     help="the address of the bluetooth device to connect to",
    # # )

    # parser.add_argument(
    #     "--macos-use-bdaddr",
    #     action="store_true",
    #     help="when true use Bluetooth address instead of UUID on macOS",
    # )

    # parser.add_argument(
    #     "--services",
    #     nargs="+",
    #     metavar="a2dadb45-08d9-4e12-97ef-57155857f646",
    #     help="if provided, only enumerate matching service(s)",
    # )

    # parser.add_argument(
    #     "--pair",
    #     action="store_true",
    #     help="pair with the device before connecting if not already paired",
    # )

    # parser.add_argument(
    #     "-d",
    #     "--debug",
    #     action="store_true",
    #     help="sets the log level to debug",
    # )

    # args = parser.parse_args(namespace=Args())

    # args = Args("NimBLE_GATT", None)

    # asyncio.run(main(args))

 
