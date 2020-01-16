import logging
import math
import random
import string
import threading

import requests

from upvtc_ct import models, settings


app_logger = logging.getLogger()


def autogenerate_data():
	app_logger.info('Generating timetable data...')

	division_data = {
		"Division of Natural Sciences and Mathematics": {
			"courses": [ 'BS Computer Science', 'BS Biology' ],
			"rooms": [
				'Room 11', 'Room 12', 'Room 15',
				'Physics Lab', 'Botany Lab', 'Chemistry Lab', 'Zoology Lab',
				'CS Lec Room 1', 'CS Lec Room 2', 'CS Lab 1', 'CS Lab 2',
				'CS Lab 3'
			],
			"num_teachers": 20,
		},
		"Division of Social Sciences": {
			"courses": [
				'BA Psychology', 'BA Economics', 'BA Political Science'
			],
			"rooms": [
				'Room 14', 'Room 21', 'Room 22', 'Room 23',
				'Psych Room', 'Psych Lab'
			],
			"num_teachers": 16,
		},
		"Division of Management": {
			"courses": [ 'BS Accountancy', 'BS Management' ],
			"rooms": [
				'DM Room 11', 'DM Room 12', 'DM Room 13', 'DM Room 14',
				'DM Room 15', 'DM Room 21', 'DM Room 22', 'DM Room 23',
				'DM Conference Room 1', 'DM Conference Room 2'
			],
			"num_teachers": 15,
		},
		"Division of Humanities": {
			"courses": [ 'BA Communication Arts' ],
			"rooms": [
				'Room 13', 'Room 24', 'Room 25', 'Humanities Lab', 'MPB',
				'DM Room 1', 'DM Room 2', 'DM Room 3'
			],
			"num_teachers": 20,
		}
	}

	app_logger.debug(':: Generating room features data...')

	_autogenerate_room_features()
	room_features = _get_room_features()

	app_logger.debug(':: Generating division-related data...')

	_autogenerate_division_related_data(division_data, room_features)

	# Add features to specific rooms.
	app_logger.debug(':: Adding features to specific rooms...')

	_add_features_to_specific_rooms()

	# Autogenerate teachers.
	app_logger.debug(':: Generating teachers...')

	teachers = _autogenerate_teachers(division_data)

	# Autogenerate GE subjects.
	app_logger.debug(':: Generating GE Subjects...')

	ges = _autogenerate_ge_subjects(teachers, room_features)

	# Autogenerate study plans.
	app_logger.debug(':: Generating Study Plans...')

	num_subject_enrollees = _autogenerate_study_plans(
		teachers, room_features, ges)
	
	# Autogenerate classes.
	app_logger.debug(':: Generating classes for each subject...')
	
	_autogenerate_classes(num_subject_enrollees)

	app_logger.info(
		'Finished generating timetable data. Have a great day! 🙂')


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
	for feature in features:
		app_logger.debug(f'- Generating room feature, {feature}.')

		room_feature = models.RoomFeature()
		room_feature.name = feature
		room_feature.save()


def _get_room_features():
	room_features = dict()
	for feature in models.RoomFeature.select().execute():
		room_features[feature.name] = feature

	return room_features


def _autogenerate_division_related_data(division_data, room_features):
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


def _add_features_to_specific_rooms():
	room_features = _get_room_features()

	# Add features to Room 12.
	_add_features_to_room_12(room_features)

	# Add features to CS labs.
	_add_features_to_cs_labs(room_features)

	# Add features to BS Biology labs.
	app_logger.debug(f'- Adding lab features to BS Biology labs...')

	_add_features_to_bs_bio_labs(room_features)

	# Add features to the MPB.
	app_logger.debug(f'- Adding features to the MPB...')

	_add_features_to_the_mpb(room_features)


def _autogenerate_teachers(division_data):
	with open(settings.NAMES_FILE) as f:
		name_parts = f.read().split()

	names = set()
	teachers = dict()
	for division_name, data in division_data.items():
		app_logger.debug(f'- Generating teachers for {division_name}...')

		division = (models.Division
					.select()
					.where(models.Division.name == division_name)
					.get())

		teachers[division_name] = list()

		for _ in range(data['num_teachers']):
			teacher = _generate_teacher_object(name_parts, names, division)

			teachers[division_name].append(teacher)

	return teachers


