from PyQt5.QtCore import QSize
from PyQt5.QtWidgets import (
	QMainWindow, QFrame, QLabel, QListWidget, QHBoxLayout,
	QVBoxLayout, QPushButton)


class MainWindow(QMainWindow):
	def __init__(self):
		super().__init__()

		self.setMinimumSize(QSize(640, 480))
		self.setWindowTitle('UPVTC Automated Course Timetabler')

		frame = QFrame(self)
		self.setCentralWidget(frame)

		# Create the top-level study plan panel.
		study_plan_ui_layout = QHBoxLayout()

		# Create the panel that will display the current study plans and will
		# expose action buttons that will edit, remove, or add study plans.
		study_plan_panel_layout = QVBoxLayout()

		study_plan_panel_title = QLabel('Study Plans')
		study_plan_panel_layout.addWidget(study_plan_panel_title)

		self.study_plan_list = QListWidget()
		study_plan_panel_layout.addWidget(self.study_plan_list)

		study_plan_action_btn_layout = QHBoxLayout()
		self.remove_study_plan_btn = QPushButton('Remove')
		self.edit_study_plan_btn = QPushButton('Edit')
		self.add_study_plan_btn = QPushButton('Add')
		study_plan_action_btn_layout.addStretch(1)
		study_plan_action_btn_layout.addWidget(self.remove_study_plan_btn)
		study_plan_action_btn_layout.addWidget(self.edit_study_plan_btn)
		study_plan_action_btn_layout.addWidget(self.add_study_plan_btn)
		study_plan_panel_layout.addLayout(study_plan_action_btn_layout)

		# Create the panel that will display the subjects required in a
		# selected study plan, and expose action buttons that will add or
		# remove subjects from the selected study plan.
		study_plan_subjects_panel_layout = QVBoxLayout()

		study_plan_subjects_panel_title = QLabel('Subjects')
		study_plan_subjects_panel_layout.addWidget(
			study_plan_subjects_panel_title)

		self.subject_list = QListWidget()
		study_plan_subjects_panel_layout.addWidget(self.subject_list)

		subject_action_btn_layout = QHBoxLayout()
		self.remove_subject_btn = QPushButton('Remove')
		self.edit_subject_btn = QPushButton('Edit')
		self.add_subject_btn = QPushButton('Add')
		subject_action_btn_layout.addStretch(1)
		subject_action_btn_layout.addWidget(self.remove_subject_btn)
		subject_action_btn_layout.addWidget(self.edit_subject_btn)
		subject_action_btn_layout.addWidget(self.add_subject_btn)
		study_plan_subjects_panel_layout.addLayout(subject_action_btn_layout)

		study_plan_ui_layout.addLayout(study_plan_panel_layout)
		study_plan_ui_layout.addLayout(study_plan_subjects_panel_layout)

		# Create the top-level action buttons that will allow for adding,
		# editing, and removing information of divisions, courses, etc., and
		# invoking the scheduling process.
		top_level_action_btn_layout = QHBoxLayout()
		self.edit_information_btn = QPushButton('Edit School Information')
		top_level_action_btn_layout.addStretch(1)
		top_level_action_btn_layout.addWidget(self.edit_information_btn)
		top_level_action_btn_layout.addStretch(1)

		main_layout = QVBoxLayout()
		main_layout.addLayout(study_plan_ui_layout)
		main_layout.addLayout(top_level_action_btn_layout)

		frame.setLayout(main_layout)
