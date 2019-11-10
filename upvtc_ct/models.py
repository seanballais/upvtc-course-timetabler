import datetime
import logging

import peewee

from upvtc_ct import settings, utils

# We really need tests around here.

db = peewee.SqliteDatabase(None, pragmas=(('foreign_keys', 'on',),))
app_logger = logging.getLogger()


def setup_models():
	ClassTimeSlotThroughDeferred.set_model(ClassTimeSlot)

	db.init(settings.DB_FILE)
	db.create_tables([
		Division,
		Course,
		Room,
		RoomFeature,
		Room.features.get_through_model(),
		TimeSlot,
		Teacher,
		Teacher.preferred_timeslots.get_through_model(),
		Subject,
		Subject.candidate_teachers.get_through_model(),
		Class,
		Class.required_features.get_through_model(),
		Class.timeslots.get_through_model(),
		StudyPlan,
		StudyPlan.subjects.get_through_model()
	])

	# Timeslots should only fall within the range of 7AM to 7PM, and with their
	# day field only set to 0, 1, or 2. Timeslots are only 30 minutes in
	# length.
	time_hour = lambda time : time // 100
	time_min = lambda time : int(((time % 100) / 100) * 60)
	for day in range(3):
		# We are using a military 24-hour clock here. However, we will be
		# representing 30 minutes as 50, instead of 30, since only integers
		# can be used in range() and the function will, as a result, assume an
		# interval of 100 units.
		for start_time in range(700, 1900, 50):
			app_logger.debug(
				f'Creating a day {day} timeslot with start time of '
				f'{time_hour(start_time):02}:{time_min(start_time):02}...')
			end_time = start_time + 50
			TimeSlot.create(
				start_time=datetime.time(
					time_hour(start_time), time_min(start_time), 0),
				end_time=datetime.time(
					time_hour(end_time), time_min(end_time), 0),
				day=day)


class Base(peewee.Model):
	date_created = peewee.DateTimeField(
		default=utils.current_datetime,
		null=False,
		unique=False)
	date_modified = peewee.DateTimeField(
		default=utils.current_datetime,
		null=False,
		unique=False)

	def save(self, *args, **kwargs):
		self.date_modified = utils.current_datetime()
		return super(Base, self).save(*args, **kwargs)

	class Meta:
		database = db


class Division(Base):
	name = peewee.CharField(
		default=None,
		unique=True,
		null=True,
		max_length=128)

	class Meta:
		indexes = (
			(( 'name', ), True),
		)

	def __str__(self):
		return str(self.name)


class Course(Base):
	name = peewee.CharField(
		default=None,
		unique=True,
		null=True,
		max_length=128)
	division = peewee.ForeignKeyField(
		Division,
		backref='courses',
		on_delete='CASCADE',
		on_update='CASCADE')

	class Meta:
		indexes = (
			(( 'name', ), True),
		)

	def __str__(self):
		return str(self.name)


class RoomFeature(Base):
	name = peewee.CharField(
		default='',
		unique=True,
		null=False,
		max_length=64)

	class Meta:
		indexes = (
			(( 'name', ), True),
		)

	def __str__(self):
		return str(self.name)


class Room(Base):
	name = peewee.CharField(
		default=None,
		unique=True,
		null=True,
		max_length=128)
	division = peewee.ForeignKeyField(
		Division,
		backref='rooms',
		null=True,
		on_delete='SET NULL',
		on_update='CASCADE')
	features = peewee.ManyToManyField(
		RoomFeature,
		backref='rooms',
		on_delete='CASCADE',
		on_update='CASCADE')

	class Meta:
		indexes = (
			(( 'name', ), True),
		)

	def __str__(self):
		return str(self.name)


class TimeSlot(Base):
	start_time = peewee.TimeField(
		default=datetime.time(0, 0, 0),
		unique=False,
		null=False,
		formats=[ '%H:%M' ])
	end_time = peewee.TimeField(
		default=datetime.time(0, 0, 0),
		unique=False,
		null=False,
		formats=[ '%H:%M' ])
	day = peewee.SmallIntegerField(  # Same explanation as the one in `Class`,
		default=0,					 # but with the reasoning that day will
		null=False)					 # only have a maximum value of 2. 0
									 # represents Mondays and Thursdays, 1
									 # representing Tuesday and Fridays, and
									 # 2 representing Wednesday. We are not
									 # scheduling graduate subjects and ROTC
									 # at the moment.

	class Meta:
		indexes = (
			(( 'start_time', 'end_time', 'day', ), True),
			(( 'start_time', ), False),
			(( 'end_time', ), False),
			(( 'day', ), False),
		)

	def __str__(self):
		return (
			f'(Day {str(self.day)} '
			f'{str(self.start_time)} - {str(self.end_time)}')


