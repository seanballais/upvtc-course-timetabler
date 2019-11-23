from collections import OrderedDict
from dataclasses import dataclass
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
		unpreferrable_timeslots = list()

		# 7AM - 8AM and 5:30PM - 7:00PM timeslots are unpreferrable.
		unpreferrable_timeslot_indexes = set([
			 0,  1, 21, 22, 23,  # Day 0 unpreferrable timeslots.
			24, 25, 45, 46, 47,  # Day 1 unpreferrable timeslots.
			48, 49, 69, 70, 71,  # Day 2 unpreferrable timeslots.
			 9, 10, 11,          # Day 0 lunch time (and unpreferrable).
			33, 34, 35,          # Day 1 lunch time (and unpreferrable).
			57, 58, 59 			 # Day 2 lunch time (and unpreferrable).
		])

		timetable = OrderedDict()
		timeslot_index = 0
		for timeslot in models.TimeSlot.select():
			timetable_timeslot = OrderedDict()
			for room in models.Room.select():
				timetable_timeslot[room] = None

			timetable[timeslot] = timetable_timeslot

			# Let's mark timeslots that are unpreferrable so that we know
			# which timeslots are unpreferrable.		
			if timeslot_index in unpreferrable_timeslot_indexes:
				unpreferrable_timeslots.append(timeslot)

			timeslot_index += 1

		# Move the first and last one hour periods of the timetable to the end
		# so that it would be easier for us to schedule classes to more
		# desirable timeslots.
		for timeslot in unpreferrable_timeslots:
			timetable.move_to_end(timeslot)

		self._timetable = timetable
		self._timeslots = self._timetable.items()

		self._timeslot_classes = dict()

	@property
	def timeslots(self):
		return self._timeslots

	def get_classes_in_timeslot(self, timeslot):
		if timeslot not in self._timeslot_classes:
			self._timeslot_classes[timeslot] = set()

		return self._timeslot_classes[timeslot]

	def add_class_to_timeslot(self, subject_class, timeslot, room):
		self._timetable[timeslot][room] = subject_class

		if timeslot not in self._timeslot_classes:
			self._timeslot_classes[timeslot] = set()

		self._timeslot_classes[timeslot].add(subject_class)

		# !!! TEMPORARY !!! WE SHOULD NOT SAVE TO THE DB AT THIS POINT.
		subject_class.timeslots.add(timeslot)
		subject_class.room = room
		subject_class.save()

	def get_class(self, timeslot, room):
		return self._timetable[timeslot][room]

	def has_class(self, timeslot, room):
		return self.get_class(timeslot, room) is not None


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


def create_schedule():
	timetable = _create_initial_timetable()


def _create_initial_timetable():
	timetable = _Timetable()
	class_conflicts = get_class_conflicts()

	for subject_class, conflicting_classes in class_conflicts.items():
		room_requirements = set(subject_class.subject.required_features)

		remaining_timeslot_jumps = 0
		room_assignment = None
		timeslots = list(timetable.timeslots)
		for timeslot_index, timeslot_item in enumerate(timeslots):
			timeslot = timeslot_item[0]

			if subject_class.subject.is_wednesday_class and timeslot.day != 2:
				# Wednesday-only classes should only be scheduled on
				# Wednesdays, so let's skip the current timeslot until we are
				# in a Wednesday timeslot.
				continue

			if remaining_timeslot_jumps > 0:
				# We should still be moving to the next period.
				if room_assignment is not None:
					# This means we are assigning timeslots to classes.
					timetable.add_class_to_timeslot(
						subject_class, timeslot, room)

				remaining_timeslot_jumps -= 1
				continue
			else:
				if room_assignment is not None:
					# We just finished assigning a room to a class, so let's
					# schedule the next class.
					break

			# Don't schedule 1.5hr classes at 8AM. So, skip if we're at 8AM
			# and scheduling a 1.5hr class.
			if (subject_class.subject.num_required_timeslots > 2
					and timeslot.start_time == '08:00:00'):
				continue

			# Check if a conflicting class has been scheduled in the same
			# timeslot.
			timeslot_classes = timetable.get_classes_in_timeslot(timeslot)
			if not conflicting_classes.isdisjoint(timeslot_classes):
				# Let's move to the next period, which is two timeslots away,
				# since we have a conflicting class in the current timeslot.
				remaining_timeslot_jumps = _get_class_timeslot_jumps(
					subject_class, timeslot)
				continue

			# Make sure that the rooms with lesser features are used more by
			# subjects that don't require a lot of features.
			division_rooms = list(subject_class
									.subject
									.division
									.rooms
									.select())
			division_rooms.sort(key=lambda r : len(r.features))
			# Okay. No conflicting classes so let's look for a room.
			for room in division_rooms:
				# Check if the room has the required features the subject
				# needs.
				room_features = set(room.features)
				if not room_requirements.issubset(room_features):
					# Room does not have the features the subject requires. So,
					# find a new room.
					continue

				# Show rooms that match the subject class's room requirements.
				if not timetable.has_class(timeslot, room):
					period_length = _get_class_timeslot_jumps(
						subject_class, timeslot)

					if _are_item_timeslots_free(
							subject_class,
							conflicting_classes,
							timeslot_index + 1,
							period_length,
							timetable,
							room):
						# Let's start assigning timeslots to classes and
						# jumping timeslots, since the timeslots in the period
						# for the class are free.
						room_assignment = room

						timetable.add_class_to_timeslot(
							subject_class, timeslot, room)

					# If the period timeslots are not free, then let's just
					# jump to the next period for the subject.

					remaining_timeslot_jumps = period_length

					break


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


def _get_class_timeslot_jumps(subject_class, timeslot):
	subject = subject_class.subject
	num_jumps = subject.num_required_timeslots - 1

	if timeslot.day == 2:
		num_jumps = (num_jumps * 2) + 1  # Plus 1 since we need to do N - 1
										 # jumps. Not adding 1 means that we
										 # will miss one required jump.

	return num_jumps


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