def _autogenerate_ge_subjects(teachers, room_features):
	num_ges_created = 0
	ges = dict()
	for division in models.Division.select().execute():
		ges[division.name] = list()
		num_ge_subjects = random.randint(4, 7)
		for _ in range(num_ge_subjects):
			ge_subject = _generate_ge_subject_object(num_ges_created, division)

			app_logger.debug(
				'-- Adding candidate teachers to subject, '
				f'{ge_subject.name}...')

			ge_subject.give_random_candidate_teachers(
				teachers[division.name], 3, 7)

			# Add that the GE subjects require a projector for now.
			subject_room_feature = room_features['Projector']
			app_logger.debug(
				f'-- Adding feature {str(subject_room_feature)} '
				f'to subject, {ge_subject.name}...')
			ge_subject.required_features.add(subject_room_feature)

			ge_subject.save()

			ges[division.name].append(ge_subject)

			num_ges_created += 1

	return ges


def _autogenerate_study_plans(teachers, room_features, ges):
	num_subjects_generated = 0
	num_subject_enrollees = dict()
	for course in models.Course.select().execute():
		for year_level in range(1, 5):
			app_logger.debug(
				f'- Generating study plan for {course} '
				f'at year level {year_level}...')

			study_plan = _generate_study_plan_object(course, year_level)

			# Generate subjects.
			num_subjects_generated = _generate_subjects_for_study_plan(
				study_plan, course, teachers,
				room_features, num_subject_enrollees, num_subjects_generated)

			# Add random GEs.
			app_logger.debug(
				f'- Adding random GEs to study plan, {str(study_plan)}...')
			
			_add_random_ges_to_study_plan(
				study_plan, ges, num_subject_enrollees)

	return num_subject_enrollees


def _autogenerate_classes(num_subject_enrollees):
	for subject, num_enrollees in num_subject_enrollees.items():
		app_logger.debug(
			f'- Generating classes for subject, {str(subject)}...')

		slots_per_class = 30  # Assume 30 slots per class for now.
		num_classes = math.ceil(num_enrollees / slots_per_class)
		for i in range(num_classes):
			app_logger.debug(
				f'-- Generating class number {i} for subject, '
				f'{str(subject)}, with {slots_per_class} slots...')

			subject_class = models.Class()
			subject_class.subject = subject
			subject_class.capacity = slots_per_class
			subject_class.save()


def _add_features_to_room_12(room_features):
	room_12_feature = room_features['Air Conditioner']
	room_12 = models.Room.select().where(models.Room.name == 'Room 12').get()

	app_logger.debug(
		f'- Adding feature {str(room_12_feature)} to {str(room_12)}...')

	room_12.features.add(room_12_feature)
	room_12.save()


def _add_features_to_cs_labs(room_features):
	cs_lab_feature = room_features['Computers']
	
	app_logger.debug(f'- Adding feature {str(cs_lab_feature)} to CS labs...')

	cs_labs = (models.Room
				.select()
				.where(models.Room.name.startswith('CS Lab'))
				.execute())
	for lab in cs_labs:
		app_logger.debug(f'-- Adding feature to lab {str(lab)}...')

		lab.features.add(cs_lab_feature)
		lab.save()


def _add_features_to_bs_bio_labs(room_features):
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


def _add_features_to_the_mpb(room_features):
	mpb_feature = room_features['Wide Space']

	mpb = models.Room.select().where(models.Room.name == 'MPB').get()
	
	app_logger.debug(f'-- Adding feature {str(mpb_feature)} to {str(mpb)}...')

	mpb.features.add(mpb_feature)
	mpb.save()


def _generate_teacher_object(name_parts, names, division):
	while True:
		first_name = random.choice(name_parts)
		last_name = random.choice(name_parts)
		teacher_name = f'{first_name} {last_name}'

		if teacher_name in names:
			continue

		break

	names.add(teacher_name)

	app_logger.debug(f'-- Generating teacher, {teacher_name}...')

	teacher = models.Teacher()
	teacher.first_name = first_name
	teacher.last_name = last_name
	teacher.division = division
	# TODO: Add unpreferred timeslots.
	teacher.save()

	return teacher


