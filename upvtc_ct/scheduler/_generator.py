import random

from ._ds import _Timetable
from ._scheduler import get_class_conflicts


def _create_initial_timetable():
	timetable = _Timetable()
	timeslots = list(timetable.timeslots)
	indexed_timeslots = enumerate(list(timetable.timeslots))
	class_conflicts = get_class_conflicts()

	# The last few (depending on the number of timeslots required
	# by a class) timeslots should not be used as starting timeslot as it would
	# cause the class to be cut and divided between two days or be incompletely
	# scheduled, which shouldn't happen..
	one_hr_classes_starting_timeslot_indexes = _get_starting_timeslot_indexes(
		2)
	typical_classes_starting_timeslot_indexes = _get_starting_timeslot_indexes(
		3)

	for subject_class, conflicting_classes in class_conflicts.items():
		timeslot_indexes = None
		if subject_class.subject.num_required_timeslots == 2:
			# It's a one hour class.
			start_timeslot_indexes = one_hr_classes_starting_timeslot_indexes
		else:
			# It's a 1.5 hour class.
			start_timeslot_indexes = typical_classes_starting_timeslot_indexes

		start_timeslot_index = random.choice(start_timeslot_indexes)

		# Randomly select a room.
		room = random.choice(
			_get_acceptable_rooms_for_subject(subject_class.subject))

		# Assign class to room and the timeslots (number of timeslots needed
		# depends on the number of timeslots required by the class and the
		# day the starting timeslot is in.
		start_timeslot = timeslots[start_timeslot_index][0]
		num_timeslots = _get_num_class_timeslots(subject_class, start_timeslot)
		end_timeslot_index = start_timeslot_index + num_timeslots - 1

		for i in range(start_timeslot_index, end_timeslot_index + 1):
			timeslot = timeslots[i][0]
			timetable.add_class_to_timeslot(subject_class, timeslot, room)

	return timetable


def _timetable_move1(timetable):
	pass


def _get_starting_timeslot_indexes(num_required_timeslots):
	starting_timeslot_indexes = list(
		range(0, 46, num_required_timeslots))

	end_timeslot_index = 0
	if num_required_timeslots == 2:
		# The last starting timeslot should be 5:00PM - 5:30PM.
		end_timeslot_index = 68
	else:
		# The last starting timeslot should be 4:00PM - 4:30PM.
		end_timeslot_index = 66

	starting_timeslot_indexes.extend(
		list(range(48, end_timeslot_index + 1, num_required_timeslots)))

	return starting_timeslot_indexes


def _get_acceptable_rooms_for_subject(subject):
	acceptable_rooms = list()
	room_requirements = set(subject.required_features)

	division_rooms = list(subject.division.rooms.select())
	division_rooms.sort(key=lambda r : len(r.features))
	for room in division_rooms:
		# Check if the room has the required features the subject
		# needs.
		room_features = set(room.features)
		if room_requirements.issubset(room_features):
			# Room have the features the subject requires, so
			# it is an acceptable_room.
			acceptable_rooms.append(room)

	return acceptable_rooms


def _get_num_class_timeslots(subject_class, starting_timeslot):
	subject = subject_class.subject
	num_jumps = subject.num_required_timeslots

	if starting_timeslot.day == 2:
		num_jumps = num_jumps * 2

	return num_jumps
