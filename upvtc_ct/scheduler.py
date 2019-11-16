from peewee import fn, Select, JOIN

from upvtc_ct import models


def assign_classes_to_teachers():
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
		class_units = (Select(columns=[
							models.Class.id,
							models.Class.assigned_teacher.alias(
								'assigned_teacher'),
							models.Subject.units.alias('units')
						])
						.from_(models.Class, models.Subject)
						.where(models.Class.subject == models.Subject.id))
		teacher_units = (Select(columns=[
								models.Teacher.id.alias('assigned_teacher'),
								fn.COALESCE(
									fn.SUM(class_units.c.units),
									0).alias('units')
							])
							.from_(models.Teacher)
							.join(
								class_units,
								JOIN.LEFT_OUTER,
								on=(models.Teacher.id
									== class_units.c.assigned_teacher))
							.group_by(models.Teacher.id))
		print(teacher_units.sql())
		print('---')
		print(subject_class)
		for teacher in teacher_units.execute(models.db):
			print(teacher)
