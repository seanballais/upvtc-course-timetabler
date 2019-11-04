import peewee

from upvtc_ct import settings, utils


db = peewee.SqliteDatabase(None)


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

	# TODO: Populate the time slots table.


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
		self.date_modified = utils.current_datetime
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


class Room(Base):
	name = peewee.CharField(
		default=None,
		unique=True,
		null=True,
		max_length=128)
	features = peewee.ManyToManyField(
		RoomFeature,
		backref='rooms',
		on_delete='CASCADE',
		on_update='CASCADE')

	class Meta:
		indexes = (
			(( 'name', ), True),
		)


class TimeSlot(Base):
	start_time = peewee.TimeField(
		default='00:00:00',
		unique=False,
		null=False,
		formats=[ '%H:%M:%S' ])
	end_time = peewee.TimeField(
		default='00:00:00',
		unique=False,
		null=False,
		formats=[ '%H:%M:%S' ])
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