class Teacher(Base):
	first_name = peewee.CharField(
		default=None,
		unique=False,
		null=True,
		max_length=128)
	last_name = peewee.CharField(
		default=None,
		unique=False,
		null=True)
	preferred_timeslots = peewee.ManyToManyField(
		TimeSlot,
		backref='preferring_teachers',
		on_delete='CASCADE',
		on_update='CASCADE')

	class Meta:
		indexes = (
			(( 'first_name', 'last_name', ), True),
		)

	def __str__(self):
		return f'{str(self.last_name)}, {str(self.first_name)}'


class Subject(Base):
	name = peewee.CharField(
		default=None,
		unique=True,
		null=True,
		max_length=128)
	units = peewee.DecimalField(
		default=None,
		unique=False,
		null=True,
		max_digits=3,
		decimal_places=2)
	division = peewee.ForeignKeyField(
		Division,
		backref='subjects',
		on_delete='CASCADE',
		on_update='CASCADE')
	candidate_teachers = peewee.ManyToManyField(
		Teacher,
		backref='candidate_subjects',
		on_delete='CASCADE',
		on_update='CASCADE')

	class Meta:
		indexes = (
			(( 'name', ), True),
		)

	def __str__(self):
		return str(self.name)


# We have to use a deferred through model since we would have a circular
# dependency between Class and ClassTimeSlot if we set the through model in
# Class's timeslots to ClassTimeSlot.
#
# Please refer to the following link:
# http://docs.peewee-orm.com/en/latest/peewee/api.html#DeferredThroughModel
ClassTimeSlotThroughDeferred = peewee.DeferredThroughModel()

class Class(Base):
	subject = peewee.ForeignKeyField(
		Subject,
		backref='classes',
		on_delete='CASCADE',
		on_update='CASCADE')
	assigned_teacher = peewee.ForeignKeyField(
		Teacher,
		backref='classes',
		on_delete='CASCADE',
		on_update='CASCADE')
	capacity = peewee.SmallIntegerField(  # SQLite does not have support for
		default=None,                     # small integer fields. They are
		null=True)						  # automatically converted to integer
										  # fields. We are using this field
										  # type for semantic purposes since
										  # the capacity of a class in UPVTC
										  # is usually less than the maximum
										  # value of a small integer field in
										  # RDBMSes.
	required_features = peewee.ManyToManyField(
		RoomFeature,
		backref=None,         # No need for the room features to know what
							  # classes require them.
		on_delete='CASCADE',
		on_update='CASCADE')
	timeslots = peewee.ManyToManyField(
		TimeSlot,
		backref='classes',
		through_model=ClassTimeSlotThroughDeferred)

	class Meta:
		indexes = (
			(( 'subject', ), False),
			(( 'capacity', ), False),
		)

	def __str__(self):
		return str(self.subject)


class ClassTimeSlot(Base):
	subject_class = peewee.ForeignKeyField(
		Class,
		on_delete='CASCADE',
		on_update='CASCADE')
	timeslot = peewee.ForeignKeyField(
		TimeSlot,
		on_delete='RESTRICT',
		on_update='RESTRICT')

	class Meta:
		indexes = (
			(( 'subject_class', ), False),
			(( 'timeslot', ), False),
		)


class StudyPlan(Base):
	subjects = peewee.ManyToManyField(
		Subject,
		backref='host_study_plans',
		on_delete='CASCADE',
		on_update='CASCADE')
	course = peewee.ForeignKeyField(
		Course,
		backref='study_plans',
		on_delete='CASCADE',
		on_update='CASCADE')
	year_level = peewee.SmallIntegerField(     # Same explanation as the one in
		default=None,                          # Class, but with the reasoning
		null=True) 							   # that the values of year_level
  											   # can only be inclusively
											   # between 1 to 5.
	num_followers = peewee.SmallIntegerField(  # Same explanation again as the
		default=None,						   # one in Class. But, instead of
		null=True)							   # class capacity, we are using
											   # the number of followers of the
											   # study plan, which, at the
											   # moment, will refer to the
											   # number of regular students
											   # there are in a course in a
											   # particular year level.
	class Meta:
		indexes = (
			( ( 'course', 'year_level', ), True),
		)

	def __str__(self):
		return f'{str(self.course)} {str(self.year_level)}'
