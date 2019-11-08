from dataclasses import dataclass

import peewee
from PyQt5.QtCore import QSize, QTime, pyqtSlot
from PyQt5.QtWidgets import (
	QMainWindow, QDialog, QWidget, QFrame, QGridLayout, QHBoxLayout,
	QVBoxLayout, QComboBox, QLabel, QLineEdit, QListWidget, QPushButton,
	QSpinBox, QDoubleSpinBox, QTimeEdit)

from upvtc_ct import models

# We seriously need tests around here.

@dataclass
class _RecordDialogWidget():
	attr: str
	widget: QWidget
	default: any


class _RecordDialog(QDialog):
	def __init__(self, parent=None):
		super().__init__(parent)

		self.widgets = dict()


class RecordDialogFactory():
	@classmethod
	def create_save_dialog(cls, model, attrs=[], title=None):
		return cls._create_dialog(model, None, attrs, title)

	@classmethod
	def create_edit_dialog(cls, model_instance, attrs=[], title=None):
		return cls._create_dialog(None, model_instance, attrs, title)

	@classmethod
	def _create_dialog(cls, model, model_instance, attrs=[], title=None):
		"""
		Creates a new dialog for creating new or editing records of a model.
		"""
		# Why still have an `attrs` variable instead of getting the attributes
		# from the model class? This is to allow us to customize the order in
		# which the widgets will be shown. As a side effect, this will also 
		# allow us to specify which attributes to expose. However, originally,
		# I intended to have such variable to make it quicker for me to
		# finish an early version of this function and try it out since I
		# initially did not know how to access the model's fields. The
		# variable's purpose pretty much transformed over time.
		#
		# On hindsight, maybe we should rename `attrs` to fields.
		dialog = _RecordDialog()
		dialog.setModal(True)
		dialog.setWindowTitle(title or f'Add {model}')

		# Add in the GUI equivalents of the fields in the model.
		dialog_layout = QVBoxLayout(dialog)
		dialog.setLayout(dialog_layout)

		m = getattr(models, model)
		for attr in attrs:
			attr_field = getattr(m, attr)
			attr_type = type(attr_field)
			attr_layout = None
			attr_widget = None
			attr_default_value = None

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

				attr_widget = attr_textfield
				attr_default_value = ''
			elif attr_type is peewee.SmallIntegerField:
				attr_layout = QHBoxLayout()

				attr_layout.addWidget(attr_label)
				attr_layout.addStretch(1)

				attr_spinbox = QSpinBox()
				attr_spinbox.setMinimum(0)
				attr_layout.addWidget(attr_spinbox)

				attr_widget = attr_spinbox
				attr_default_value = 0				
			elif attr_type is peewee.DecimalField:
				attr_layout = QHBoxLayout()

				attr_layout.addWidget(attr_label)
				attr_layout.addStretch(1)

				attr_spinbox = QDoubleSpinBox()
				attr_spinbox.setMinimum(1.0)
				attr_spinbox.setSingleStep(0.5)
				attr_layout.addWidget(attr_spinbox)

				attr_widget = attr_spinbox
				attr_default_value = 1.0
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

				attr_widget = attr_timefield
				attr_default_value = QTime(7, 0)
			elif attr_type is peewee.ForeignKeyField:
				attr_layout = QVBoxLayout()

				attr_layout.addWidget(attr_label)

				attr_options = QComboBox()
				attr_options.addItem('None', None)
				linked_model = attr_field.rel_model
				for record in linked_model.select():
					attr_options.addItem(str(record), record)
				attr_layout.addWidget(attr_options)

				attr_widget = attr_options
				attr_default_value = None
			elif attr_type is peewee.ManyToManyField:
				attr_layout = QVBoxLayout()

				attr_layout.addWidget(attr_label)

				# Add the panel that lets you select linked model instances
				# and add them to the instances the new model instance
				# is linked to.
				attr_add_options_layout = QGridLayout()
				attr_add_options_layout.setColumnStretch(0, 1)

				attr_options = QComboBox()
				linked_model = attr_field.rel_model
				for record in linked_model.select():
					attr_options.addItem(str(record), record)

				attr_add_option_btn = QPushButton('Add')

				attr_add_options_layout.addWidget(attr_options, 0, 0)
				attr_add_options_layout.addWidget(attr_add_option_btn, 0, 1)

				attr_layout.addLayout(attr_add_options_layout)

				# Add the list of linked model instances connected to the
				# new model instance.
				attr_linked_models = QListWidget()
				attr_layout.addWidget(attr_linked_models)

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

				dialog.widgets[attr] = _RecordDialogWidget(attr_linked_models)
				attr_widget = attr_linked_models
				attr_default_value = []

			if model_instance is not None:
				attr_default_value = getattr(model_instance, attr)

			dialog.widgets[attr] = _RecordDialogWidget(
				attr, attr_textfield, attr_default_value)

			dialog_layout.addLayout(attr_layout)

		dialog_action_btn_layout = QHBoxLayout()
		
		save_btn = QPushButton('Save')
		@pyqtSlot()
		def save_action():
			if model_instance is None:
				# We're an "Add Record" dialog.
				new_instance = m()
				for widget in dialog.widgets:
					# Maybe I should have checked the widget type instead
					# the field type of the attribute?
					attr_type = type(getattr(m, widget.attr))
					if attr_type is peewee.CharField:
						setattr(m, widget.attr, widget.widget.text())
					elif attr_type is peewee.SmallIntegerField:
						setattr(m, widget.attr, widget.widget.value())
					elif attr_type is peewee.DecimalField:
						setattr(m, widget.attr, widget.widget.value())
					elif attr_type is peewee.TimeField:
						setattr(m, widget.attr, widget.widget.toPython())
					elif attr_type is peewee.ForeignKeyField:
						setattr(m, widget.attr, widget.widget.currentData())
					elif attr_type is peewee.ManyToManyField:
						list_widget = widget.widget
						field = getattr(m, widget.attr)
						field.add([
							widget.widget.item(i) for i in list_widget.count()
						])

				new_instance.save()
			else:
				# We're an "Edit Record" dialog
				pass

			dialog.done(QDialog.Accepted)
		save_btn.clicked.connect(save_action)

		cancel_btn = QPushButton('Cancel')
		@pyqtSlot()
		def cancel_action():
			dialog.done(QDialog.Rejected)
		cancel_btn.clicked.connect(cancel_action)

		dialog_action_btn_divider = DividerLineFactory.create_divider_line(
			QFrame.HLine) 
		dialog_layout.addWidget(dialog_action_btn_divider)

		dialog_action_btn_layout.addStretch(1)
		dialog_action_btn_layout.addWidget(cancel_btn)
		dialog_action_btn_layout.addWidget(save_btn)
		dialog_layout.addLayout(dialog_action_btn_layout)

		return dialog


