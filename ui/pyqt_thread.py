from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QMainWindow, QVBoxLayout
from PyQt6.QtGui import QColor, QPalette
from PyQt6.QtWidgets import QWidget
from PyQt6.QtCore import Qt

from PyQt6.QtWidgets import QApplication, QWidget
from PyQt6.QtCore import QThread, QObject, pyqtSignal
from PyQt6.QtCore import QRunnable, Qt, QThreadPool
from enum import Enum
from protocol import (
    MCU_CANVAS_WIDTH,
    MCU_CANVAS_HEIGHT,
    WriteData,
    QueueRead,
    WriteDataType,
    StopwatchUiData
)

import asyncio
import sys
import threading
import queue
import ble_setup as ble


class LabelContainer:
    def __init__(self):
        self.timer = TimerLabel("red")


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
    def __init__(self, write_mcu_queue):
        super().__init__()

        self.mouse_state = MouseState

        self.write_mcu_queue: queue.Queue = write_mcu_queue
        # self.read_queue: queue.Queue = read_queue;

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

        self.init_threads()


    def init_threads(self):

        self.ble_worker = BleThread(self.write_mcu_queue)
        self.ble_worker.request_ui_data.connect(self.update_time_label_pos)
        self.ble_worker.start()


    def update_time_label_pos(self, ui_data: StopwatchUiData):
        w_ratio = self.width() / MCU_CANVAS_WIDTH
        h_ratio = self.height() / MCU_CANVAS_HEIGHT
        self.labels.timer.move(
            int(ui_data.x * w_ratio),
            int(ui_data.y * h_ratio)
        )

        width = int(ui_data.w * w_ratio)
        height = int(ui_data.h * h_ratio)
        self.labels.timer.setFixedSize(width, height)


    def sync_from_mcu(self):
        self.write_mcu_queue.put_nowait(WriteData(WriteDataType.RequestData))


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

            self.write_mcu_queue.put_nowait(WriteData(
               WriteDataType.TimerPosition,
               int(round(self.labels.timer.x() * w_ratio)),
               int(round(self.labels.timer.y() * h_ratio)),
            ))

        self.mouse_state.drag = False
            

def app_thread(write_mcu_queue: queue.Queue, read_queue: queue.Queue):
    
    app = QApplication(sys.argv)

    # Create a Qt widget, which will be our window.
    window = MainWindow(write_mcu_queue)
    window.show()  # IMPORTANT!!!!! Windows are hidden by default.
    app.exec()



class BleThread(QThread):
    
    request_ui_data = pyqtSignal(StopwatchUiData)
    
    def __init__(self, write_mcu_queue):
        super().__init__()
        self.write_mcu_queue: queue.Queue = write_mcu_queue


    def run(self):
        asyncio.run(ble.ble_setup(ble.Args("NimBLE_GATT"), self.write_mcu_queue, self.request_ui_data))


if __name__ == "__main__":
    q = queue.Queue()
    q2 = queue.Queue()
    app_thread(q, q2)
