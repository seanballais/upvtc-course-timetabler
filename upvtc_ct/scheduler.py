from peewee import fn

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
					subject_teachers.subject.alias('teachers'),
					num_candidate_teachers.alias('num_candidate_teachers'))
				.group_by(subject_teachers.subject))
	sorted_classes = (models.Class
						.select(models.Class)
						.join(
							query,
							on=(models.Class.subject == query.c.teachers))
						.order_by(query.c.num_candidate_teachers.asc()))
	for subject_class in sorted_classes:
		print(subject_class)