class DividerLineFactory():
	@classmethod
	def create_divider_line(cls, line_orientation):
		line = QFrame()
		line.setFrameShape(line_orientation)  # Should only accept
											  # QFrame.VLine or QFrame.HLine.
		line.setFrameShadow(QFrame.Raised)

		return line


class EditInformationWindow(QMainWindow):
	_instance = None

	def __init__(self):
		super().__init__()

		self.setMinimumSize(QSize(1280, 480))
		self.setWindowTitle('Edit school information')

		frame = QFrame(self)
		self.setCentralWidget(frame)

		model_information_layout = QHBoxLayout()

		# Automatically create the panels for the specified models.
		information_models = [ 'Division', 'Course', 'Subject', 'Room' ]
		ctr = 0
		for information_model in information_models:
			model_layout = QVBoxLayout()

			# Let's start with the label.
			panel_title = QLabel(f'{information_model}s')
			model_layout.addWidget(panel_title)

			# And, of course, the list.
			model_instances_list = QListWidget()
			model_layout.addWidget(model_instances_list)

			# And then the action buttons.
			action_btn_layout = QHBoxLayout()

			remove_btn = QPushButton('Remove')
			edit_btn = QPushButton('Edit')
			add_btn = QPushButton('Add')
			action_btn_layout.addWidget(remove_btn)
			action_btn_layout.addStretch(1)
			action_btn_layout.addWidget(edit_btn)
			action_btn_layout.addWidget(add_btn)
			model_layout.addLayout(action_btn_layout)

			model_information_layout.addLayout(model_layout)

			if ctr < len(information_models) - 1:
				model_information_layout.addWidget(
					DividerLineFactory().create_divider_line(QFrame.VLine))

			ctr += 1

		frame.setLayout(model_information_layout)

	@classmethod
	def get_instance(cls):
		if EditInformationWindow._instance is None:
			EditInformationWindow._instance = EditInformationWindow()

		return EditInformationWindow._instance


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

		# Add the panel that will add subjects to a study plan.
		add_subjects_layout = QGridLayout()
		add_subjects_layout.setColumnStretch(0, 1)

		subject_options = QComboBox()
		# TODO: Only show subjects that are offered by the division of the
		#       course of the selected study plan.
		for subject in models.Subject.select():
			subject_options.addItem(str(subject), subject)

		add_subject_btn = QPushButton('Add')

		add_subjects_layout.addWidget(subject_options, 0, 0)
		add_subjects_layout.addWidget(add_subject_btn, 0, 1)

		study_plan_subjects_panel_layout.addLayout(add_subjects_layout)

		# Add the rest of the study plan subjects panel.
		subject_list = QListWidget()
		study_plan_subjects_panel_layout.addWidget(subject_list)

		subject_action_btn_layout = QHBoxLayout()
		remove_subject_btn = QPushButton('Remove')
		edit_subject_btn = QPushButton('Edit')
		subject_action_btn_layout.addStretch(1)
		subject_action_btn_layout.addWidget(remove_subject_btn)
		subject_action_btn_layout.addWidget(edit_subject_btn)
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
		edit_information_btn.clicked.connect(
			self._show_edit_information_window)

	@pyqtSlot()
	def _show_edit_information_window(self):
		EditInformationWindow.get_instance().show()

	@pyqtSlot()
	def _add_study_plan_action(self):
		action_dialog = RecordDialogFactory.create_dialog(
			'StudyPlan',
			None,
			[ 'course', 'year_level', 'num_followers', 'subjects' ],
			title='Add a new study plan')
		action_dialog.show()
