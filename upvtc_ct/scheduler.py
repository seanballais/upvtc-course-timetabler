from collections import OrderedDict
from dataclasses import dataclass
from numba import cuda
import random

from peewee import fn, Select, JOIN

from upvtc_ct import models


cached_class_conflicts = None


class UnschedulableException(Exception):
	pass


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

	@property
	def classes(self):
		return self._classes
	

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


def reset_schedule():
	models.Class.timeslots.get_through_model().delete().execute(models.db)

	for subject_class in models.Class.select():
		subject_class.room = None
		subject_class.assigned_teacher = None
		subject_class.save()


def view_text_form_class_conflicts():
	try:
		class_conflicts = get_class_conflicts()
	except UnschedulableException as e:
		app_logger.error(
			 'Oh no! It is impossible to create an feasible schedule'
			f' because {e}')

		return

	print('-' * 60)
	print(f'| {"CLASS CONFLICTS":57}|')
	print('-' * 60)
	print(f'| :: {"Class":16}| :: {"Conflicting Classes":33}|')
	print('-' * 60)
	for subject_class, conflicting_classes in class_conflicts.items():
		classes = [
			f'{str(c)}' for c in conflicting_classes
		]
		print(f'| {str(subject_class):19}| {", ".join(classes):36}|')
		print('-' * 60)


def view_text_form_schedule():
	print('-' * 64)
	for timeslot in models.TimeSlot.select():
		timeslot_classes = [
			f'[{str(c.room)}] {str(c)}' for c in timeslot.classes
		]
		print(
			f'{str(timeslot):31} | {", ".join(timeslot_classes)}')
		print('-' * 64)


def create_schedule(use_gpu=False):
	reset_teacher_assignments()
	assign_teachers_to_classes()

	timetable = _create_initial_timetable()

	if use_gpu:
		print(_gpu_compute_timetable_cost(timetable))
	else:
		print(_compute_timetable_cost(timetable))


def get_class_conflicts(invalid_cache=False):
	global cached_class_conflicts

	if cached_class_conflicts is not None:
		return cached_class_conflicts

	class_conflicts = OrderedDict()
	class_capacity = dict()

	for study_plan in models.StudyPlan.select():
		students = [ _Student() for _ in range(study_plan.num_followers) ]

		for subject in study_plan.subjects:
			subject_classes = list(subject.classes)
			student_idx = 0

			for subject_class in subject_classes:
				class_capacity[subject_class] = subject_class.capacity

				while (class_capacity[subject_class] > 0
						and student_idx < study_plan.num_followers):
					students[student_idx].add_class(subject_class)

					student_idx += 1					
					class_capacity[subject_class] -= 1

			if student_idx != study_plan.num_followers:
				raise UnschedulableException(
					f'{str(subject)} does not have enough classes to handle'
					 ' all the students that needs to take it up.')

		# Compute now for the conflict weights.
		# Oh god, this code block disgusts me, and I'm the one who wrote it.
		# Got a deadline so better have something than coming up with a
		# more optimal but run out of time while doing so.
		# - Sean Ballais
		for student in students:
			for curr_class in student.classes:
				for student_class in student.classes:
					if student_class is not curr_class:
						if curr_class not in class_conflicts:
							class_conflicts[curr_class] = set()

						class_conflicts[curr_class].add(student_class)

	# Sort the class conflicts in such a way that the class with the most
	# number of conflicts get to be the first item in the dictionary.
	class_conflicts = OrderedDict(
		sorted(class_conflicts.items(), key=lambda c: len(c[1])))

	cached_class_conflicts = class_conflicts

	return class_conflicts


def reset_teacher_assignments():
	for subject_class in models.Class.select():
		subject_class.assigned_teacher = None
		subject_class.save()


