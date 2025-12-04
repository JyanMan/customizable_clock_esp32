
import sys
import ble_setup as ble
import pyqt_thread as pyqt
import asyncio
import threading
import time
import queue

test_q = queue.Queue()
read_queue = queue.Queue()

def ble_thread():
    asyncio.run(ble.ble_setup(ble.Args("NimBLE_GATT"), test_q, read_queue))


def main():

    pyqt.app_thread(test_q, read_queue)

    # threads = [
    #     threading.Thread(target=ble_thread, daemon=True),
    #     threading.Thread(target=pyqt.app_thread, args=(test_q, read_queue))
    # ]

    # for t in threads:
    #     t.start()


    # for t in threads:
    #     t.join()


if __name__ == "__main__":
    main()
    
