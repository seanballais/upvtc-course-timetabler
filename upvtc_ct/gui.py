import peewee
from PyQt5.QtCore import QSize, QTime, pyqtSlot
from PyQt5.QtWidgets import (
	QMainWindow, QDialog, QFrame, QHBoxLayout, QVBoxLayout, QComboBox, QLabel,
	QLineEdit, QListWidget, QPushButton, QSpinBox, QDoubleSpinBox, QTimeEdit)

from upvtc_ct import models


class AddRecordDialog():
	cached_add_record_windows = dict()

	@classmethod
	def for_model(cls,
				  model: str,
				  attrs=[],
				  title=None,
				  save_func=None,
				  cancel_func=None,
				  reset_on_close=True):
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
			dialog.setWindowTitle(title or f'Add {model}')

			# Add in the GUI equivalents of the fields in the model.
			dialog_layout = QVBoxLayout(dialog)
			dialog.setLayout(dialog_layout)

			m = getattr(models, model)
			attr_widgets = dict()  # key -> (widget object, default value,)
			for attr in attrs:
				attr_field = getattr(m, attr)

				attr_type = type(attr_field)
				attr_layout = None
				capitalize_func = lambda s: s.capitalize()
				attr_label = QLabel(
					' '.join(list(map(capitalize_func, attr.split('_')))))
				# TODO: Refactor these conditional blocks. Use a design
				#       pattern. You may refer to the following link:
				#           https://itnext.io/how-we-avoided-if-else-and-wrote
				#           -extendable-code-with-strategy-pattern-256e34b90caf
				if attr_type is peewee.CharField:
					attr_layout = QHBoxLayout()

					attr_layout.addWidget(attr_label)
					attr_layout.addStretch(1)

					attr_textfield = QLineEdit()
					attr_layout.addWidget(attr_textfield)

					attr_widgets[attr] = (attr_textfield, '',)
				elif attr_type is peewee.SmallIntegerField:
					attr_layout = QHBoxLayout()

					attr_layout.addWidget(attr_label)
					attr_layout.addStretch(1)

					attr_spinbox = QSpinBox()
					attr_spinbox.setMinimum(0)
					attr_layout.addWidget(attr_spinbox)

					attr_widgets[attr] = (attr_spinbox, 0,)
				elif attr_type is peewee.DecimalField:
					attr_layout = QHBoxLayout()

					attr_layout.addWidget(attr_label)
					attr_layout.addStretch(1)

					attr_spinbox = QDoubleSpinBox()
					attr_spinbox.setMinimum(1.0)
					attr_spinbox.setSingleStep(0.5)
					attr_layout.addWidget(attr_spinbox)

					attr_widgets[attr] = (attr_spinbox, 1.0,)
				elif attr_type is peewee.TimeField:
					attr_layout = QHBoxLayout()

					attr_layout.addWidget(attr_label)
					attr_layout.addStretch(1)

					attr_timefield = QTimeEdit(QTime(7, 0))
					attr_timefield.setMinimumTime(QTime(7, 0))
					# The time below is the start of the last timeslot
					# of a day.
					attr_timefield.setMaximumTime(QTime(18, 30))
					attr_layout.addWidget(attr_timefield)

					attr_widgets[attr] = (attr_timefield, QTime(7, 0),)
				elif attr_type is peewee.ForeignKeyField:
					attr_layout = QVBoxLayout()

					attr_layout.addWidget(attr_label)

					attr_options = QComboBox()
					attr_options.addItem('None', None)
					linked_model = attr_field.rel_model
					for record in linked_model.select():
						attr_options.addItem(str(record), record)
					attr_layout.addWidget(attr_options)

					attr_widgets[attr] = (attr_options, None,)
				elif attr_type is peewee.ManyToManyField:
					attr_layout = QVBoxLayout()

					attr_layout.addWidget(attr_label)

					# Add the panel that lets you select linked model instances
					# and add them to the instances the new model instance
					# is linked to.
					attr_add_options_layout = QHBoxLayout()
					attr_options = QComboBox()
					linked_model = attr_field.rel_model
					for record in linked_model.select():
						attr_options.addItem(str(record), record)

					attr_add_option_btn = QPushButton('Add')

					attr_add_options_layout.addWidget(attr_options)
					attr_add_options_layout.addWidget(attr_add_option_btn)

					attr_layout.addLayout(attr_add_options_layout)

					attr_widgets[f'{attr}_options'] = (attr_options, None,)

					# Add the list of linked model instances connected to the
					# new model instance.
					attr_linked_models = QListWidget()
					attr_layout.addWidget(attr_linked_models)

					attr_widgets[f'{attr}_linked_list'] = (
						attr_linked_models, None,
					)

					# Add the action buttons for linked model instances.
					attr_linked_models_action_btn_layout = QHBoxLayout()
					attr_remove_linked_model_btn = QPushButton('Remove')
					attr_edit_linked_model_btn = QPushButton('Edit')
					attr_linked_models_action_btn_layout.addStretch(1)
					attr_linked_models_action_btn_layout.addWidget(
						attr_remove_linked_model_btn)
					attr_linked_models_action_btn_layout.addWidget(
						attr_edit_linked_model_btn)

					attr_layout.addLayout(attr_linked_models_action_btn_layout)

				dialog_layout.addLayout(attr_layout)

			dialog_action_btn_layout = QHBoxLayout()
			save_btn = QPushButton('Save')
			@pyqtSlot()
			def save_action():
				if save_func is not None:
					save_func()
				dialog.done(QDialog.Accepted)

			cancel_btn = QPushButton('Cancel')
			@pyqtSlot()
			def cancel_action():
				if cancel_func is not None:
					cancel_func()
				dialog.done(QDialog.Rejected)
			cancel_btn.clicked.connect(cancel_action)

			dialog_action_btn_layout.addStretch(1)
			dialog_action_btn_layout.addWidget(cancel_btn)
			dialog_action_btn_layout.addWidget(save_btn)
			dialog_layout.addLayout(dialog_action_btn_layout)

			if reset_on_close:
				@pyqtSlot()
				def reset_widgets():
					# TODO: Refactor these conditional blocks. Use a design
					#       pattern. You may refer to the following link:
					#           https://itnext.io/how-we-avoided-if-else-and-
					#			wrote-extendable-code-with-strategy-pattern-
					#           256e34b90caf
					for _, attr_info in attr_widgets.items():
						widget, default_widget_value = attr_info
						if type(widget) is QLineEdit:
							widget.setText(default_widget_value)
						elif (type(widget) is QSpinBox
							  or type(widget) is QDoubleSpinBox):
							widget.setValue(default_widget_value)
						elif type(widget) is QTimeEdit:
							widget.setTime(default_widget_value)
						elif type(widget) is QComboBox:
							widget.setCurrentIndex(0)  # The item with an index
													   # of 0 will always be
													   # None.
						elif type(widget) is QListWidget:
							widget.clear()

				dialog.finished.connect(reset_widgets)

			# Add the model to the cache so that we don't have to keep on
			# recreating the dialog.
			cls.cached_add_record_windows[model] = dialog

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
			[ 'course', 'year_level', 'num_followers', 'subjects' ],
			title='Add a new study plan')
		action_dialog.show()
		action_dialog.open()
