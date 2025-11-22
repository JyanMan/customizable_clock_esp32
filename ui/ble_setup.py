import argparse
import asyncio
import logging
import queue

from typing import Optional

from bleak import BleakScanner, BleakClient
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData

import keyboard


def check_for_key():
    key_press = keyboard.read_key()
    # hex values are defined in esp code
    if (key_press == 'w'):
        print("")
        return 0x01
    elif (key_press == 'a'):
        print("")
        return 0x02
    elif (key_press == 's'):
        print("")
        return 0x03
    elif (key_press == 'd'):
        print("")
        return 0x04
    elif (key_press == 'q'):
        print("")
        return 0x05
    elif (key_press == 'j'):
        print("")
        return 0x06
    elif (key_press == 'k'):
        print("")
        return 0x07
    else:
        return None


"""
while True:
    main()
"""


logger = logging.getLogger(__name__)


class Args(argparse.Namespace):
    # name: Optional[str]
    # address: Optional[str]
    # macos_use_bdaddr: bool
    # services: list[str]
    # pair: bool
    # debug: bool

    def __init__(self, name: str, services: list[str] = None):
        self.name = name
        self.services = services
        self.macos_use_bdaddr = False
        self.pair = False
        self.debug = False

        


async def ble_setup(args: Args, test_q: queue.Queue):
#     args = parser.parse_args(namespace=Args())
    # log_level = logging.DEBUG if args.debug else logging.INFO
    # logging.basicConfig(
    #     level=log_level,
    #     format="%(asctime)-15s %(name)-8s %(levelname)s: %(message)s",
    # )

    # args = Args("NimBLE_GATT", None)
    log_level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(
        level=log_level,
        format="%(asctime)-15s %(name)-8s %(levelname)s: %(message)s",
    )
    logger.info("starting scan...")
    print("asdfasdfasdfasdf")

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

                if test_q.empty():
                    continue

                result = test_q.get_nowait();  
                if result:
                    print("received")
                test_q.task_done()

                data_to_send = 0x01
                # if (data_to_send is None):
                #     continue

                await client.write_gatt_char(chr_uuid, bytes([data_to_send]), response=False)

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

 
