from PyQt5.QtCore import QSize
from PyQt5.QtWidgets import QMainWindow


class MainWindow(QMainWindow):
	def __init__(self):
		QMainWindow.__init__(self)

		self.setMinimumSize(QSize(800, 600))
		self.setWindowTitle('UPVTC Automated Course Timetabler')