def assign_teachers_to_classes():
	# Classes will tend to be assigned to teachers with fewer units. This is to
	# reduce the chances of teachers being overloaded. Additionally, subjects
	# with fewer number of candidate teachers will have their classes assigned
	# teachers first to assign the class a teacher who is not likely overloaded
	# from other subjects.
	for subject_class in _get_sorted_classes_query():
		# Let's compile the rows of teacher units into an easier to use
		# data structure.
		teacher_units = dict()
		for teacher in _get_teacher_units_query().execute(models.db):
			teacher_units[teacher['assigned_teacher']] = teacher['units']

		# Assign teachers to classes now.
		candidate_teachers = list(subject_class.subject.candidate_teachers)
		candidate_teachers.sort(key=lambda t : teacher_units[t.id])
		_shuffle_teachers_with_same_units(candidate_teachers, teacher_units)

		subject_class.assigned_teacher = candidate_teachers[0]
		subject_class.save()


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


def _compute_timetable_cost(timetable):
	cost = 0
	hc_penalty = 10000
	sc_penalty = 1
	class_conflicts = get_class_conflicts()

	# Compute penalty for hard constraints (HCs).
	cost += _compute_hc1_constraint(timetable, hc_penalty)
	cost += _compute_hc2_constraint(timetable, hc_penalty)
	cost += _compute_hc3_constraint(timetable, hc_penalty)
	cost += _compute_hc4_constraint(timetable, hc_penalty)
	cost += _compute_hc5_constraint(timetable, hc_penalty)
	cost += _compute_hc6_constraint(timetable, hc_penalty)
	print(cost)
	cost += _compute_hc7_constraint(timetable, hc_penalty)

	# Compute penalty for soft constraints (SCs).
	cost += _compute_sc1_constraint(timetable, sc_penalty)
	cost += _compute_sc2_constraint(timetable, sc_penalty)
	cost += _compute_sc3_constraint(timetable, sc_penalty)

	return cost


def _gpu_compute_timetable_cost(timetable):
	cost = 0
	hc_penalty = 10000
	sc_penalty = 1
	class_conflicts = get_class_conflicts()

	# Compute penalty for hard constraints (HCs).
	cost += _compute_hc1_constraint(timetable, hc_penalty)
	cost += _compute_hc2_constraint(timetable, hc_penalty)
	cost += _gpu_compute_hc3_constraint(timetable, hc_penalty)
	cost += _compute_hc4_constraint(timetable, hc_penalty)
	cost += _compute_hc5_constraint(timetable, hc_penalty)
	cost += _compute_hc6_constraint(timetable, hc_penalty)
	print(cost)
	cost += _compute_hc7_constraint(timetable, hc_penalty)

	# Compute penalty for soft constraints (SCs).
	cost += _compute_sc1_constraint(timetable, sc_penalty)
	cost += _compute_sc2_constraint(timetable, sc_penalty)
	cost += _compute_sc3_constraint(timetable, sc_penalty)

	return cost


def _compute_hc1_constraint(timetable, hc_penalty):
	cost = 0
	class_conflicts = get_class_conflicts()

	for timeslot, rooms in timetable.timeslots:
		# Check for student conflicts.
		timeslot_classes = timetable.get_classes_in_timeslot(timeslot)
		for subject_class in timeslot_classes:
			conflicting_classes = class_conflicts[subject_class]
			if not conflicting_classes.isdisjoint(timeslot_classes):
				# Oh no. We have a conflicting class here.
				cost += hc_penalty

		# Check for teacher conflicts.
		num_teacher_assignments = dict()
		for subject_class in timeslot_classes:
			if subject_class.assigned_teacher not in num_teacher_assignments:
				num_teacher_assignments[subject_class.assigned_teacher] = 1
			else:
				num_teacher_assignments[subject_class.assigned_teacher] += 1

		penalty_condition = lambda n: hc_penalty * n if n > 1 else 0
		penalty = sum(list(map(
			penalty_condition, num_teacher_assignments.values())))
		cost += penalty

	return cost


def _compute_hc2_constraint(timetable, hc_penalty):
	cost = 0
	for _, rooms in timetable.timeslots:
		# If two or more classes have been scheduled to the same
		# room and timeslot, then apply a penalty.
		penalty_condition = lambda c: hc_penalty if len(c) > 1 else 0
		penalty = sum(list(map(penalty_condition, rooms.values())))
		cost += penalty

	return cost


def _compute_hc3_constraint(timetable, hc_penalty):
	# Not yet checking for room since, for now, being scheduled a timeslot
	# would also mean being scheduled a room. TBA rooms not yet considered.
	classes = models.Class.select()
	return sum(list(map(
		lambda c: hc_penalty if not timetable.has_class(c) else 0, classes)))


