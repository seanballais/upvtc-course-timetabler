import peewee

from upvtc_ct import settings, utils


def setup_models():
	pass


class Base(peewee.Model):
	date_created = peewee.DateTimeField(
		default=utils.current_time,
		null=False,
		unique=False)
	date_modified = peewee.DateTimeField(
		default=utils.current_time,
		null=False,
		unique=False)

	def save(self, *args, **kwargs):
		self.date_modified = utils.current_time
		return super(Base, self).save(*args, **kwargs)

	class Meta:
		database = peewee.SqliteDatabase(settings.DB_FILE)
