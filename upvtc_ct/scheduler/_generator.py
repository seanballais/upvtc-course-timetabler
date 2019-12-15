import random

from ._ds import _Timetable
from ._utils import (
	get_class_conflicts, _get_starting_timeslot_indexes,
	_get_num_class_timeslots)


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
		room = _get_new_random_room_for_class(subject_class)

		start_timeslot = timeslots[start_timeslot_index][0]
		num_timeslots = _get_num_class_timeslots(subject_class, start_timeslot)
		end_timeslot_index = start_timeslot_index + num_timeslots - 1

		timetable.add_class(
			subject_class, start_timeslot_index, num_timeslots, room)

	return timetable


def _timetable_move1(timetable, subject_class):
	num_session_slots = len(timetable.get_class_timeslots(subject_class))
	if num_session_slots > 3:
		# Oh, so we assigned the class on a Wednesday. Divide! We're sure
		# that num_session_sltos will always be divisible by 2.
		num_session_slots /= 2

	starting_timeslot_indexes = _get_starting_timeslot_indexes(
		num_session_slots)
	new_starting_timeslot_index = random.choice(starting_timeslot_indexes)

	new_room = _get_new_random_room_for_class(subject_class)

	timetable.move_class_to_timeslot(
		subject_class, new_starting_timeslot_index)
	timetable.change_class_room(subject_class, new_room)


def _get_new_random_room_for_class(subject_class):
	return random.choice(
		_get_acceptable_rooms_for_subject(subject_class.subject))


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
