from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QMainWindow, QVBoxLayout
from PyQt6.QtGui import QColor, QPalette
from PyQt6.QtWidgets import QWidget
from PyQt6.QtCore import Qt

from PyQt6.QtWidgets import QApplication, QWidget
from PyQt6.QtCore import QThread, QObject, pyqtSignal
from PyQt5.QtCore import QRunnable, Qt, QThreadPool
from enum import Enum

import asyncio
import sys
import threading
import queue
import ble_setup as ble


MCU_CANVAS_WIDTH = 320
MCU_CANVAS_HEIGHT = 240


class QueueRead[Enum]:
    Transform = 0x01


class LabelContainer:
    def __init__(self):
        self.timer = TimerLabel("red")




class WriteData:
    def __init__(self, data_type: int,  timer_x: int, timer_y: int):
        self.data_type = data_type
        self.timer_x = timer_x
        self.timer_y = timer_y


class Color(QWidget):
    def __init__(self, color):
        super().__init__()
        self.setAutoFillBackground(True)

        palette = self.palette()
        palette.setColor(QPalette.ColorRole.Window, QColor(color))
        self.setPalette(palette)

        
class MouseState:
    prev_x: int = 0
    prev_y: int = 0
    x: int = 0
    y: int = 0
    drag: bool = False


class TimerLabel(Color):
    def __init__(self, color):
        super().__init__(color)
        self.setFixedSize(500, 300)
        

    def updateMouseDrag(self, mouse_state: MouseState, e):

        # check if overlapping the box
        timer_x = self.x()
        timer_y = self.y()
        max_x = timer_x + self.width()
        max_y = timer_y + self.height()

        mouse_x = mouse_state.x
        mouse_y = mouse_state.y
        
        if not (mouse_x > timer_x and mouse_x < max_x \
            and mouse_y > timer_y and mouse_y < max_y):
            mouse_state.drag = False
            return;

        if not mouse_state.drag:
            return
        
        new_pos = e.position()

        dir_x = new_pos.x() - mouse_x
        dir_y = new_pos.y() - mouse_y
        
        mouse_state.prev_x = mouse_x
        mouse_state.prev_y = mouse_y

        mouse_state.x = new_pos.x()
        mouse_state.y = new_pos.y()

        self.move(timer_x + int(dir_x), timer_y + int(dir_y))




