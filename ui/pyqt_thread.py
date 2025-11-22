from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QMainWindow

import sys
import threading
import queue


class MainWindow(QMainWindow):
    def __init__(self, test_q):
        super().__init__()

        self.test_q: queue.Queue = test_q

        self.setWindowTitle("Clock Editor")

        button = QPushButton("Press Me!")
        button.setCheckable(True)
        button.clicked.connect(self.the_button_was_clicked)

        # Set the central widget of the Window.
        self.setCentralWidget(button)

    def the_button_was_clicked(self):
        print("Clicked from window!")
        self.test_q.put_nowait(True)


def app_thread(test_q: queue.Queue):
    app = QApplication(sys.argv)

    # Create a Qt widget, which will be our window.
    window = MainWindow(test_q)
    window.show()  # IMPORTANT!!!!! Windows are hidden by default.
    app.exec()


if __name__ == "__main__":
    app_thread()