def _generate_ge_subject_object(num_ges_created, division):
	subject_name = f'GE {num_ges_created} - {str(division)}'

	app_logger.debug(f'- Generating GE subject, {subject_name}..')

	ge_subject = models.Subject()
	ge_subject.name = subject_name
	ge_subject.units = 3.0
	ge_subject.division = division

	# GE subject must be saved first before we are able to assign
	# candidate teachers to it.
	ge_subject.save()

	return ge_subject


def _generate_study_plan_object(course, year_level):
	# Assume for now that all courses have four year levels.
	study_plan = models.StudyPlan()
	study_plan.course = course
	study_plan.year_level = year_level
	study_plan.num_followers = random.randint(35, 75)

	# Study plan must be saved first before we are able to assign
	# subjects to it.
	study_plan.save()

	return study_plan


def _generate_subjects_for_study_plan(study_plan,
									  course,
									  teachers,
									  room_features,
									  num_subject_enrollees,
									  num_subjects_generated):
	num_subjects_to_generate = random.randint(6, 8)
	for _ in range(num_subjects_to_generate):
		is_subject_lab = True if random.random() < 0.1 else False
		subject_name = (
			f'Subject {num_subjects_generated} - {str(course.division)}')

		_generate_subject(
			subject_name, study_plan, num_subject_enrollees,
			course, teachers, room_features, is_subject_lab)
		
		if is_subject_lab:
			# Let's create a lecture subject counterpart.
			_generate_subject(
				subject_name, study_plan, num_subject_enrollees,
				course, teachers, room_features, is_subject_lab=False)

		num_subjects_generated += 1

	return num_subjects_generated


def _add_random_ges_to_study_plan(study_plan, ges, num_subject_enrollees):
	num_ges_to_add = random.randint(3, 4)
	selected_ges = set()
	for _ in range(num_ges_to_add):
		selected_division = random.choice(list(ges.keys()))
		while True:
			selected_ge = random.choice(ges[selected_division])
			if selected_ge not in selected_ges:
				selected_ges.add(selected_ge)
				break

		app_logger.debug(f'-- Adding GE subject, {str(selected_ge)}...')

		study_plan.subjects.add(selected_ge)

		if selected_ge not in num_subject_enrollees:
			num_subject_enrollees[selected_ge] = study_plan.num_followers
		else:
			num_subject_enrollees[selected_ge] += study_plan.num_followers

	study_plan.save()


def _generate_subject(subject_name,
					  study_plan,
					  num_subject_enrollees,
					  course,
					  teachers,
					  room_features,
					  is_subject_lab):
	if is_subject_lab:
		subject_name = f'(Lab) {subject_name}'

	subject = _generate_subject_object(subject_name, course, is_subject_lab)

	app_logger.debug(
		'--- Adding candidate teachers '
		f'to subject, {subject.name}...')

	subject.give_random_candidate_teachers(
		teachers[subject.division.name], 3, 7)

	_apply_features_to_subject(subject, course, room_features, is_subject_lab)

	study_plan.subjects.add(subject)

	# No need to check if the subject instance is in
	# num_subject_enrollees since this is the first time
	# we would create an entry for the subject instance.
	num_subject_enrollees[subject] = study_plan.num_followers


def _generate_subject_object(subject_name, course, is_subject_lab):
	app_logger.debug(f'-- Generating and adding subject {subject_name}...')

	subject = models.Subject()				
	subject.name = subject_name
	subject.division = course.division

	if is_subject_lab:
		# Lab subjects don't have units.
		subject.units = 0.0
	else:
		# It's a lecture subject.
		subject.units = 3.0

	# Subject must be saved first before we are able to assign
	# candidate teachers to it.
	subject.save()

	return subject


def _apply_features_to_subject(subject, course, room_features, is_subject_lab):
	subject_room_features = list()
	subject_room_features.append(room_features['Projector'])
	if is_subject_lab:
		lab_types = [
			'Chemistry', 'Physics', 'Botany',
			'Zoology', 'Computers'
		]
		lab_type = random.choice(lab_types)
		if lab_type == 'Computers':
			subject_room_features.append(
				room_features['Computers'])
		elif (course.division.name
				== 'Division of Natural Sciences and Mathematics'):
			# The BS Biology lab equipments must be restricted to DNSM.
			subject_room_features.append(
				room_features[f'{lab_type} Lab Equipment'])

	for feature in subject_room_features:
		app_logger.debug(
			f'-- Adding feature {str(feature)} '
			f'to subject, {subject.name}...')
		subject.required_features.add(feature)

	subject.save()
