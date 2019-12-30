import logging
import random
import string

from upvtc_ct import models


app_logger = logging.getLogger()


def autogenerate_data(num_divisions):
	app_logger.info('Generating timetable data...')

	app_logger.info('Generating divisions...')
	_autogenerate_divisions(num_divisions)
	# Autogenerate courses.
	# Autogenerate room features.
	# Autogenerate rooms.
	# Autogenerate teachers.
	# Autogenerate subjects.
	# Autogenerate study plans.
	# Autogenerate classes.


def _autogenerate_divisions(num_divisions):
	for _ in range(num_divisions):
		d = models.Division()

		name_length = random.randint(2, 5)
		d.name = ''.join([
			random.choice(string.ascii_uppercase) for _  in range(name_length)
		])

		app_logger.debug(f'- Generated division, {division_name}.')
