from collections import OrderedDict
import heapq
import logging

from peewee import fn, Select, JOIN

from upvtc_ct import models

from ._cost_computation import _compute_timetable_cost
from ._ds import _Student
from ._exceptions import UnschedulableException
from ._generator import _create_initial_timetable
from ._utils import _shuffle_slice, get_class_conflicts


app_logger = logging.getLogger()


def create_schedule(population_size=25):
	app_logger.info(':: Creating a new schedule...')
	app_logger.info('    Parameters:')
	app_logger.info(f'    - Population Size: {population_size}')

	reset_teacher_assignments()
	assign_teachers_to_classes()

	solutions = _create_initial_timetable_generation(population_size)

	# Start the genetic algorithm.

	# Permanently apply the assignments of the timetable with the best cost
	# to the database.
	best_timetable = heapq.heappop(solutions)[1]
	for c in best_timetable.classes:
		c.save()


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


def reset_teacher_assignments():
	for subject_class in models.Class.select():
		subject_class.assigned_teacher = None
		subject_class.save()


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


def _create_initial_timetable_generation(population_size):
	solutions = list()
	for i in range(population_size):
		app_logger.debug(f'Generating candidate timetable #{i + 1}...')
		
		timetable = _create_initial_timetable()
		timetable_cost = _compute_timetable_cost(timetable)

		# NOTE: heapq sorts ascendingly.
		heapq.heappush(solutions, (timetable_cost, timetable,))

	return solutions


def _create_new_timetable_generation(parent_timetables, population_size=25):
	pass


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
