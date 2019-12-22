import copy
import heapq
import random
import logging

from upvtc_ct import settings

from ._cost_computation import _compute_timetable_cost
from ._ds import _Timetable
from ._utils import (
	get_class_conflicts, _get_num_class_timeslots,
	_get_starting_timeslot_indexes)


app_logger = logging.getLogger()


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
		# We need to copy subject_class since we don't want different
		# timetables to share the same instance of a class.
		subject_class = copy.deepcopy(subject_class)

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


def _create_initial_timetable_generation(population_size=25):
	solutions = list()
	for i in range(population_size):
		app_logger.info(f'Generating candidate timetable #{i + 1}...')
		
		timetable = _create_initial_timetable()
		timetable_cost = _compute_timetable_cost(timetable)

		app_logger.info(
			f'- Candidate timetable #{i + 1} has cost: {timetable_cost}')

		# NOTE: heapq sorts ascendingly.
		heapq.heappush(solutions, (timetable_cost, id(timetable), timetable,))

	return solutions


def _create_new_timetable_generation(parent1,
									 parent2,
									 population_size=25,
									 mutation_chance=0.2):
	new_solutions = list()
	for i in range(population_size):
		app_logger.info(f'Generating candidate timetable #{i + 1}...')

		timetable = _create_offspring_timetable(
			parent1, parent2, mutation_chance)
		timetable_cost = _compute_timetable_cost(timetable)

		app_logger.info(
			f'- Candidate timetable #{i + 1} has cost: {timetable_cost}')

		# NOTE: heapq sorts ascendingly.
		heapq.heappush(
			new_solutions, (timetable_cost, id(timetable), timetable,))

	return new_solutions


def _get_new_random_room_for_class(subject_class):
	return random.choice(
		_get_acceptable_rooms_for_subject(subject_class.subject))


def _timetable_move1(timetable):
	subject_class = random.choice(list(timetable.classes))
	num_session_slots = len(timetable.get_class_timeslots(subject_class))
	if num_session_slots > 3:
		# Oh, so we assigned the class on a Wednesday. Divide! We're sure
		# that num_session_sltos will always be divisible by 2.
		num_session_slots //= 2

	starting_timeslot_indexes = _get_starting_timeslot_indexes(
		num_session_slots)
	new_starting_timeslot_index = random.choice(starting_timeslot_indexes)

	new_room = _get_new_random_room_for_class(subject_class)

	timetable.move_class_to_timeslot(
		subject_class, new_starting_timeslot_index)
	timetable.change_class_room(subject_class, new_room)


def _timetable_move2(timetable):
	classes = list(timetable.classes)
	class1 = random.choice(classes)
	class2 = random.choice(classes)

	# Not swapping the rooms since the room of one of the classes may not be
	# sufficient for the other class. So, we just swap the timeslots.
	class1_timeslots = timetable.get_class_timeslots(class1)
	class2_timeslots = timetable.get_class_timeslots(class2)

	class1_starting_timeslot_index = timetable.get_timeslot_index(
		class1_timeslots[0])
	class2_starting_timeslot_index = timetable.get_timeslot_index(
		class2_timeslots[0])

	# Swapping time!
	timetable.move_class_to_timeslot(class1, class2_starting_timeslot_index)
	timetable.move_class_to_timeslot(class2, class1_starting_timeslot_index)


def _create_offspring_timetable(timetable1, timetable2, mutation_chance=0.2):
	base_parent = random.choice([ timetable1, timetable2 ])
	offspring = copy.deepcopy(base_parent)

	auxiliary_parent = timetable1 if base_parent == timetable2 else timetable2

	crossovered_class = random.choice(list(auxiliary_parent.classes))
	class_timeslots = list(
		auxiliary_parent.get_class_timeslots(crossovered_class))
	class_starting_timeslot_index = auxiliary_parent.get_timeslot_index(
		class_timeslots[0])

	# Non-optimal. Eek! Looks for the class to be crossovered to the offspring
	# in the offspring.
	for c in offspring.classes:
		if str(c) == str(crossovered_class):
			target_offspring_class = c
			break
	else:
		target_offspring_class = None

	offspring.move_class_to_timeslot(
		target_offspring_class, class_starting_timeslot_index)

	# Let's mutate the offspring if the chance is right.
	if random.random() < mutation_chance:
		_mutate_timetable(offspring)

	return offspring


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


def _mutate_timetable(timetable):
	move = random.choice([ _timetable_move1, _timetable_move1 ])
	move(timetable)
