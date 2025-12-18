from enum import Enum


MCU_CANVAS_WIDTH = 320
MCU_CANVAS_HEIGHT = 240


class StopwatchUiData:
    def __init__(self, x, y, w, h):
        self.x = x
        self.y = y
        self.w = w
        self.h = h


class WriteDataType(Enum):
    RequestData=0x00
    TimerPosition=0x01


class QueueRead[Enum]:
    Transform = 0x01


class WriteData:
    def __init__(self, data_type: WriteDataType,  timer_x: int = 0, timer_y: int = 0):
        self.data_type = data_type
        self.timer_x = timer_x
        self.timer_y = timer_y
