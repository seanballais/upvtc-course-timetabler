import logging
import os
import sys

from PyQt5 import QtWidgets
import docopt

from upvtc_ct import __version__, gui, models, scheduler, settings


def main():
	# Set up logging.
	if not bool(int(os.getenv('UPVTC_CT_DB_DEBUG', 0))):
		# Disable Peewee logging since the environment variable is not set to a
		# non-zero value, since Peewee should only log when we need it to, to
		# reduce noise in the console logs. By default, whenever the logging
		# level of the root logger is set to DEBUG (which happens whenever
		# UPVTC_CT_DEBUG is set to a non-zero value), Peewee logs.
		peewee_logger = logging.getLogger('peewee')
		peewee_logger.propagate = False

	app_logger = logging.getLogger()
	log_level = (logging.DEBUG if bool(int(os.getenv('UPVTC_CT_DEBUG', 0)))
							   else logging.INFO)
	log_formatter = logging.Formatter(
		'%(asctime)s | [%(levelname)s] %(message)s')
	app_logger.setLevel(log_level)
	
	console_handler = logging.StreamHandler()  # We should always log to the
											   # console.
	console_handler.setFormatter(log_formatter)
	app_logger.addHandler(console_handler)

	# Set up argument parsing and obtain arguments through docopt.
	doc_string = (
		f'Automated Course Timetabler for UPVTC (v{__version__})\n'
		 '\n'
		 'Usage:\n'
		 '  upvtc_ct [--no-gui] [--reset-teacher-assignments] '
		 '[--assign-teachers-to-classes] [--view-text-form-class-conflicts] '
		 '[--reset-schedule]\n'
		 '           [--schedule [--population-size=<ps> '
		 '--num-generations=<ng> --mutation-chance=<mc>]]'
		 ' [--view-text-form-schedule]\n'
		 '  upvtc_ct (-h | --help)\n'
		 '  upvtc_ct --version\n'
		 '\n'
		 'Options:\n'
		 '  -h --help                         Show this help text.\n'
		 '  --version                         Show version.\n'
		 '  --no-gui                          Run without a GUI.\n'
		 '  --reset-teacher-assignments       Reset the class assignments of '
		 'teachers.\n'
		 '  --assign-teachers-to-classes      Assign teachers to classes '
		 'before performing any further actions.\n'
		 '  --view-text-form-class-conflicts  Show the classes that conflict '
		 'or share students for each class.\n'
		 '  --schedule                        Create a schedule.\n'
		 '  --population-size=<ps>            Sets the population size '
		 '[default: 25].\n'
		 '  --num-generations=<ng>            Sets the number of generations '
		 'to generate [default: 10].\n'
		 '  --mutation-chance=<mc>            Sets the mutation chance of '
		 'offsprings in each generation [default: 0.2].\n'
		 '  --reset-schedule                  Resets the schedule.\n'
		 '  --view-text-form-schedule         View the schedule in text form.')
	arguments = docopt.docopt(doc_string, version=__version__)

	# Make sure we have an application folder already. We store our database
	# file in the application folder.
	if not os.path.isdir(settings.APP_DATA_DIR):
		app_logger.info(
			 'Application folder non-existent. Creating at '
			f'{ settings.APP_DATA_DIR }... ')
		
		os.mkdir(settings.APP_DATA_DIR)

	# At this point, we're sure that we have an application folder already.
	# We can now write logs directly to the log file.
	log_file_handler = logging.FileHandler(settings.LOG_FILE)
	log_file_handler.setFormatter(log_formatter)
	app_logger.addHandler(log_file_handler)

	# Make sure we already have the database file.
	if not os.path.isfile(settings.DB_FILE):
		app_logger.info(
			 'Application database non-existent. Creating database, '
			f'{settings.DB_FILE} (at {settings.APP_DATA_DIR}), '
			 'initializing it, and populating it with tables...')

		models.setup_models()
	else:
		app_logger.info(
			f'Application database ({settings.DB_FILE}) already exists. '
			 'Initializing database...')
		models.db.init(settings.DB_FILE)

	# We got everything setup so we can start the application proper based on
	# the passed arguments.
	if arguments['--reset-teacher-assignments']:
		scheduler.reset_teacher_assignments()

	if arguments['--assign-teachers-to-classes']:
		scheduler.assign_teachers_to_classes()

	if arguments['--view-text-form-class-conflicts']:
		scheduler.view_text_form_class_conflicts()

	if arguments['--reset-schedule']:
		scheduler.reset_schedule()

	if arguments['--schedule']:
		scheduler.create_schedule(
			int(arguments['--population-size']),
			int(arguments['--num-generations']),
			float(arguments['--mutation-chance']))

	if arguments['--view-text-form-schedule']:
		scheduler.view_text_form_schedule()

	if not arguments['--no-gui']:
		app_gui = QtWidgets.QApplication([])

		main_window = gui.MainWindow()
		main_window.show()

		sys.exit(app_gui.exec_())


if __name__ == '__main__':
	main()
