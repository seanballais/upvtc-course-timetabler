from dataclasses import dataclass
import random

from peewee import fn, Select, JOIN

from upvtc_ct import models


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


def get_class_conflicts():
	class_conflicts = dict()
	class_capacity = dict()

	for study_plan in models.StudyPlan.select():
		students = [ _Student() for _ in range(study_plan.num_followers) ]

		for subject in study_plan.subjects:
			subject_classes = list(subject.classes)
			student_idx = 0

			for subject_class in subject_classes:
				class_capacity[str(subject_class)] = subject_class.capacity

				while (class_capacity[str(subject_class)] > 0
						and student_idx < study_plan.num_followers):
					students[student_idx].add_class(subject_class)

					student_idx += 1					
					class_capacity[str(subject_class)] -= 1

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
						if str(curr_class) not in class_conflicts:
							class_conflicts[str(curr_class)] = set()

						class_conflicts[str(curr_class)].add(
							student_class)

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