def _compute_hc4_constraint(timetable, hc_penalty):
	cost = 0
	classes = models.Class.select()
	for subject_class in classes:
		num_required_timeslots = subject_class.subject.num_required_timeslots
		timeslots = timetable.get_class_timeslots(subject_class)
		if (timeslots[0].day == 2
				and len(timeslots) != num_required_timeslots * 2):
			cost += hc_penalty

	return cost


def _compute_hc5_constraint(timetable, hc_penalty):
	cost = 0
	classes = models.Class.select()
	for subject_class in classes:
		room = timetable.get_class_room(subject_class)
		room_features = set(room.features)
		room_requirements = set(subject_class.subject.required_features)
		if not room_requirements.issubset(room_features):
			# Room does not have the features the subject requires.
			cost += hc_penalty

	return cost


def _compute_hc6_constraint(timetable, hc_penalty):
	cost = 0
	classes = models.Class.select()
	for subject_class in classes:
		num_required_timeslots = subject_class.subject.num_required_timeslots

		# Note that the classes are given time slots that are in the same day.
		# So, we only need to check the first timeslot to confirm whether or
		# not the hard constraint is violated.
		timeslots = timetable.get_class_timeslots(subject_class)
		if (subject_class.subject.is_wednesday_class
				and timeslots[0].day != 2):
			cost += hc_penalty

	return cost


def _compute_hc7_constraint(timetable, hc_penalty):
	cost = 0

	for subject_class in models.Class.select():
		timeslots = timetable.get_class_timeslots(subject_class)
		# Make sure that the end time and start time of two adjacent
		# timeslots respectively must be the same. There is a penalty
		# for every timeslot if it is not.
		for i in range(len(timeslots) - 1):
			t1 = timeslots[i]
			t2 = timeslots[i + 1]
			if t1.end_time != t2.start_time:
				# The timeslots are disconnected and there is a hole
				# in between.
				cost += hc_penalty

	return cost


def _compute_sc1_constraint(timetable, sc_penalty):
	cost = 0
	for _, rooms in timetable.timeslots:
		for room, classes in rooms.items():
			for subject_class in classes:
				if subject_class.subject.division != room.division:
					# Class is scheduled in a room that is not under the
					# division the class is offered by.
					cost += sc_penalty

	return cost


def _compute_sc2_constraint(timetable, sc_penalty):
	cost = 0
	unpreferred_timeslots_indexes = set([
		9, 10, 11,          # Day 0 lunch time (and unpreferrable).
		33, 34, 35,          # Day 1 lunch time (and unpreferrable).
		57, 58, 59,			 # Day 2 lunch time (and unpreferrable).
		0,  1,  			 # Day 0 unpreferrable morning timeslots.
		24, 25,              # Day 1 unpreferrable morning timeslots.
		48, 49,              # Day 2 unpreferrable morning timeslots.
		21, 22, 23,          # Day 0 unpreferrable evening timeslots.
		45, 46, 47,			 # Day 1 unpreferrable evening timeslots.
		69, 70, 71			 # Day 2 unpreferrable evening timeslots.
	])
	index = 0
	for timeslot, _ in timetable.timeslots:
		if index in unpreferred_timeslots_indexes:
			# If there are classes scheduled in the unpreferrable timeslot,
			# increase the cost by the soft constraint penalty cost for each
			# class.
			classes = timetable.get_classes_in_timeslot(timeslot)
			cost += len(classes) * sc_penalty

		index += 1

	return cost


def _compute_sc3_constraint(timetable, sc_penalty):
	cost = 0
	unpreferred_timeslots = dict()

	for teacher in models.Teacher.select():
		unpreferred_timeslots[teacher] = set(teacher.unpreferred_timeslots)

	for timeslot, _ in timetable.timeslots:
		classes = timetable.get_classes_in_timeslot(timeslot)
		for subject_class in classes:
			teacher = subject_class.assigned_teacher
			if timeslot in unpreferred_timeslots[teacher]:
				cost += sc_penalty

	return cost


