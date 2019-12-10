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
		timetable = OrderedDict()
		self._timeslot_classes = dict()
		for timeslot in models.TimeSlot.select():
			self._timeslot_classes[timeslot] = set()

			timetable_timeslot = OrderedDict()
			for room in models.Room.select():
				timetable_timeslot[room] = set()

			timetable[timeslot] = timetable_timeslot

		self._timetable = timetable

		self._classes = set()
		self._class_timeslots = dict()
		self._class_room = dict()

	@property
	def timeslots(self):
		return self._timetable.items()

	def get_classes_in_timeslot(self, timeslot):
		return self._timeslot_classes[timeslot]

	def add_class_to_timeslot(self, subject_class, timeslot, room):
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
		# !!! TEMPORARY !!! WE SHOULD NOT SAVE TO THE DB AT THIS POINT.
		subject_class.save()

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
