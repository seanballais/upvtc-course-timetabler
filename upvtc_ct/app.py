import logging
import os

import docopt

from upvtc_ct import __version__, models, settings


def main():
	# Set up logging.
	app_logger = logging.getLogger()
	log_level = (logging.DEBUG if bool(int(os.getenv('UPVTC_CT_DEBUG')))
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
		 '  upvtc_ct\n'
		 '  upvtc_ct (-h | --help)\n'
		 '  upvtc_ct --version\n'
		 '\n'
		 'Options:\n'
		 '  -h --help   Show this help text.\n'
		 '  --version   Show version.\n')
	arguments = docopt.docopt(doc_string, version=__version__)

	# Make sure we have an application folder already. We store our database
	# file in the application folder.
	if not os.path.isdir(settings.APP_DATA_DIR):
		# TODO: Use a log function instead of print().
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
		# TODO: Use a log function instead of print().
		app_logger.info(
			 'Application database file non-existent. Creating database '
			f'file, {settings.DB_FILE}, at {settings.APP_DATA_DIR}, '
			 'and populating it with tables...')

		models.setup_models()


if __name__ == '__main__':
	main()
