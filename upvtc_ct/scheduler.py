from peewee import fn, Select, JOIN

from upvtc_ct import models


def assign_teachers_to_classes():
	# Classes will tend to be assigned to teachers with fewer units. This is to
	# reduce the chances of teachers being overloaded. Additionally, subjects
	# with fewer number of candidate teachers will have their classes assigned
	# teachers first to assign the class a teacher who is not likely overloaded
	# from other subjects.
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
	for subject_class in sorted_classes:
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

		# Let's compile the rows of teacher units into an easier to use
		# data structure.
		teacher_units = dict()
		for teacher in teacher_units_q.execute(models.db):
			teacher_units[teacher['assigned_teacher']] = teacher['units']

		# Assign teachers to classes now.
		candidate_teachers = list(subject_class.subject.candidate_teachers)
		candidate_teachers.sort(key=lambda t : teacher_units[t.id])
		subject_class.assigned_teacher = candidate_teachers[0]
		subject_class.save()
