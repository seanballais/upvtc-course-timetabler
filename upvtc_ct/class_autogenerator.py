import logging
import random
import string

from upvtc_ct import models


app_logger = logging.getLogger()


def autogenerate_data():
	app_logger.info('Generating timetable data...')

	app_logger.debug(':: Generating room features data...')
	room_features = _autogenerate_room_features()

	app_logger.debug(':: Generating division-related data...')

	division_data = {
		"Division of Natural Sciences and Mathematics": {
			"courses": [ 'BS Computer Science', 'BS Biology' ],
			"rooms": [
				'Room 11', 'Room 12', 'Room 15',
				'Physics Lab', 'Botany Lab', 'Chemistry Lab', 'Zoology Lab',
				'CS Lec Room 1', 'CS Lec Room 2', 'CS Lab 1', 'CS Lab 2',
				'CS Lab 3'
			],
		},
		"Division of Social Sciences": {
			"courses": [
				'BA Psychology', 'BA Economics', 'BA Political Science'
			],
			"rooms": [
				'Room 14', 'Room 21', 'Room 22', 'Room 23',
				'Psych Room', 'Psych Lab'
			]
		},
		"Division of Management": {
			"courses": [ 'BS Accountancy', 'BS Management' ],
			"rooms": [
				'DM Room 11', 'DM Room 12', 'DM Room 13', 'DM Room 14',
				'DM Room 15', 'DM Room 21', 'DM Room 22', 'DM Room 23',
				'DM Conference Room 1', 'DM Conference Room 2'
			]
		},
		"Division of Humanities": {
			"courses": [ 'BA Communication Arts' ],
			"rooms": [
				'Room 13', 'Room 24', 'Room 25', 'Humanities Lab', 'MPB',
				'DM Room 1', 'DM Room 2', 'DM Room 3'
			]
		}
	}
	for division_name, data in division_data.items():
		app_logger.debug(f'- Generating division, {division_name}...')
		division = models.Division()
		division.name = division_name
		division.save()

		app_logger.debug(f'-- Generating division courses...')
		for course_name in data['courses']:
			app_logger.debug(
				f'--- Generating course {course_name} for '
				f'division, {division_name}...')

			course = models.Course()
			course.name = course_name
			course.division = division
			course.save()

		app_logger.debug(f'-- Generating division rooms...')
		for room_name in data['rooms']:
			app_logger.debug(
				f'--- Generating room {room_name} for '
				f'division, {division_name}...')

			room = models.Room()
			room.name = room_name
			room.division = division
			room.save()

			# Add some features to rooms, so that it would match the real
			# world.
			if 'Room' in room_name or 'Lab' in room_name:
				# Okay, the room is just a regular room like the rooms in the
				# AS Building.
				feature = room_features['Projector']
				app_logger.debug(
					f'--- Adding feature {str(feature)} to room {room_name}...')

				room.features.add(feature)

	# Add features to specific rooms.
	app_logger.debug(':: Adding features to specific rooms...')

	# Add features to Room 12.
	room_12_feature = room_features['Air Conditioner']
	room_12 = models.Room.select().where(models.Room.name == 'Room 12').get()

	app_logger.debug(
		f'- Adding feature {str(room_12_feature)} to {str(room_12)}...')

	room_12.features.add(room_12_feature)
	room_12.save()

	# Add features to CS labs.
	cs_lab_feature = room_features['Computers']
	
	app_logger.debug(f'- Adding feature {str(cs_lab_feature)} to CS labs...')

	cs_labs = (models.Room
				.select()
				.where('CS Lab' in models.Room.name)
				.execute())
	for lab in cs_labs:
		app_logger.debug(f'-- Adding feature to lab {str(lab)}...')

		lab.features.add(cs_lab_feature)
		lab.save()

	# Add features to BS Biology labs.
	app_logger.debug(f'- Adding lab features to BS Biology labs...')

	physics_lab = (models.Room
					.select()
					.where(models.Room.name == 'Physics Lab')
					.get())
	botany_lab = (models.Room
					.select()
					.where(models.Room.name == 'Botany Lab')
					.get())
	zoology_lab = (models.Room
					.select()
					.where(models.Room.name == 'Zoology Lab')
					.get())
	chemistry_lab = (models.Room
						.select()
						.where(models.Room.name == 'Chemistry Lab')
						.get())
	bio_labs = {
		"Physics Lab": physics_lab,
		"Botany Lab": botany_lab,
		"Zoology Lab": zoology_lab,
		"Chemistry Lab": chemistry_lab
	}
	for lab_name, lab in bio_labs.items():
		feature = room_features[f'{lab_name} Equipment']

		app_logger.debug(
			f'-- Adding feature {str(feature)} to lab {str(lab)}...')

		lab.features.add(feature)
		lab.save()

	# Add features to the MPB.
	app_logger.debug(f'- Adding features to the MPB...')

	mpb_feature = room_features['Wide Space']

	mpb = models.Room.select().where(models.Room.name == 'MPB').get()
	
	app_logger.debug(f'-- Adding feature {str(mpb_feature)} to {str(mpb)}...')

	mpb.features.add(mpb_feature)
	mpb.save()

	# Autogenerate teachers.
	# Autogenerate subjects.
	# Autogenerate study plans.
	# Autogenerate classes.


def _autogenerate_room_features():
	features = [
		'Air Conditioner',
		'Chemistry Lab Equipment',
		'Physics Lab Equipment',
		'Botany Lab Equipment',
		'Zoology Lab Equipment',
		'Computers',
		'Projector',
		'Wide Space'
	]
	feature_objects = dict()
	for feature in features:
		app_logger.debug(f'- Generating room feature, {feature}.')

		room_feature = models.RoomFeature()
		room_feature.name = feature
		room_feature.save()

		feature_objects[feature] = room_feature

	return feature_objects
