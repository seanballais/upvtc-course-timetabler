from collections import OrderedDict


cached_class_conflicts = None


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


def _shuffle_slice(l, start, stop):
	# Obtained from https://stackoverflow.com/a/31416673/1116098.
	i = start
	while i < stop - i:
		idx = random.randrange(i, stop)
		l[i], l[idx] = l[idx], l[i]

		i += 1