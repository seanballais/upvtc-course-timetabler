from collections import OrderedDict
import logging

from upvtc_ct import models

from ._exceptions import InvalidStartingTimeslotIndex
from . import _utils


app_logger = logging.getLogger()


class _Student():
	def __init__(self):
		self._classes = set()

	@property
	def classes(self):
		return self._classes			

	def add_class(self, new_class):
		self._classes.add(new_class)


class _Timetable():
	def __init__(self):
		self._timetable = OrderedDict()
		self._timeslot_classes = dict()
		self._timeslot_indexes = dict()
		_timeslot_ctr = 0
		for timeslot in models.TimeSlot.select():
			self._timeslot_classes[timeslot] = set()
			self._timeslot_indexes[timeslot] = _timeslot_ctr

			timetable_timeslot = OrderedDict()
			for room in models.Room.select():
				timetable_timeslot[room] = set()

			self._timetable[timeslot] = timetable_timeslot

			_timeslot_ctr += 1

		self._classes = set()
		self._class_timeslots = dict()
		self._class_room = dict()

		self._used_timeslot_indexes = set()

	@property
	def timeslots(self):
		return self._timetable.items()

	@property
	def used_timeslot_indexes(self):
		return self._used_timeslot_indexes

	@property
	def classes(self):
		return self._classes

	def add_class(self, subject_class, starting_timeslot_idx, num_slots, room):
		app_logger.debug(
			f'Adding class {str(subject_class)} to {str(room)}...')

		# Assign class to room and the timeslots (number of timeslots needed
		# depends on the number of timeslots required by the class and the
		# day the starting timeslot is in.
		timeslots = list(self.timeslots)
		end_timeslot_idx = starting_timeslot_idx + (num_slots - 1)
		for i in range(starting_timeslot_idx, end_timeslot_idx + 1):
			self._used_timeslot_indexes.add(i)

			timeslot = timeslots[i][0]
			self._add_class_to_timeslot(subject_class, timeslot, room)

	def move_class_to_timeslot(self, subject_class, new_starting_timeslot_idx):
		app_logger.debug(f'Moving timeslot of {str(subject_class)}...')

		original_timeslots = self._class_timeslots[subject_class]
		num_session_slots = len(original_timeslots)
		if num_session_slots > 3:
			# Oh, so we assigned the class on a Wednesday. Divide! We're sure
			# that num_session_sltos will always be divisible by 2.
			num_session_slots //= 2

		# Let's check first if the new starting timeslot index is a valid one.
		starting_indexes = _utils._get_starting_timeslot_indexes(
			num_session_slots)
		if new_starting_timeslot_idx not in starting_indexes:
			raise InvalidStartingTimeslotIndex(
				f'Attempted to move class, {str(subject_class)}, to an invalid'
				f' starting timeslot index of {new_starting_timeslot_idx}.')

		# Reset the class's timeslots. Only the timeslots since we're moving
		# the class to a different timeslot, and not the room.
		room = self.get_class_room(subject_class)
		app_logger.debug(
			f'Room of class, {str(subject_class)}, is {str(room)}.')
		for timeslot in original_timeslots:
			self._timetable[timeslot][room].remove(subject_class)
			self._timeslot_classes[timeslot].remove(subject_class)

		del self._class_timeslots[subject_class]

		subject_class.timeslots.clear()

		# Now, change the timeslot of the class.
		if new_starting_timeslot_idx >= 48:
			# It's a Wednesday class now, so its slots must be twice.
			num_required_slots = num_session_slots * 2
		else:
			num_required_slots = num_session_slots

		self.add_class(subject_class,
					   new_starting_timeslot_idx,
					   num_required_slots,
					   room)

	def change_class_room(self, subject_class, new_room):
		original_timeslots = self._class_timeslots[subject_class]
		room = self._class_room[subject_class]
		for timeslot in original_timeslots:
			self._timetable[timeslot][room].remove(subject_class)
			self._timetable[timeslot][new_room].add(subject_class)

		self._class_room[subject_class] = new_room
		subject_class.room = new_room

	def get_classes_in_timeslot(self, timeslot):
		return self._timeslot_classes[timeslot]

	def get_class_at_timeslot_room(self, timeslot, room):
		return self._timetable[timeslot][room]

	def get_class_room(self, subject_class):
		return self._class_room.get(subject_class, None)

	def get_class_timeslots(self, subject_class):
		return self._class_timeslots.get(subject_class, [])

	def get_timeslot_index(self, timeslot):
		return self._timeslot_indexes[timeslot]

	def has_class_at_timeslot_room(self, timeslot, room):
		return self.get_class(timeslot, room) is not None

	def has_class(self, subject_class):
		return subject_class in self._classes

	def _add_class_to_timeslot(self, subject_class, timeslot, room):
		self._timetable[timeslot][room].add(subject_class)
		self._classes.add(subject_class)
		self._class_room[subject_class] = room

		if timeslot not in self._timeslot_classes:
			self._timeslot_classes[timeslot] = set()

		self._timeslot_classes[timeslot].add(subject_class)

		if subject_class not in self._class_timeslots:
			self._class_timeslots[subject_class] = list()

		self._class_timeslots[subject_class].append(timeslot)

		subject_class.timeslots.add(timeslot)
		subject_class.room = room
