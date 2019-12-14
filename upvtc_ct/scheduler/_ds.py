from collections import OrderedDict

from upvtc_ct import models


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
		for timeslot in models.TimeSlot.select():
			self._timeslot_classes[timeslot] = set()

			timetable_timeslot = OrderedDict()
			for room in models.Room.select():
				timetable_timeslot[room] = set()

			self._timetable[timeslot] = timetable_timeslot

		self._classes = set()
		self._class_timeslots = dict()
		self._class_room = dict()

	@property
	def timeslots(self):
		return self._timetable.items()

	@property
	def classes(self):
		return self._classes
	
	def get_classes_in_timeslot(self, timeslot):
		return self._timeslot_classes[timeslot]

	def add_class(self, subject_class, starting_timeslot_idx, num_slots, room):
		# Assign class to room and the timeslots (number of timeslots needed
		# depends on the number of timeslots required by the class and the
		# day the starting timeslot is in.
		timeslots = list(self.timeslots)
		end_timeslot_idx = starting_timeslot_idx + (num_slots - 1)
		for i in range(starting_timeslot_idx, end_timeslot_idx + 1):
			timeslot = timeslots[i][0]
			self._add_class_to_timeslot(subject_class, timeslot, room)

	def get_class_at_timeslot_room(self, timeslot, room):
		return self._timetable[timeslot][room]

	def has_class_at_timeslot_room(self, timeslot, room):
		return self.get_class(timeslot, room) is not None

	def has_class(self, subject_class):
		return subject_class in self._classes

	def get_class_room(self, subject_class):
		return self._class_room.get(subject_class, None)

	def get_class_timeslots(self, subject_class):
		return self._class_timeslots.get(subject_class, [])

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
