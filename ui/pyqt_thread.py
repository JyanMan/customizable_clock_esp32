from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QMainWindow, QStackedLayout
from PyQt6.QtGui import QColor, QPalette
from PyQt6.QtWidgets import QWidget
from PyQt6.QtCore import Qt


import sys
import threading
import queue


MCU_CANVAS_WIDTH = 320
MCU_CANVAS_HEIGHT = 240


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
    def __init__(self, test_q):
        super().__init__()

        self.mouse_state = MouseState

        self.test_q: queue.Queue = test_q

        self.timer_label = TimerLabel("red")
        
        self.layout = QStackedLayout()
        # layout.addWidget(Color("grey"))
        self.layout.addWidget(self.timer_label)
        self.layout.addWidget(Color("pink"))

        self.layout.setCurrentIndex(2)

        self.main_widget = QWidget()
        self.main_widget.setLayout(self.layout)
        self.setCentralWidget(self.main_widget)


    def mouseMoveEvent(self, e):
        self.timer_label.updateMouseDrag(self.mouse_state, e)


    def mouseDoubleClickEvent(self, e):
        self.mouse_state.drag = True

        new_pos = e.position()
        
        self.mouse_state.x = new_pos.x()
        self.mouse_state.y = new_pos.y()


    def mouseReleaseEvent(self, e):
        if self.mouse_state.drag:
            w_ratio = MCU_CANVAS_WIDTH / self.width()
            h_ratio = MCU_CANVAS_HEIGHT / self.height()

            self.test_q.put_nowait((
               int(round(self.timer_label.x() * w_ratio)),
               int(round(self.timer_label.y() * h_ratio))
            ))

        self.mouse_state.drag = False
            

def app_thread(test_q: queue.Queue):
    app = QApplication(sys.argv)

    # Create a Qt widget, which will be our window.
    window = MainWindow(test_q)
    window.show()  # IMPORTANT!!!!! Windows are hidden by default.
    app.exec()


if __name__ == "__main__":
    q = queue.Queue()
    app_thread(q)