def _gpu_compute_hc3_constraint(timetable, hc_penalty):
	# Not yet checking for room since, for now, being scheduled a timeslot
	# would also mean being scheduled a room. TBA rooms not yet considered.
	timetable_classes = set()
	for c in timetable.classes:
		timetable_classes.add(id(c))

	classes = set(map(lambda c: id(c), list(models.Class.select())))

	cost = [ 0 ]

	_gpu_compute_hc3_constraint_kernel(
		timetable_classes, classes, hc_penalty, cost)
	return cost[0]


@cuda.jit
def _gpu_compute_hc3_constraint_kernel(timetable_classes,
    								   classes,
									   hc_penalty,
									   cost):
	# Not yet checking for room since, for now, being scheduled a timeslot
	# would also mean being scheduled a room. TBA rooms not yet considered.
	cost[0] = 0
	for c in classes:
		if not c in timetable_classes:
			cost[0] += hc_penalty


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


def _are_item_timeslots_free(subject_class,
							 conflicting_classes,
							 starting_index,
							 period_length,
							 timetable,
							 room):
	timeslots = list(timetable.timeslots)
	for i in range(starting_index, starting_index + period_length):
		try:
			timeslot = timeslots[i][0]
		except IndexError:
			return False

		# Check if a conflicting class has been scheduled in the same
		# timeslot.
		timeslot_classes = timetable.get_classes_in_timeslot(timeslot)
		if (not conflicting_classes.isdisjoint(timeslot_classes)
				or timetable.has_class(timeslot, room)):
			# We have a conflicting class in the current timeslot.
			return False

	return True


def _get_num_class_timeslots(subject_class, starting_timeslot):
	subject = subject_class.subject
	num_jumps = subject.num_required_timeslots

	if starting_timeslot.day == 2:
		num_jumps = num_jumps * 2

	return num_jumps


def _shuffle_teachers_with_same_units(candidate_teachers, teacher_units):
	curr_units = None
	curr_start = 0
	must_shuffle = False
	index = 0
	while index < len(candidate_teachers) - 1:
		teacher = candidate_teachers[index]
		if curr_units != teacher_units[teacher.id]:
			curr_units = teacher_units[teacher.id]

			if must_shuffle:
				# Note: randrange() in _shuffle_slice() is exclusive.
				_shuffle_slice(candidate_teachers, curr_start, index)
			else:
				# First time going through the list so let's not shuffle yet
				# but let it be known that later changes in the teacher units
				# should make us shuffle the appropriate sublist.
				must_shuffle = True

			curr_start = index

		index += 1


def _get_sorted_classes_query():
	# Sort classes based on the number of candidate teachers a subject
	# has ascendingly.
	subject_teachers = models.Subject.candidate_teachers.get_through_model()
	num_candidate_teachers = fn.COUNT(subject_teachers.teacher)
	query = (subject_teachers
				.select(
					subject_teachers.subject.alias('subject'),
					num_candidate_teachers.alias('num_candidate_teachers'))
				.group_by(subject_teachers.subject))
	sorted_classes = (models.Class
						.select(models.Class)
						.join(
							query,
							on=(models.Class.subject == query.c.subject))
						.order_by(query.c.num_candidate_teachers.asc()))
	return sorted_classes


def _get_teacher_units_query():
	class_units_q = (Select(columns=[
							models.Class.id,
							models.Class.assigned_teacher.alias(
								'assigned_teacher'),
							models.Subject.units.alias('units')
						])
						.from_(models.Class, models.Subject)
						.where(models.Class.subject == models.Subject.id))
	teacher_units_q = (Select(columns=[
							models.Teacher.id.alias('assigned_teacher'),
							fn.COALESCE( 
								fn.SUM(class_units_q.c.units),
								0).alias('units')
							])
						.from_(models.Teacher)
						.join(
							class_units_q,
							JOIN.LEFT_OUTER,
							on=(models.Teacher.id
									== class_units_q.c.assigned_teacher))
						.group_by(models.Teacher.id))
	return teacher_units_q


def _shuffle_slice(l, start, stop):
	# Obtained from https://stackoverflow.com/a/31416673/1116098.
	i = start
	while i < stop - i:
		idx = random.randrange(i, stop)
		l[i], l[idx] = l[idx], l[i]

		i += 1
