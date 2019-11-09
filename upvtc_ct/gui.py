from dataclasses import dataclass
import logging

import peewee
from PyQt5.QtCore import Qt, QSize, QTime, pyqtSlot
from PyQt5.QtWidgets import (
	QMainWindow, QDialog, QWidget, QFrame, QGridLayout, QHBoxLayout,
	QVBoxLayout, QComboBox, QLabel, QLineEdit, QListWidget, QListWidgetItem,
	QPushButton, QSpinBox, QDoubleSpinBox, QTimeEdit)

from upvtc_ct import models, utils

# We seriously need tests around here.
app_logger = logging.getLogger()


@dataclass
class _RecordDialogWidget():
	attr: str
	widget: QWidget
	default: any


class _RecordDialog(QDialog):
	def __init__(self, parent=None):
		super().__init__(parent)

		self.widgets = dict()
		self.has_performed_modifications = False


class RecordDialogFactory():
	@classmethod
	def create_add_dialog(cls, model, attrs=[], title=None):
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

		for attr in attrs:
			attr_field = getattr(model, attr)
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

				if attr_options.count() == 0:  # No items in the combo box.
					attr_add_option_btn.setEnabled(False)

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

				# Add the behaviours.
				@pyqtSlot()
				def add_selected_options_item():
					item_index = attr_options.currentIndex()
					item = attr_options.itemData(item_index)
					item_text = attr_options.itemText(item_index)

					attr_options.removeItem(item_index)

					item_list_item = QListWidgetItem(item_text)
					item_list_item.setData(Qt.UserRole, item)

					attr_linked_models.addItem(item_list_item)

				attr_add_option_btn.clicked.connect(add_selected_options_item)

				attr_widget = attr_linked_models
				attr_default_value = []

			if model_instance is not None:
				attr_default_value = getattr(model_instance, attr)

			dialog.widgets[attr] = _RecordDialogWidget(
				attr, attr_widget, attr_default_value)

			dialog_layout.addLayout(attr_layout)

		dialog_action_btn_layout = QHBoxLayout()
		
		save_btn = QPushButton('Save')
		cancel_btn = QPushButton('Cancel')

		@pyqtSlot()
		def _save_action():
			# TODO: Fix issue where creating an instance model with a
			#       non-nullable field set to null causes a crash. To fix this,
			#       do not show a "None" option to a non-nullable field.
			# We're an "Add Record" dialog. Otherwise, we're an "Edit Record"
			# dialog.
			if model_instance is None:
				model_instance = model()

			for widget in dialog.widgets.values():
				# Maybe I should have checked the widget type instead
				# the field type of the attribute?
				attr_type = type(getattr(model, widget.attr))
				if attr_type is peewee.CharField:
					setattr(model_instance, widget.attr, widget.widget.text())
				elif attr_type is peewee.SmallIntegerField:
					setattr(model_instance, widget.attr, widget.widget.value())
				elif attr_type is peewee.DecimalField:
					setattr(model_instance, widget.attr, widget.widget.value())
				elif attr_type is peewee.TimeField:
					setattr(
						model_instance,
						widget.attr,
						widget.widget.toPython())
				elif attr_type is peewee.ForeignKeyField:
					setattr(
						instance,
						widget.attr,
						widget.widget.currentData())
				elif attr_type is peewee.ManyToManyField:
					# Clearing away all associated objects then adding them
					# back in is pretty much a nuclear method for editing an
					# instance's many-to-many field. But, it is the easiest
					# to implement anyway. No need to optimize for now.
					list_widget = widget.widget

					list_widget.clear()  # This only *technically* matters
										 # when we're an "Edit Record" dialog.
					
					field = getattr(instance, widget.attr)
					for index in range(list_widget.count()):
						field.add(widget.widget.item(i))

			new_instance.save()

			dialog.has_performed_modifications = True

			dialog.done(QDialog.Accepted)
		save_btn.clicked.connect(_save_action)

		@pyqtSlot()
		def _cancel_action():
			dialog.done(QDialog.Rejected)
		cancel_btn.clicked.connect(_cancel_action)

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
		information_models = [
			models.Division,
			models.Course,
			models.Subject,
			models.Teacher,
			models.Room,
			models.RoomFeature
		]
		information_models_titles = [
			'Division', 'Course', 'Subject', 'Teacher', 'Room', 'Room Feature'
		]
		information_model_attrs = [
			[ 'name' ],
			[ 'name', 'division' ],
			[ 'name', 'units', 'division', 'candidate_teachers' ],
			[ 'first_name', 'last_name', 'preferred_timeslots' ],
			[ 'name', 'division', 'features' ],
			[ 'name' ]
		]
		ctr = 0
		for model, title, attrs in zip(
				information_models,
				information_models_titles,
				information_model_attrs):
			model_layout = QVBoxLayout()

			# Let's start with the label.
			panel_title = QLabel(f'{title}s')
			model_layout.addWidget(panel_title)

			# And, of course, the list.
			model_instances_list = QListWidget()
			model_layout.addWidget(model_instances_list)
			for instance in model.select():
				list_item = QListWidgetItem(str(instance))
				list_item.setData(Qt.UserRole, instance)

				model_instances_list.addItem(list_item)
			model_instances_list.sortItems()

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

			# Add the behaviours.
			# This connection might pose a problem when working with multiple
			# threads.
			add_btn.clicked.connect(
				# Why do we need to create arguments to the lambda function
				# below? 
				(lambda checked,
						model=model,
						attrs=attrs,
						title=title,
						model_instances_list = model_instances_list :
					self._add_model_instance_action(
						model, attrs, title, model_instances_list)))

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

	@pyqtSlot()
	def _add_model_instance_action(self,
								   model,
		 						   attrs,
								   title,
		 						   model_instances_list):
		add_dialog = RecordDialogFactory.create_add_dialog(
			model, attrs, f'Add a new {title.lower()}')

		add_dialog.show()
		add_dialog.exec_()  # Must do this to Block execution of the
							# code below until the dialog is closed.

		if add_dialog.has_performed_modifications:
			new_instance = (model
							.select()
							.order_by(model.id.desc())
							.get())
			new_list_item = QListWidgetItem(str(new_instance))
			new_list_item.setData(Qt.UserRole, new_instance)

			model_instances_list.addItem(new_list_item)
			model_instances_list.sortItems()


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
		study_plan_action_btn_layout.addWidget(remove_study_plan_btn)
		study_plan_action_btn_layout.addStretch(1)
		study_plan_action_btn_layout.addWidget(edit_study_plan_btn)
		study_plan_action_btn_layout.addWidget(add_study_plan_btn)
		study_plan_panel_layout.addLayout(study_plan_action_btn_layout)

		study_plan_ui_layout.addLayout(study_plan_panel_layout)

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
		action_dialog = RecordDialogFactory.create_add_dialog(
			models.StudyPlan,
			[ 'course', 'year_level', 'num_followers', 'subjects' ],
			title='Add a new study plan')
		action_dialog.show()