class MainWindow(QMainWindow):
    def __init__(self, test_q, read_queue):
        super().__init__()

        self.mouse_state = MouseState

        self.test_q: queue.Queue = test_q
        self.read_queue: queue.Queue = read_queue;

        self.labels = LabelContainer()
        
        self.sync_from_mcu_btn = QPushButton()
        self.sync_from_mcu_btn.clicked.connect(self.sync_from_mcu)
        self.sync_from_mcu_btn.setFixedSize(150, 70)
        self.sync_from_mcu_btn.setText("Sync From MCU")
        self.sync_from_mcu_btn.move(200, 0)

        self.layout = QVBoxLayout()
        self.layout.addWidget(self.sync_from_mcu_btn)
        self.layout.addWidget(self.labels.timer)

        self.main_widget = Color("pink")
        self.main_widget.setLayout(self.layout)
        self.setCentralWidget(self.main_widget)

        self.init_workers()


    def init_workers(self):

        pool = QThreadPool.globalInstance()
        self.ble_worker = BleWorker(self.test_q, self.read_queue)
        self.queue_worker = QueueWorker(self.test_q, self.read_queue, self)
        pool.start(self.ble_worker)
        pool.start(self.queue_worker)
        # self.thread = QThread()

        # self.ble_worker = BleWorker(self.test_q, self.read_queue)
        # self.ble_worker.moveToThread(self.thread)

        # self.queue_worker = QueueWorker(self.test_q, self.read_queue)
        # self.queue_worker.moveToThread(self.thread)

        # self.thread.started.connect(self.ble_worker.run)
        # self.thread.started.connect(self.queue_worker.run)
        # self.thread.start()


    def update_time_label_pos(self, x, y, w, h):
        w_ratio = self.width() / MCU_CANVAS_WIDTH
        h_ratio = self.height() / MCU_CANVAS_HEIGHT
        self.labels.timer.move(
            int(x * w_ratio),
            int(y * h_ratio)
        )

        # width = int(w * w_ratio)
        # height = int(h * h_ratio)
        # self.labels.timer.setFixedSize(width, height)


    def sync_from_mcu(self):
        self.test_q.put_nowait(WriteData(0, 0, 0))
        pass
        # self.test_q.put_nowait()
        # if not self.read_queue.empty():
        #     new_pos: queue.Queue = self.read_queue.get_nowait()
        #     x = new_pos[0]
        #     y = new_pos[1]
        #     w_ratio = self.width() / MCU_CANVAS_WIDTH
        #     h_ratio = self.height() / MCU_CANVAS_HEIGHT
        #     self.labels.timer.move(
        #         int(x * w_ratio),
        #         int(y * h_ratio)
        #     )

        #     width = int(new_pos[2] * w_ratio)
        #     height = int(new_pos[3] * h_ratio)
        #     self.labels.timer.setFixedSize(width, height)


    def mouseMoveEvent(self, e):
        self.labels.timer.updateMouseDrag(self.mouse_state, e)


    def mouseDoubleClickEvent(self, e):
        self.mouse_state.drag = True

        new_pos = e.position()
        
        self.mouse_state.x = new_pos.x()
        self.mouse_state.y = new_pos.y()


    def mouseReleaseEvent(self, e):
        if self.mouse_state.drag:
            w_ratio = MCU_CANVAS_WIDTH / self.width()
            h_ratio = MCU_CANVAS_HEIGHT / self.height()

            self.test_q.put_nowait(WriteData(
               1,
               int(round(self.labels.timer.x() * w_ratio)),
               int(round(self.labels.timer.y() * h_ratio)),
            ))

        self.mouse_state.drag = False
            

def app_thread(test_q: queue.Queue, read_queue: queue.Queue):
    
    app = QApplication(sys.argv)

    # Create a Qt widget, which will be our window.
    window = MainWindow(test_q, read_queue)
    window.show()  # IMPORTANT!!!!! Windows are hidden by default.
    app.exec()


class QueueWorker(QRunnable):
    def __init__(self, write_queue, read_queue, main_window):
        super().__init__()        
        self.write_queue = write_queue
        self.read_queue = read_queue
        self.main_window: MainWindow = main_window


    def read_queues(self):
        if not self.read_queue.empty():
            data: bytes = self.read_queue.get_nowait()
            data_type = data[0]
            print(f"data_type: {data_type}")
            match data_type:
                case QueueRead.Transform:
                    print("received current transform from mcu")
                    x = int.from_bytes(data[3:5], byteorder='little')
                    y = int.from_bytes(data[1:3], byteorder='little')
                    w = int.from_bytes(data[5:9], byteorder="little")
                    h = int.from_bytes(data[9:13], byteorder="little")
                    self.main_window.update_time_label_pos(x, y, w, h)
                    print(f"x: {x}, y: {y}, w: {w}, h: {h}")
                    # send data to window
                case _:
                    print(f"unknown received data type {data_type}")


    def run(self):
        while True:
            self.read_queues()
            


class BleWorker(QRunnable):
    def __init__(self, test_q, read_queue):
        super().__init__()
        self.test_q: queue.Queue = test_q
        self.read_queue: queue.Queue = read_queue


    def run(self):
        asyncio.run(ble.ble_setup(ble.Args("NimBLE_GATT"), self.test_q, self.read_queue))


if __name__ == "__main__":
    q = queue.Queue()
    q2 = queue.Queue()
    app_thread(q, q2)
