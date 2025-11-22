from PyQt6.QtWidgets import QApplication, QWidget

import sys
import ble_setup as ble
import pyqt_thread as pyqt
import asyncio
import threading
import time
import queue

test_q = queue.Queue()

def ble_thread():
    asyncio.run(ble.ble_setup(ble.Args("NimBLE_GATT"), test_q))


def main():

    threads = [
        threading.Thread(target=ble_thread, daemon=True),
        threading.Thread(target=pyqt.app_thread, args=(test_q,))
    ]

    for t in threads:
        t.start()


    for t in threads:
        t.join()


if __name__ == "__main__":
    main()
    
