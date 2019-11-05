import peewee
from PyQt5.QtCore import QSize, pyqtSlot
from PyQt5.QtWidgets import (
	QMainWindow, QDialog, QFrame, QHBoxLayout, QVBoxLayout, QLabel,
	QListWidget, QPushButton, QSpinBox)

from upvtc_ct import models


class AddRecordDialog():
	cached_add_record_windows = dict()

	@classmethod
	def for_model(cls, model: str, attrs=[]):
		"""
		Creates a new dialog for creating new records of a model. If a dialog
		has already been created beforehand, we use the previously created
		dialog.
		"""
		if model in cls.cached_add_record_windows:
			return cls.cached_add_record_windows[model]
		else:
			dialog = QDialog()
			dialog.setModal(True)
			dialog.setWindowTitle(f'Add {model}')

			# Add in the GUI equivalents of the fields in the model.
			dialog_layout = QVBoxLayout(dialog)
			#dialog.setLayout(dialog_layout)

			m = getattr(models, model)
			for attr in attrs:
				model_attr = getattr(m, attr)
				attr_type = type(model_attr)
				attr_layout = None
				if attr_type is peewee.ManyToManyField:
					raise NotImplementedError()
				elif attr_type is peewee.ForeignKeyField:
					raise NotImplementedError()
				elif attr_type is peewee.SmallIntegerField:
					attr_layout = QHBoxLayout()

					capitalize_func = lambda s: s.capitalize()
					attr_label = QLabel(
						' '.join(list(map(capitalize_func, attr.split('_')))))
					attr_layout.addWidget(attr_label)

					attr_layout.addStretch(1)

					attr_spinbox = QSpinBox()
					attr_spinbox.setMinimum(0)
					attr_layout.addWidget(attr_spinbox)

				dialog_layout.addLayout(attr_layout)

			return dialog


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

		study_plan_list = QListWidget()
		study_plan_panel_layout.addWidget(study_plan_list)

		study_plan_action_btn_layout = QHBoxLayout()
		remove_study_plan_btn = QPushButton('Remove')
		edit_study_plan_btn = QPushButton('Edit')
		add_study_plan_btn = QPushButton('Add')
		study_plan_action_btn_layout.addStretch(1)
		study_plan_action_btn_layout.addWidget(remove_study_plan_btn)
		study_plan_action_btn_layout.addWidget(edit_study_plan_btn)
		study_plan_action_btn_layout.addWidget(add_study_plan_btn)
		study_plan_panel_layout.addLayout(study_plan_action_btn_layout)

		# Create the panel that will display the subjects required in a
		# selected study plan, and expose action buttons that will add or
		# remove subjects from the selected study plan.
		study_plan_subjects_panel_layout = QVBoxLayout()

		study_plan_subjects_panel_title = QLabel('Subjects')
		study_plan_subjects_panel_layout.addWidget(
			study_plan_subjects_panel_title)

		subject_list = QListWidget()
		study_plan_subjects_panel_layout.addWidget(subject_list)

		subject_action_btn_layout = QHBoxLayout()
		remove_subject_btn = QPushButton('Remove')
		edit_subject_btn = QPushButton('Edit')
		add_subject_btn = QPushButton('Add')
		subject_action_btn_layout.addStretch(1)
		subject_action_btn_layout.addWidget(remove_subject_btn)
		subject_action_btn_layout.addWidget(edit_subject_btn)
		subject_action_btn_layout.addWidget(add_subject_btn)
		study_plan_subjects_panel_layout.addLayout(subject_action_btn_layout)

		study_plan_ui_layout.addLayout(study_plan_panel_layout)
		study_plan_ui_layout.addLayout(study_plan_subjects_panel_layout)

		# Create the top-level action buttons that will allow for adding,
		# editing, and removing information of divisions, courses, etc., and
		# invoking the scheduling process.
		top_level_action_btn_layout = QHBoxLayout()
		edit_information_btn = QPushButton('Edit School Information')
		top_level_action_btn_layout.addStretch(1)
		top_level_action_btn_layout.addWidget(edit_information_btn)
		top_level_action_btn_layout.addStretch(1)

		main_layout = QVBoxLayout()
		main_layout.addLayout(study_plan_ui_layout)
		main_layout.addLayout(top_level_action_btn_layout)

		frame.setLayout(main_layout)

		# Connect the buttons to actions.
		add_study_plan_btn.clicked.connect(self._add_study_plan_action)

	@pyqtSlot()
	def _add_study_plan_action(self):
		action_dialog = AddRecordDialog().for_model(
			'StudyPlan',
			[ 'year_level', 'num_followers' ])
		action_dialog.show()
		action_dialog.exec_()
