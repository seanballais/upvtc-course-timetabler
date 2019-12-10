from upvtc_ct import models

from ._scheduler import get_class_conflicts

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
